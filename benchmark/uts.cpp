#include "treesearchpar.h"
#include "uts.c"
#include "rng/brg_sha1.c"

// ===========================================================================

int main(int argc, char *argv[]) {
  UTSConfig config;
  Node root;
  double t1, t2;

  uts_parseParams(&config, argc, argv);
  uts_printParams(&config);
  uts_initRoot(&config, &root);

  Result r;

  benchmark_taskparts([&] (auto sched) {
    t1 = uts_wctime();
    r = treeSearch(&config, 0, &root);
    t2 = uts_wctime();
  });

  uts_showStats(&config, 1, 0, t2-t1, r.size, r.leaves, r.maxdepth);

  return 0;
}
