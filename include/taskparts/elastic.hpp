#pragma once

#include <assert.h>
#include <optional>
#include <atomic>
#include <vector>
#include <functional>
#include <algorithm>

#include "scheduler.hpp"
#include "timing.hpp"
#include "perworker.hpp"
#include "atomic.hpp"
#include "hash.hpp"
#if defined(TASKPARTS_POSIX)
#include "posix/semaphore.hpp"
#include "posix/spinlock.hpp"
#elif defined(TASKPARTS_DARWIN)
// later: add semaphore
#include "darwin/spinlock.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/semaphore.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

namespace taskparts {

#if defined(TASKPARTS_USE_SPINNING_SEMAPHORE) || defined(TASKPARTS_DARWIN)
using dflt_semaphore = spinning_counting_semaphore;
#else
using dflt_semaphore = semaphore;
#endif

/*---------------------------------------------------------------------*/
/* Elastic work stealing (driven by surplus) */

template <typename Stats, typename Logging, typename Semaphore=dflt_semaphore,
          size_t max_lg_P=perworker::default_max_nb_workers_lg>
class elastic {
public:

  // for now...
  static constexpr 
  bool override_rand_worker = false;
  
  using gamma_type = struct gamma_struct {
    int32_t surplus = 0;
    int16_t stealing = 0;
    int16_t suspended = 0;
    auto needs_sentinel() -> bool {
      return (surplus >= 1) && (stealing == 0) && (suspended >= 1);
    }
  };

  static constexpr
  char node_locked = 0, node_unlocked = 1;

  using delta_type = struct delta_struct {
    unsigned int lock : 1 = node_unlocked;
    int surplus : 31 = 0;
    int16_t stealing = 0;
    int16_t suspended = 0;
    struct delta_struct operator+(struct delta_struct other) {
      delta_type d = *this;
      d.surplus += other.surplus;
      d.stealing += other.stealing;
      d.suspended += other.suspended;
      return d;
    }
    auto empty() -> bool {
      return (surplus == 0) && (stealing == 0) && (suspended == 0);
    }
    //    auto apply(gamma_type g) -> gamma_type {
    gamma_type apply(gamma_type g) const {
      g.surplus += surplus;
      g.stealing += stealing;
      g.suspended += suspended;
      return g;
    }
  };

  using node_type = struct node_struct {
    int i = -1;                   // index of leaf node (-1 for interior nodes)
    int worker_id = -1;           // id of worker assigned to leaf node (-1 for interior nodes)
    std::atomic<gamma_type> gamma;
    std::atomic<delta_type> delta;
  };

  static
  int alpha, beta;

  static
  cache_aligned_fixed_capacity_array<node_type, 1 << max_lg_P> nodes_array;

  static
  perworker::array<std::vector<node_type*>> paths;

  static
  perworker::array<Semaphore> semaphores;

  static
  perworker::array<uint64_t> rng;

  static
  int lg_P;

  static
  auto next_random_number(size_t my_id = perworker::my_id()) -> size_t {
    auto& nb = rng[my_id];
    nb = (size_t)(hash(my_id) + hash(nb));
    return nb;
  }
  
  static
  auto apply_delta(node_type* n, delta_type d) -> void {
    n->gamma.store(d.apply(n->gamma.load()));
  }

  template <typename T, typename Update>
  static
  auto update(const Update& update, std::atomic<T>& cell) -> T {
    while (true) {
      auto v = cell.load();
      auto orig = v;
      auto next = update(orig);
      if (cell.compare_exchange_strong(orig, next)) {
        return next;
      }
    }
  }

  static
  auto path_size() -> int {
    assert((int)paths.mine().size() == (lg_P + 1));
    return lg_P + 1;
  }

  static
  auto path_index_of_leaf() -> int {
    return path_size() - 1;
  }

  static
  auto root_node(size_t my_id = perworker::my_id()) -> node_type* {
    return paths[my_id][0];
  }

  static
  auto leaf_node(size_t my_id = perworker::my_id()) -> node_type* {
    auto i = path_index_of_leaf();
    return paths[my_id][i];
  }
  
