// A parallel pipeline that deduplicates and compresses a stream of chunks.
// Each chunk passes through four stages:
//
//   0  read the chunk                      in order, one chunk at a time
//   1  hash it, and look the hash up       in order, one chunk at a time
//   2  compress it, unless it is a         in parallel with other chunks
//      duplicate
//   3  write it, or a reference to the     in order, one chunk at a time
//      earlier chunk with the same hash
//
// The stages are those of the dedup benchmark as pipelined by Lee et al.
// (SPAA 2013, Figure 4), much simplified: chunks have a fixed size, the hash
// is FNV-1a, and the compressor is run-length encoding.
//
// usage: pipeline <serial|dag> [chunks] [chunk bytes] [throttle] [check]

#include "microtaskparts.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <unordered_set>
#include <vector>

using namespace microtaskparts;

using bytes = std::vector<uint8_t>;

static size_t chunk_szb;

// Fill the input with chunks drawn from a small pool of contents, so that
// many chunks are duplicates, and each of which has runs of equal bytes.
static void generate(bytes &input, size_t nb_chunks) {
  input.resize(nb_chunks * chunk_szb);
  size_t nb_contents = std::max<size_t>(1, nb_chunks / 4);
  for (size_t i = 0; i < nb_chunks; i++) {
    uint64_t state = hash(hash(i) % nb_contents);
    uint8_t *x = &input[i * chunk_szb];
    size_t k = 0;
    while (k < chunk_szb) {
      state = hash(state);
      size_t run = 1 + (state & 15);
      uint8_t value = (uint8_t)(state >> 8);
      for (; (run > 0) && (k < chunk_szb); run--) {
        x[k++] = value;
      }
    }
  }
}

// Stage 1.
struct deduplicator {
  std::unordered_set<uint64_t> seen;

  static uint64_t digest(const uint8_t *x) {
    uint64_t d = 14695981039346656037ul;
    for (size_t k = 0; k < chunk_szb; k++) {
      d = (d ^ x[k]) * 1099511628211ul;
    }
    return d;
  }
  // Returns whether a chunk with digest d has been seen before.
  bool is_duplicate(uint64_t d) { return !seen.insert(d).second; }
};

// Stage 2.
static bytes compress(const uint8_t *x) {
  bytes y;
  for (size_t k = 0; k < chunk_szb;) {
    size_t run = 1;
    while ((k + run < chunk_szb) && (run < 255) && (x[k + run] == x[k])) {
      run++;
    }
    y.push_back((uint8_t)run);
    y.push_back(x[k]);
    k += run;
  }
  return y;
}

// Stage 3.
struct writer {
  bytes output;

  void append(uint64_t v) {
    for (int b = 0; b < 8; b++) {
      output.push_back((uint8_t)(v >> (8 * b)));
    }
  }
  void write_reference(uint64_t d) {
    output.push_back('R');
    append(d);
  }
  void write_chunk(uint64_t d, const bytes &y) {
    output.push_back('C');
    append(d);
    append(y.size());
    output.insert(output.end(), y.begin(), y.end());
  }
};

static void pipeline_serial(const bytes &input, size_t nb_chunks,
                            deduplicator &dd, writer &w) {
  for (size_t i = 0; i < nb_chunks; i++) {
    const uint8_t *x = &input[i * chunk_szb];
    uint64_t d = deduplicator::digest(x);
    if (dd.is_duplicate(d)) {
      w.write_reference(d);
    } else {
      w.write_chunk(d, compress(x));
    }
  }
}

// The loop runs stages 0 and 1, and makes one vertex per chunk. The vertex
// runs stage 2, waits for the vertex of the previous chunk, and runs stage
// 3. The edge from the previous vertex is added while that vertex is
// running, or after it has finished.
//
// At most throttle chunks are in progress: before it starts chunk i, the
// loop waits for chunk i - throttle.
static void pipeline_dag(const bytes &input, size_t nb_chunks, size_t throttle,
                         deduplicator &dd, writer &w) {
  // The vertices that may still be used, oldest first.
  std::deque<std::unique_ptr<vertex>> window;
  for (size_t i = 0; i < nb_chunks; i++) {
    const uint8_t *x = &input[i * chunk_szb];   // stage 0
    uint64_t d = deduplicator::digest(x);       // stage 1
    bool duplicate = dd.is_duplicate(d);
    if (window.size() >= throttle) {
      wait(window[window.size() - throttle].get());
      // That vertex has finished, so nothing waits any longer for the
      // vertices before it.
      while (window.size() > throttle) {
        window.pop_front();
      }
    }
    vertex *previous = window.empty() ? nullptr : window.back().get();
    vertex *v = new_vertex([x, d, duplicate, previous, &w] {
      bytes y;
      if (!duplicate) {
        y = compress(x); // stage 2
      }
      if (previous != nullptr) {
        wait(previous);
      }
      if (duplicate) { // stage 3
        w.write_reference(d);
      } else {
        w.write_chunk(d, y);
      }
    });
    window.emplace_back(v);
    release(v);
  }
  if (!window.empty()) {
    wait(window.back().get());
  }
}

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  size_t nb_chunks = (argc > 2) ? std::atol(argv[2]) : 20000;
  chunk_szb = (argc > 3) ? std::atol(argv[3]) : 16384;
  size_t throttle = (argc > 4) ? std::atol(argv[4]) : 0;
  bool check = (argc > 5) && (std::strcmp(argv[5], "check") == 0);
  if (chunk_szb < 1) {
    std::fprintf(stderr, "pipeline: a chunk must have at least one byte\n");
    return 2;
  }
  bytes input;
  generate(input, nb_chunks);
  deduplicator dd;
  writer w;
  auto start = std::chrono::steady_clock::now();
  if (std::strcmp(version, "serial") == 0) {
    pipeline_serial(input, nb_chunks, dd, w);
  } else if (std::strcmp(version, "dag") == 0) {
    launch([&] {
      if (throttle == 0) {
        throttle = 4 * get_nb_workers();
      }
      pipeline_dag(input, nb_chunks, throttle, dd, w);
    });
  } else {
    std::fprintf(stderr, "pipeline: unknown version %s\n", version);
    return 2;
  }
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
  std::printf("pipeline %s chunks=%zu bytes=%zu throttle=%zu\n", version,
              nb_chunks, chunk_szb, throttle);
  std::printf("output    %zu bytes, %zu distinct chunks\n", w.output.size(),
              dd.seen.size());
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", elapsed.count());
  if (check) {
    deduplicator reference_dd;
    writer reference;
    pipeline_serial(input, nb_chunks, reference_dd, reference);
    bool same = (reference.output == w.output);
    std::printf("check     %s\n", same ? "ok" : "WRONG");
    return same ? 0 : 1;
  }
  return 0;
}
