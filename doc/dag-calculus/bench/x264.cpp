// A pipeline that encodes video, with the dependencies between frames of
// the x264 encoder.
//
// The video is a sequence of groups: a reference frame followed by a few
// in-between frames. A frame is a grid of 16 x 16 macroblocks. A macroblock
// of a reference frame is predicted from the previous reference frame, as
// decoded: the encoder searches that frame for the best match within a few
// pixels, then encodes the difference. So row r of reference frame i can be
// encoded as soon as rows up to r + reach of reference frame i - 1 are, and
// consecutive reference frames are encoded together, one a few rows behind
// the other. The in-between frames of a group are predicted from the two
// reference frames around them, and nothing depends on them, so they are
// encoded in parallel, by fork join, once those two are done.
//
//   first    read the frames of group i               in order
//   middle   encode the reference frame, row by row:
//            row r is stage r + i * reach, and waits
//            for the previous iteration to leave that
//            stage; then encode the in-between frames
//   last     write the group                          in order
//
// The numbering of the stages is the one Lee et al. (SPAA 2013) give x264:
// iteration i starts i * reach stages late, so that waiting for the stage of
// the same number is waiting for the row `reach` rows further down. This is
// a pipeline whose nodes and edges are known only as it runs.
//
// The encoder is much simplified: one search of whole pixels per macroblock,
// a difference that is quantized and not transformed, and a digest in place
// of an entropy coder. The frames are synthetic.
//
// The first stage is serial, so its time bounds the speedup: the program
// prints it, after the time of the whole.
//
// usage: x264 <serial|dag> [groups] [width] [height] [in-between frames]
//             [throttle] [check]

#include "harness.hpp"
#include "pipe.hpp"

#include <vector>

using namespace bench;

static constexpr int mb = 16;    // a macroblock is mb x mb pixels
static constexpr int range = 12; // the search looks this many pixels away
static constexpr int reach = (range + mb - 1) / mb; // in rows of macroblocks
static constexpr int quantum = 6;

static int width, height;   // in pixels, multiples of mb
static int nb_rows;         // of macroblocks
static int nb_between;      // in-between frames per group

using frame = std::vector<uint8_t>;

// Frame number f of the video: two textured layers that move at different
// speeds, the nearer one covering a band of the picture, plus noise.
static void read_frame(int f, frame &out) {
  out.resize((size_t)width * height);
  for (int y = 0; y < height; y++) {
    bool near = (y > height / 3) && (y < 2 * height / 3);
    int sx = near ? 3 * f : f, sy = near ? -f : f / 2;
    for (int x = 0; x < width; x++) {
      uint64_t u = (uint64_t)((x + sx) >> 2), v = (uint64_t)((y + sy) >> 2);
      uint64_t texture = hash(u * 73856093u ^ v * 19349663u ^ (near ? 1 : 0));
      uint64_t noise = hash(((uint64_t)f << 40) ^ ((uint64_t)y << 20) ^ x);
      out[(size_t)y * width + x] =
          (uint8_t)(32 + (texture & 127) + (noise & 7));
    }
  }
}

static int clamp(int v, int lo, int hi) {
  return (v < lo) ? lo : ((v > hi) ? hi : v);
}

// The sum of absolute differences between the macroblock of cur at (x, y)
// and the block of ref at (x + dx, y + dy), which lies inside the frame.
static unsigned sad(const uint8_t *cur, const uint8_t *ref, int x, int y,
                    int dx, int dy) {
  unsigned total = 0;
  for (int j = 0; j < mb; j++) {
    const uint8_t *c = cur + (size_t)(y + j) * width + x;
    const uint8_t *r = ref + (size_t)(y + dy + j) * width + x + dx;
    for (int i = 0; i < mb; i++) {
      total += (unsigned)std::abs((int)c[i] - (int)r[i]);
    }
  }
  return total;
}

struct match {
  unsigned cost;
  int dx, dy;
};