  static
  auto update_path(delta_type delta, std::vector<node_type*>& path, size_t id = perworker::my_id()) -> gamma_type {
    gamma_type result = root_node()->gamma.load();
    auto nb_workers = perworker::nb_workers();
    if (nb_workers == 1) {
      return result;
    }
    auto update_root = [id, &result] (delta_type d) {
      result = update([=] (gamma_type g) { return d.apply(g); }, root_node()->gamma);
    };
    if (nb_workers == 2) {
      update_root(delta);
      return result;
    }
    auto up = [id, update_root] (int i, delta_type d) -> int {
      int j;
      for (j = i; j > 0; j--) {
        auto n = paths[id][j];
        delta_type d_o;
        while (true) {
          d_o = n->delta.load();
          delta_type d_n;
          d_n.lock = node_locked;
          if (d_o.lock == node_unlocked) { // no delegation
            if (n->delta.compare_exchange_strong(d_o, d_n)) {
              apply_delta(n, d);
              break;
            }
          } else { // delegation
            assert(d_o.lock == node_locked);
            d_n = d_o + d;
            if (n->delta.compare_exchange_strong(d_o, d_n)) {
              // delegate the update of d_n to the worker holding the lock on d_o
              assert(j >= 1);
              assert(d_n.lock == node_locked);
              return j;
            }
          }
        }
      }
      assert(j == 0);
      update_root(d);
      return j;
    };
    auto down = [id] (int i) -> std::pair<int, delta_type> {
      int j;
      delta_type d_n;
      for (j = (i + 1); j < path_index_of_leaf(); j++) {
        auto n = paths[id][j];
        delta_type d_o;
        while (true) {
          d_o = n->delta.load();
          assert(d_o.lock == node_locked);
          if (d_o.empty()) { // no delegation to handle
            d_n.lock = node_unlocked;
            if (n->delta.compare_exchange_strong(d_o, d_n)) {
              break;
            }
          } else { // handle delegation
            d_n.lock = node_locked;
            if (n->delta.compare_exchange_strong(d_o, d_n)) {
              apply_delta(n, d_o);
              return std::make_pair(j - 1, d_o);
            }
          }
        }
      }
      assert(d_n.empty());
      assert(j == path_index_of_leaf());
      return std::make_pair(j, d_n);      
    };
    auto i = path_index_of_leaf() - 1;
    while (true) {
      i = up(i, delta);
      auto [_i, _delta] = down(i);
      i = _i;
      delta = _delta;
      if (i == path_index_of_leaf()) {
        break;
      }
    }
    return result;
  }

  static
  auto update_path(delta_type delta, size_t my_id = perworker::my_id()) -> gamma_type {
    auto n = leaf_node(my_id);
    update([=] (gamma_type g) { return delta.apply(g); }, n->gamma);
    auto g = update_path(delta, paths[my_id], my_id);
    while (g.needs_sentinel()) {
      auto id = next_random_number() % perworker::nb_workers();
      if (try_resume(id)) {
        break;
      }
      g = root_node()->gamma.load();
    }
    return g;
  }

  static
  auto incr_surplus(size_t my_id = perworker::my_id()) -> void {
    auto delta = delta_type{.surplus = +1};
    update_path(delta, my_id);
    Stats::increment(Stats::configuration_type::nb_surplus_transitions);
  }

  static
  auto decr_surplus(size_t my_id = perworker::my_id()) -> void {
    auto delta = delta_type{.surplus = -1};
    update_path(delta, my_id);
  }

  static
  auto incr_stealing(size_t my_id = perworker::my_id()) -> void {
    auto delta = delta_type{.stealing = +1};
    update_path(delta, my_id);
  }

  static
  auto decr_stealing(size_t my_id = perworker::my_id()) -> void {
    auto delta = delta_type{.stealing = -1};
    auto n = leaf_node(my_id);
    auto g = update([=] (gamma_type g) { return delta.apply(g); }, n->gamma);
    // scale up
    auto m = alpha;
    while ((m > 0) || g.needs_sentinel()) {
      if (g.suspended == 0) {
        break;
      }
      auto id = next_random_number() % perworker::nb_workers();
      if (try_resume(id)) {
        m--;
      }
      g = root_node()->gamma.load();
    }
  }

  static
  auto incr_suspended(size_t my_id = perworker::my_id()) -> void {
    auto delta = delta_type{.suspended = +1, .stealing = -1};
    update_path(delta, my_id);
  }

  static
  auto decr_suspended(size_t my_id = perworker::my_id()) -> void {
    auto delta = delta_type{.surplus = 0, .suspended = -1, .stealing = +1};
    update_path(delta, my_id);
  }

