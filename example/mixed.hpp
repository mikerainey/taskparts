#pragma once

#include <vector>
#include <signal.h>
#include <taskparts/fiber.hpp>
#include <taskparts/hash.hpp>

size_t nb_par_iters;
size_t nb_iterations = 0;

using hash_value_type = uint64_t;

template <typename Scheduler=taskparts::minimal_scheduler<>>
class alternate_par : public taskparts::fiber<Scheduler> {
public:
  using trampoline_type = enum trampoline_enum { entry, work };
  trampoline_type trampoline = entry;
  size_t lo, hi, k;
  hash_value_type hv;
  hash_value_type* hvp;
  taskparts::fiber<Scheduler>* tk;
  alternate_par(size_t lo, size_t hi, hash_value_type* hvp
		, taskparts::fiber<Scheduler>* tk, Scheduler sched=Scheduler())
    : taskparts::fiber<Scheduler>(), lo(lo), hi(hi), hvp(hvp), tk(tk) { }
  auto run() -> taskparts::fiber_status_type {
    switch (trampoline) {
      case entry: {
        assert(lo <= hi);
        if (lo == hi) {
          return taskparts::fiber_status_finish;
        }
        k = lo++;
        trampoline = work;
        if (lo == hi) {
          return taskparts::fiber_status_continue;
        }
        auto mid = (lo + hi) / 2;
        auto f1 = new alternate_par(lo, mid, &hvp[lo], tk);
        auto f2 = new alternate_par(mid, hi, &hvp[mid], tk);
        taskparts::fiber<Scheduler>::add_edge(f1, tk);
        taskparts::fiber<Scheduler>::add_edge(f2, tk);
        f1->release();
        f2->release();
        return taskparts::fiber_status_continue;
      }
      case work: {
        auto h = hv;
        for (auto i = 0; i < nb_par_iters; i++) {
          h = taskparts::hash(h);
        }
        hv = h;
	hvp[k] = hv;
        break;
      }
    }
    return taskparts::fiber_status_finish;
  }
};

size_t nb_seq_iters;
size_t nb_par_paths;
volatile
bool keep_going = true;

void catcher(int signum) {
  keep_going = false;
}

template <typename Scheduler=taskparts::minimal_scheduler<>>
class alternate : public taskparts::fiber<Scheduler> {
public:
  using trampoline_type = enum trampoline_enum { entry };
  trampoline_type trampoline = entry;
  hash_value_type hv = 0;
  std::vector<hash_value_type> hvs;
  alternate(Scheduler sched=Scheduler())
    : taskparts::fiber<Scheduler>() {
    keep_going = true;
    hvs.resize(nb_par_paths);
    struct sigaction sact;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;
    sigaction(SIGALRM, &sact, nullptr);
    alarm(1);
  }
  auto run() -> taskparts::fiber_status_type {
    auto h = hv;
    for (auto i = 0; i < nb_seq_iters; i++) {
      h = taskparts::hash(h);
    }
    hv = h;
    nb_iterations++;
    if (! keep_going) {
      return taskparts::fiber_status_finish;
    }
    auto f = new alternate_par(0, nb_par_paths, &hvs[0], this, Scheduler());
    taskparts::fiber<Scheduler>::add_edge(f, this);
    f->release();
    return taskparts::fiber_status_pause;
  }
};