// The best match for the macroblock at (x, y) within the range, among the
// blocks that lie inside the frame. Ties go to the first in scan order.
static match search(const uint8_t *cur, const uint8_t *ref, int x, int y) {
  match best{~0u, 0, 0};
  for (int dy = -range; dy <= range; dy++) {
    if ((y + dy < 0) || (y + dy + mb > height)) {
      continue;
    }
    for (int dx = -range; dx <= range; dx++) {
      if ((x + dx < 0) || (x + dx + mb > width)) {
        continue;
      }
      unsigned cost = sad(cur, ref, x, y, dx, dy);
      if (cost < best.cost) {
        best = match{cost, dx, dy};
      }
    }
  }
  return best;
}

// Encode the macroblock of cur at (x, y) against a prediction: ref moved by
// (dx, dy), or a flat gray if ref is null. Writes what a decoder would
// rebuild into recon, if recon is not null, and returns a digest of the
// encoding, chained from d.
static uint64_t encode_block(const uint8_t *cur, const uint8_t *ref,
                             uint8_t *recon, int x, int y, int dx, int dy,
                             uint64_t d) {
  d = hash(d ^ (((uint64_t)(uint32_t)dx << 32) | (uint32_t)dy));
  for (int j = 0; j < mb; j++) {
    uint64_t line = 0;
    for (int i = 0; i < mb; i++) {
      size_t at = (size_t)(y + j) * width + x + i;
      int predicted =
          (ref == nullptr) ? 128 : ref[(size_t)(y + dy + j) * width + x + dx + i];
      int level = ((int)cur[at] - predicted) / quantum;
      line = line * 31 + (uint64_t)(uint32_t)level;
      if (recon != nullptr) {
        recon[at] = (uint8_t)clamp(predicted + level * quantum, 0, 255);
      }
    }
    d = hash(d ^ line);
  }
  return d;
}

// Row r of a reference frame. prev is the previous reference frame as
// decoded, or null for the first frame of the video.
__attribute__((noinline)) static uint64_t
encode_reference_row(const frame &cur, const frame *prev, frame &recon, int r) {
  uint64_t d = (uint64_t)r;
  for (int x = 0; x < width; x += mb) {
    match m{0, 0, 0};
    if (prev != nullptr) {
      m = search(cur.data(), prev->data(), x, r * mb);
    }
    d = encode_block(cur.data(), (prev == nullptr) ? nullptr : prev->data(),
                     recon.data(), x, r * mb, m.dx, m.dy, d);
  }
  return d;
}

// Row r of an in-between frame, predicted from whichever of the two
// reference frames around it matches better.
__attribute__((noinline)) static uint64_t
encode_between_row(const frame &cur, const frame &before, const frame &after,
                   int r) {
  uint64_t d = (uint64_t)r;
  for (int x = 0; x < width; x += mb) {
    match m0 = search(cur.data(), before.data(), x, r * mb);
    match m1 = search(cur.data(), after.data(), x, r * mb);
    bool second = (m1.cost < m0.cost);
    match m = second ? m1 : m0;
    d = hash(d ^ (second ? 1 : 0));
    d = encode_block(cur.data(), second ? after.data() : before.data(),
                     nullptr, x, r * mb, m.dx, m.dy, d);
  }
  return d;
}

// One iteration of the pipeline. Group i is the reference frame numbered
// i * (nb_between + 1) and the in-between frames just before it.
struct group {
  int i = 0;
  frame reference;
  std::vector<frame> between;
  std::vector<uint64_t> digests; // one per row of each frame
};

// The reference frames as decoded. Iteration i uses number i modulo their
// count.
static std::vector<frame> decoded;
static frame &decoded_of(int i) { return decoded[(size_t)i % decoded.size()]; }

// The time spent in the first stage, in seconds. Its calls are in order, in
// every version, so nothing else writes this while one of them runs.
static double reading = 0.0;

static void read_group(int i, group &g) {
  struct timer {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    ~timer() {
      std::chrono::duration<double> d = std::chrono::steady_clock::now() - start;
      reading += d.count();
    }
  } t;
  g.i = i;
  int f = i * (nb_between + 1);
  read_frame(f, g.reference);
  g.between.resize((i == 0) ? 0 : nb_between);
  for (int b = 0; b < (int)g.between.size(); b++) {
    read_frame(f - nb_between + b, g.between[b]);
  }
  g.digests.assign((1 + g.between.size()) * nb_rows, 0);
}