  static
  auto try_suspend(size_t my_id = perworker::my_id()) -> void {
    auto flip = [=] () -> bool {
      return (next_random_number(my_id) % beta) == 0;
    };
    if (! flip()) {
      return;
    }
    auto delta1 = delta_type{.suspended = +1, .stealing = -1};
    auto n = leaf_node(my_id);
    auto g = n->gamma.load();
    n->gamma.store(delta1.apply(g));
    if (update_path(delta1, paths[my_id], my_id).needs_sentinel()) {
      try_resume(my_id);
    }
    { // start suspending the caller
      Stats::on_exit_acquire();
      Logging::log_enter_sleep(my_id, 0l, 0l);
      Stats::on_enter_sleep();
      semaphores[my_id].wait();
      Stats::on_exit_sleep();
      Logging::log_event(exit_sleep);
      Stats::on_enter_acquire();
    } // end suspending the caller
    auto delta2 = delta_type{.suspended = -1, .stealing = +1};
    update_path(delta2, my_id);
  }

  static
  auto try_resume(size_t worker_id = perworker::my_id()) -> bool {
    auto n = leaf_node(worker_id);
    auto g = n->gamma.load();
    if (g.suspended == 0) {
      return false;
    }
    auto orig = g;
    auto next = orig;
    next.suspended = 0;
    next.stealing = 1;
    if (! n->gamma.compare_exchange_strong(orig, next)) {
      return false;
    }
    semaphores[worker_id].post();
    return true;
  }

  static
  auto nb_leaves() -> int {
    return 1 << lg_P;
  }

  static
  auto nb_nodes(int lg) -> int {
    return (1 << lg) - 1;
  }

  static
  auto nb_nodes() -> int {
    return nb_nodes(lg_P + 1);
  }

  static
  auto tree_index_of_first_leaf() -> int {
    return nb_nodes(lg_P);
  }

  static
  auto is_root(int n) -> bool {
    return n == 0;
  }

  static
  auto parent_of(int n) -> int {
    assert(! is_root(n));
    return (n - 1) / 2;
  }
  
  static
  auto tree_index_of_leaf_node_at(int i) -> int {
    if (lg_P == 0) {
      assert(i == 0);
      return 0;
    }
    assert((i >= 0) && (i < nb_leaves()));
    auto l = tree_index_of_first_leaf();
    auto k = l + i;
    assert(k > 0 && k < nb_nodes());
    return k;
  }

  static
  auto initialize() {
    if (const auto env_p = std::getenv("TASKPARTS_ELASTIC_ALPHA")) {
      alpha = std::stoi(env_p);
    } else {
      alpha = 2;
    }
    if (const auto env_p = std::getenv("TASKPARTS_ELASTIC_BETA")) {
      beta = std::max(2, std::stoi(env_p));
    } else {
      beta = 2;
    }
    // find the nearest setting of lg_P s.t. lg_P is the smallest
    // number for which 2^{lg_P} < perworker::nb_workers()
    lg_P = 0;
    while ((1 << lg_P) < perworker::nb_workers()) {
      lg_P++;
    }
    // initialize tree nodes
    for (int i = 0; i < nb_nodes(); i++) {
      new (&nodes_array[i]) node_type;
      nodes_array[i].i = i;
    }
    { // write ids of workers in their leaf nodes
      auto j = tree_index_of_first_leaf();
      for (auto i = 0; i < nb_leaves(); i++, j++) {
        nodes_array[j].worker_id = i;
      }
    }
    // initialize each array in the paths structure s.t. each such array stores its
    // leaf-to-root path in the tree
    // n: index of a leaf node in the heap array
    auto mk_path = [] (int n) -> std::vector<node_type*> {
      std::vector<node_type*> r;
      if (lg_P == 0) {
        r.push_back(&nodes_array[0]);
        return r;
      }
      do {
        r.push_back(&nodes_array[n]);
        n = parent_of(n);
      } while (! is_root(n));
      r.push_back(&nodes_array[n]);
      std::reverse(r.begin(), r.end());
      return r;
    };
    for (auto i = 0; i < nb_leaves(); i++) {
      auto nd = tree_index_of_leaf_node_at(i);
      paths[i] = mk_path(nd);
      assert(paths[i].size() == path_size());
      paths[i][path_size() - 1]->worker_id = i;
    }
  }
  
};

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
int elastic<Stats, Logging, Semaphore, max_lg_P>::alpha;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
int elastic<Stats, Logging, Semaphore, max_lg_P>::beta;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
cache_aligned_fixed_capacity_array<typename elastic<Stats, Logging, Semaphore, max_lg_P>::node_type, 1 << max_lg_P> elastic<Stats, Logging, Semaphore, max_lg_P>::nodes_array;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
perworker::array<std::vector<typename elastic<Stats, Logging, Semaphore, max_lg_P>::node_type*>> elastic<Stats, Logging, Semaphore, max_lg_P>::paths;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
perworker::array<Semaphore> elastic<Stats, Logging, Semaphore, max_lg_P>::semaphores;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
perworker::array<uint64_t> elastic<Stats, Logging, Semaphore, max_lg_P>::rng;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_P>
int elastic<Stats, Logging, Semaphore, max_lg_P>::lg_P;

/*---------------------------------------------------------------------*/
/* Elastic work stealing (driven by surplus) */

template <typename Stats, typename Logging, typename Semaphore=dflt_semaphore>
class elastic_surplus {
public:
  
