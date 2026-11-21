#pragma once

// On-the-fly pipelines in the style of Cilk-P (Lee et al., SPAA 2013), as a
// client of the four operations of DAG calculus.
//
// A pipeline is a loop. Each iteration runs
//
//   first    stage 0, in the loop itself: one iteration at a time, in order;
//   middle   in a vertex of its own, in parallel with other iterations,
//            except where it asks to wait for the previous one (below);
//   last     in the same vertex, once the previous iteration has finished:
//            one iteration at a time, in order.
//
// The middle of an iteration numbers its stages as it goes, with increasing
// numbers of its choice, starting from 1, through the handle that it
// receives:
//
//   s.next(j)   enter stage j                       (pipe_stage of Cilk-P)
//   s.wait(j)   enter stage j once the previous     (pipe_stage_wait)
//               iteration has left all of its
//               stages numbered j or less
//
// So the nodes of the pipeline and the edges between iterations are decided
// while the pipeline runs, and an iteration may skip stages.
//
// At most `throttle` iterations are in progress: before iteration i starts,
// iterations up to i - throttle have finished.
//
// How a wait is implemented. Each iteration publishes the number of the
// stage it is in. An iteration that must wait makes a vertex with an empty
// body, the event, publishes the stage it waits for, and waits for the
// event. The previous iteration, when it enters a later stage, releases the
// event. An iteration that finds the previous one far enough along makes no
// vertex: it pays for a wait only when it does wait.

#include "../microtaskparts/microtaskparts.hpp"

#include <climits>
#include <deque>
#include <memory>

namespace pipeline {

using namespace microtaskparts;

// How far an iteration has come, as its successor sees it.
struct progress {
  static constexpr long nobody = -1;
  static constexpr long finished = LONG_MAX;

  // The stage this iteration is in.
  std::atomic<long> stage{0};
  // The stage that the next iteration waits for this one to leave, or
  // nobody. The two are read and written in the order of Dekker's protocol,
  // so the default, sequentially consistent, ordering matters here.
  std::atomic<long> wanted{nobody};
  // What to release when that stage has been left. Written before wanted
  // is, and read only by whoever takes wanted back to nobody.
  vertex *event = nullptr;

  // This iteration enters stage s, which is past all it entered before.
  void enter(long s) {
    stage.store(s);
    long w = wanted.load();
    // Stages only increase, so wanted never takes the same value twice, and
    // the exchange succeeds only for the wait whose stage we read.
    if ((w != nobody) && (w < s) && wanted.compare_exchange_strong(w, nobody)) {
      release(event);
    }
  }

  // Called by the next iteration: return once this one has left stage j.
  void await_past(long j) {
    if (stage.load() > j) {
      return;
    }
    vertex *e = new_vertex([] {});
    event = e;
    wanted.store(j);
    if ((stage.load() > j) && (wanted.exchange(nobody) != nobody)) {
      // It left stage j in the meantime, and has not seen our request.
      delete e;
      return;
    }
    wait(e);
    delete e;
  }
};

// The handle through which the middle of an iteration names its stages.
struct stages {
  progress *mine;
  progress *previous; // null in the first iteration

  void next(long j) { mine->enter(j); }
  void wait(long j) {
    mine->enter(j);
    if (previous != nullptr) {
      previous->await_past(j);
    }
  }
};

template <typename Item, typename Middle, typename Last>
struct iteration : vertex {
  Item item;
  progress p;
  iteration *previous = nullptr;
  const Middle *middle = nullptr;
  const Last *last = nullptr;

  void run() override {
    stages s{&p, (previous == nullptr) ? nullptr : &previous->p};
    (*middle)(s, item);
    p.enter(progress::finished);
    if (previous != nullptr) {
      microtaskparts::wait(previous);
    }
    (*last)(item);
  }
};

// Run the pipeline, and return the number of its iterations.
//
//   first(i, item) -> bool   stage 0 of iteration i: fill in the item, or
//                            return false to end the loop
//   middle(s, item)
//   last(item)
template <typename Item, typename First, typename Middle, typename Last>
size_t pipe_while(size_t throttle, const First &first, const Middle &middle,
                  const Last &last) {
  using iter = iteration<Item, Middle, Last>;
  if (throttle < 1) {
    throttle = 1;
  }
  // The iterations that may still be used, oldest first.
  std::deque<std::unique_ptr<iter>> window;
  size_t i = 0;
  for (;; i++) {
    if (window.size() >= throttle) {
      microtaskparts::wait(window[window.size() - throttle].get());
      // That iteration has finished, and it waited for the one before it,
      // so nothing uses the iterations before it any longer.
      while (window.size() > throttle) {
        window.pop_front();
      }
    }
    std::unique_ptr<iter> it(new iter);
    if (!first(i, it->item)) {
      break;
    }
    it->previous = window.empty() ? nullptr : window.back().get();
    it->middle = &middle;
    it->last = &last;
    iter *v = it.get();
    window.push_back(std::move(it));
    release(v);
  }
  if (!window.empty()) {
    microtaskparts::wait(window.back().get());
  }
  return i;
}

} // namespace pipeline