static void encode_row(group &g, int r) {
  g.digests[r] = encode_reference_row(
      g.reference, (g.i == 0) ? nullptr : &decoded_of(g.i - 1),
      decoded_of(g.i), r);
}

static void encode_between(group &g, int b, int r) {
  g.digests[(1 + b) * nb_rows + r] = encode_between_row(
      g.between[b], decoded_of(g.i - 1), decoded_of(g.i), r);
}

struct writer {
  uint64_t digest = 0;
  size_t nb_frames = 0;
  void write(const group &g) {
    for (uint64_t d : g.digests) {
      digest = hash(digest ^ d);
    }
    nb_frames += 1 + g.between.size();
  }
};

static void x264_serial(int nb_groups, writer &w) {
  for (int i = 0; i < nb_groups; i++) {
    group g;
    read_group(i, g);
    for (int r = 0; r < nb_rows; r++) {
      encode_row(g, r);
    }
    for (int b = 0; b < (int)g.between.size(); b++) {
      for (int r = 0; r < nb_rows; r++) {
        encode_between(g, b, r);
      }
    }
    w.write(g);
  }
}

// The decoded frame of group i is overwritten by group i + throttle + 1,
// which starts only once group i + 1, the last to read it, has finished.
static void x264_dag(int nb_groups, size_t throttle, writer &w) {
  // Past every stage that is a row.
  const long rows_done = LONG_MAX / 2;
  pipeline::pipe_while<group>(
      throttle,
      [&](size_t i, group &g) {
        if ((int)i == nb_groups) {
          return false;
        }
        read_group((int)i, g);
        return true;
      },
      [&](pipeline::stages &s, group &g) {
        for (int r = 0; r < nb_rows; r++) {
          s.wait(1 + r + (long)g.i * reach);
          encode_row(g, r);
        }
        // Let the next reference frame finish before our in-between frames.
        s.next(rows_done);
        long nb = (long)g.between.size() * nb_rows;
        parallel_for(0, nb, [&](long k) {
          encode_between(g, (int)(k / nb_rows), (int)(k % nb_rows));
        });
      },
      [&](group &g) { w.write(g); });
}

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  int nb_groups = (int)arg(argc, argv, 2, 60);
  width = (int)arg(argc, argv, 3, 640);
  height = (int)arg(argc, argv, 4, 368);
  nb_between = (int)arg(argc, argv, 5, 2);
  size_t throttle = (size_t)arg(argc, argv, 6, 0);
  bool check = wants_check(argc, argv, 7);
  bool parallel = !is(version, "serial");
  if ((parallel && !is(version, "dag")) || (width < mb) || (height < mb) ||
      (width % mb != 0) || (height % mb != 0) || (nb_between < 0) ||
      (nb_groups < 0)) {
    std::fprintf(stderr,
                 "usage: x264 <serial|dag> [groups] [width] [height] "
                 "[in-between frames] [throttle] [check]\n"
                 "the width and the height are multiples of %d\n", mb);
    return 2;
  }
  if (throttle == 0) {
    throttle = 4 * get_nb_workers();
  }
  nb_rows = height / mb;
  decoded.assign(parallel ? throttle + 1 : 2, frame((size_t)width * height));
  writer w;
  region r = measure(parallel, [&] {
    if (parallel) {
      x264_dag(nb_groups, throttle, w);
    } else {
      x264_serial(nb_groups, w);
    }
  });
  std::printf("x264 %s groups=%d width=%d height=%d between=%d throttle=%zu\n",
              version, nb_groups, width, height, nb_between, throttle);
  std::printf("output    %zu frames, digest %016llx\n", w.nb_frames,
              (unsigned long long)w.digest);
  report(r);
  std::printf("reading   %.3f s in the first stage\n", reading);
  if (check) {
    decoded.assign(2, frame((size_t)width * height));
    writer reference;
    x264_serial(nb_groups, reference);
    return report_check((reference.digest == w.digest) &&
                        (reference.nb_frames == w.nb_frames));
  }
  return 0;
}