  using counters_type = struct counters_struct {
    int32_t surplus;
    int16_t sleeping;
    int16_t stealing;
    counters_struct() : surplus(0), sleeping(0), stealing(0) { }
  };
  
  static
  perworker::array<std::atomic<counters_type>> worker_counters;

  static
  std::atomic<counters_type> global_counters;
  
  static
  perworker::array<Semaphore> semaphores;

  static
  perworker::array<uint64_t> rng;

  static
  int alpha;

  static
  int beta;
  
  template <typename Update>
  static
  auto update_counters(std::atomic<counters_type>& c, const Update& u) -> counters_type {
    auto valid_counters = [] (counters_type c) -> bool {
      auto nb_workers = perworker::nb_workers();
      auto valid_nb_workers = [&] (int n) {
        return (n >= 0) && (n < nb_workers);
      };
      return valid_nb_workers((int)c.sleeping) && valid_nb_workers((int)c.stealing);
    };
#ifndef TASKPARTS_ELASTIC_OVERRIDE_ADAPTIVE_BACKOFF
    while (true) {
      auto v = c.load();
      auto orig = v;
      auto next = u(orig);
      assert(valid_counters(orig));
      assert(valid_counters(next));
      if (c.compare_exchange_strong(orig, next)) {
        return next;
      }
    }
#else
    // We mitigate contention on the counters cell by using Adaptive
    // Feedback (as proposed in Ben-David's dissertation)
    uint64_t max_delay = 1;
    while (true) {
      auto v = c.load();
      auto orig = v;
      auto next = u(orig);
      if (c.compare_exchange_strong(orig, next)) {
        return next;
      }
      max_delay *= 2;
      cycles::spin_for(hash(my_id + max_delay) % max_delay);
    }
#endif
  }

  template <typename Update>
  static
  auto update_counters(size_t id, const Update& u) -> counters_type {
    return update_counters(worker_counters[id], u);
  }

  static
  auto try_to_wake(size_t id) -> bool {
    if (! decr_sleeping_of_worker(id)) {
      return false;
    }
    semaphores[id].post();
    return true;
  }
  
  template <typename Update>
  static
  auto update_global_counters(const Update& u, bool was_trying_to_sleep = false) -> counters_type {
    auto my_id = perworker::my_id();
    auto nb_workers = perworker::nb_workers();
    auto need_sentinel = [nb_workers] (counters_type s) {
      return (s.sleeping >= 1) && (s.surplus >= 1) && (s.stealing == 0);
    };
    auto s = update_counters(global_counters, u);
    while (need_sentinel(s)) {
      if (was_trying_to_sleep) {
        try_to_wake(my_id);
        break;
      }
      auto target_id = next_rng() % nb_workers;
      if (try_to_wake(target_id)) {
        break;
      }
      s = global_counters.load();
    }
    return s;
  }
  
  static
  auto incr_surplus(size_t my_id = perworker::my_id()) {
    update_counters(my_id, [&] (counters_type s) {
      assert(s.stealing == 0);
      assert(s.sleeping == 0);
      s.surplus++;
      return s;
    });
    auto s = update_global_counters([&] (counters_type s) {
      s.surplus++;
      return s;
    });
    Stats::increment(Stats::configuration_type::nb_surplus_transitions);
  }
  
  static
  auto decr_surplus(size_t id) {
    update_counters(id, [&] (counters_type s) {
      assert((id != perworker::my_id()) || (s.sleeping == 0));
      assert((id != perworker::my_id()) || (s.stealing == 0));
      s.surplus--;
      return s;
    });
    auto s = update_global_counters([&] (counters_type s) {
      s.surplus--;
      return s;
    });
  }
  
  static
  auto incr_stealing(size_t my_id = perworker::my_id()) {
    update_counters(my_id, [&] (counters_type s) {
      assert(s.stealing == 0);
      assert(s.sleeping == 0);
      s.stealing++;
      return s;
    });
    auto s = update_global_counters([&] (counters_type s) {
      s.stealing++;
      return s;
    });
  }
  
