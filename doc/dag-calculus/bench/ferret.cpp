// A pipeline that answers similarity queries: for each query image, the ten
// most similar entries of a database, written in the order of the queries.
//
//   first    load the image                        in order
//   middle   extract its features, and rank the    in parallel
//            database by distance to them
//   last     write the answer                      in order
//
// The shape is that of the ferret benchmark of PARSEC as pipelined by Lee et
// al. (SPAA 2013): a serial stage, parallel stages, a serial stage. It is
// much simplified. The images are synthetic, the features are sums over the
// cells of a grid, and the ranking scans the whole database, where ferret
// segments an image, indexes with locality-sensitive hashing and ranks by
// the earth mover's distance. As in ferret, nearly all of the work is in the
// parallel stages.
//
// usage: ferret <serial|dag> [queries] [database] [throttle] [check]

#include "harness.hpp"
#include "pipe.hpp"

#include <vector>

using namespace bench;

static constexpr int side = 64;       // an image has side x side pixels
static constexpr int cells = 8;       // and cells x cells cells
static constexpr int dims = cells * cells;
static constexpr int nb_best = 10;

using image = std::vector<uint8_t>;
struct features {
  float x[dims];
};
struct answer {
  int ids[nb_best];
};

// The images come from one stream of random numbers, as a file would be
// read: image i cannot be made before image i - 1.
struct loader {
  uint64_t state = 42;
  void load(image &im) {
    im.resize(side * side);
    // A few bright rectangles on a noisy background.
    for (auto &px : im) {
      state = hash(state);
      px = (uint8_t)(state & 31);
    }
    for (int k = 0; k < 6; k++) {
      state = hash(state);
      int x0 = (int)(state % side), y0 = (int)((state >> 8) % side);
      int w = 4 + (int)((state >> 16) % 24), h = 4 + (int)((state >> 24) % 24);
      uint8_t level = (uint8_t)(64 + ((state >> 32) % 160));
      for (int y = y0; (y < y0 + h) && (y < side); y++) {
        for (int x = x0; (x < x0 + w) && (x < side); x++) {
          im[y * side + x] = level;
        }
      }
    }
  }
};

__attribute__((noinline)) static void extract(const image &im, features &f) {
  int per_cell = side / cells;
  for (int c = 0; c < dims; c++) {
    int cy = c / cells, cx = c % cells;
    unsigned sum = 0;
    for (int y = cy * per_cell; y < (cy + 1) * per_cell; y++) {
      for (int x = cx * per_cell; x < (cx + 1) * per_cell; x++) {
        sum += im[y * side + x];
      }
    }
    f.x[c] = (float)sum / (float)(per_cell * per_cell);
  }
}

// The nb_best entries nearest to q, nearest first; ties go to the lower id.
__attribute__((noinline)) static void
rank(const std::vector<features> &database, const features &q, answer &a) {
  float best[nb_best];
  int nb = 0;
  for (int id = 0; id < (int)database.size(); id++) {
    const features &e = database[id];
    float d = 0.0f;
    for (int c = 0; c < dims; c++) {
      float diff = e.x[c] - q.x[c];
      d += diff * diff;
    }
    if ((nb == nb_best) && !(d < best[nb - 1])) {
      continue;
    }
    int k = (nb < nb_best) ? nb++ : nb_best - 1;
    for (; (k > 0) && (d < best[k - 1]); k--) {
      best[k] = best[k - 1];
      a.ids[k] = a.ids[k - 1];
    }
    best[k] = d;
    a.ids[k] = id;
  }
  for (int k = nb; k < nb_best; k++) {
    a.ids[k] = -1;
  }
}

struct writer {
  uint64_t digest = 0;
  size_t nb_written = 0;
  void write(const answer &a) {
    for (int k = 0; k < nb_best; k++) {
      digest = hash(digest ^ (uint64_t)(uint32_t)a.ids[k]);
    }
    nb_written++;
  }
};

struct query {
  image im;
  answer a;
};

static void ferret_serial(const std::vector<features> &database,
                          size_t nb_queries, writer &w) {
  loader l;
  for (size_t i = 0; i < nb_queries; i++) {
    query q;
    l.load(q.im);
    features f;
    extract(q.im, f);
    rank(database, f, q.a);
    w.write(q.a);
  }
}

static void ferret_dag(const std::vector<features> &database,
                       size_t nb_queries, size_t throttle, writer &w) {
  loader l;
  pipeline::pipe_while<query>(
      throttle,
      [&](size_t i, query &q) {
        if (i == nb_queries) {
          return false;
        }
        l.load(q.im);
        return true;
      },
      [&](pipeline::stages &, query &q) {
        features f;
        extract(q.im, f);
        rank(database, f, q.a);
      },
      [&](query &q) { w.write(q.a); });
}

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  size_t nb_queries = (size_t)arg(argc, argv, 2, 6000);
  size_t nb_entries = (size_t)arg(argc, argv, 3, 20000);
  size_t throttle = (size_t)arg(argc, argv, 4, 0);
  bool check = wants_check(argc, argv, 5);
  bool parallel = !is(version, "serial");
  if (parallel && !is(version, "dag")) {
    std::fprintf(stderr, "usage: ferret <serial|dag> [queries] [database] "
                         "[throttle] [check]\n");
    return 2;
  }
  if (throttle == 0) {
    throttle = 4 * get_nb_workers();
  }
  // The database: the features of images from a stream of their own.
  std::vector<features> database(nb_entries);
  {
    loader l;
    l.state = 7;
    image im;
    for (auto &e : database) {
      l.load(im);
      extract(im, e);
    }
  }
  writer w;
  region r = measure(parallel, [&] {
    if (parallel) {
      ferret_dag(database, nb_queries, throttle, w);
    } else {
      ferret_serial(database, nb_queries, w);
    }
  });
  std::printf("ferret %s queries=%zu database=%zu throttle=%zu\n", version,
              nb_queries, nb_entries, throttle);
  std::printf("output    %zu answers, digest %016llx\n", w.nb_written,
              (unsigned long long)w.digest);
  report(r);
  if (check) {
    writer reference;
    ferret_serial(database, nb_queries, reference);
    return report_check((reference.digest == w.digest) &&
                        (w.nb_written == nb_queries));
  }
  return 0;
}