  static
  auto decr_stealing(size_t my_id = perworker::my_id()) {
    update_counters(my_id, [&] (counters_type s) {
      assert(s.stealing == 1);
      assert(s.sleeping == 0);
      s.stealing--;
      return s;
    });
    auto s = update_global_counters([&] (counters_type s) {
      s.stealing--;
      return s;
    });
    auto random_other_worker = [&] (size_t my_id) -> size_t {
      auto nb_workers = perworker::nb_workers();
      size_t id = next_rng(my_id) % (nb_workers - 1);
      if (id >= my_id) {
        id++;
      }
      return id;
    };
    int nb_to_wake = alpha;
    while (nb_to_wake > 0) {
      if (global_counters.load().sleeping == 0) {
        return;
      }
      if (try_to_wake(random_other_worker(my_id))) {
        nb_to_wake--;
      }
    }
  }
  
  static
  auto incr_sleeping(size_t my_id = perworker::my_id()) {
    update_counters(my_id, [&] (counters_type s) {
      assert(s.stealing == 1);
      assert(s.sleeping == 0);
      s.sleeping++;
      s.stealing--;
      return s;
    });
    auto s = update_global_counters([&] (counters_type s) {
      s.sleeping++;
      s.stealing--;
      return s;
    }, true);
  }
  
  static
  auto decr_sleeping_global() {
    auto s = update_global_counters([&] (counters_type s) {
      s.sleeping--;
      s.stealing++;
      return s;
    });
  }
  
  static
  auto decr_sleeping_of_worker(size_t id) -> bool {
    auto& cs = worker_counters[id];
    while (true) {
      auto s = cs.load();
      if (s.sleeping == 0) {
        return false;
      }
      assert(s.sleeping == 1);
      assert(s.stealing == 0);
      auto orig = s;
      auto next = s;
      next.sleeping--;
      next.stealing++;
      if (cs.compare_exchange_strong(orig, next)) {
        return true;
      }
    }
  }

  static
  auto decr_sleeping(size_t my_id = perworker::my_id()) {
    auto& cs = worker_counters[my_id];
    auto s = cs.load();
    while (s.sleeping != 0) {
      decr_sleeping_of_worker(my_id);
      s = cs.load();
    }
    assert(s.sleeping == 0);
    assert(s.stealing == 1);
    decr_sleeping_global();
  }

  static
  auto next_rng(size_t my_id = perworker::my_id()) -> size_t {
    auto& nb = rng[my_id];
    nb = (size_t)(hash(my_id) + hash(nb));
    return nb;
  }

  static
  auto try_suspend(size_t) {
    auto my_id = perworker::my_id();
    auto flip = [&] () -> bool {
      auto n = next_rng(my_id);
      // LATER: replace the coin flip below b/c currently the
      // calculation inconsistent with the defintion of beta given in
      // the paper
      return (n % beta) == 0;
      //return (beta == 1) ? true : (n % beta) < (beta - 1);
    };
    if (! flip()) {
      return;
    }
    Stats::on_exit_acquire();
    Logging::log_enter_sleep(my_id, 0l, 0l);
    Stats::on_enter_sleep();
    incr_sleeping(my_id);
    semaphores[my_id].wait();
    decr_sleeping(my_id);
    Stats::on_exit_sleep();
    Logging::log_event(exit_sleep);
    Stats::on_enter_acquire();
  }

  static
  auto initialize() {
    if (const auto env_p = std::getenv("TASKPARTS_ELASTIC_ALPHA")) {
      alpha = std::stoi(env_p);
    } else {
      alpha = 2;
    }
    if (const auto env_p = std::getenv("TASKPARTS_ELASTIC_BETA")) {
      beta = std::max(2, std::stoi(env_p));
    } else {
      beta = 2;
    }
  }
  
  static constexpr
  bool override_rand_worker = false;
  
};

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<std::atomic<typename elastic_surplus<Stats, Logging, Semaphore>::counters_type>> elastic_surplus<Stats, Logging, Semaphore>::worker_counters;

template <typename Stats, typename Logging, typename Semaphore>
std::atomic<typename elastic_surplus<Stats, Logging, Semaphore>::counters_type> elastic_surplus<Stats, Logging, Semaphore>::global_counters;

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<Semaphore> elastic_surplus<Stats, Logging, Semaphore>::semaphores;

template <typename Stats, typename Logging, typename Semaphore>
int elastic_surplus<Stats, Logging, Semaphore>::alpha;

template <typename Stats, typename Logging, typename Semaphore>
int elastic_surplus<Stats, Logging, Semaphore>::beta;

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<uint64_t> elastic_surplus<Stats, Logging, Semaphore>::rng;  

} // end namespace
