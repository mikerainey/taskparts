#pragma once

#include <assert.h>
#include <optional>
#include <atomic>
#include <vector>
#include <functional>

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
    //aprintf("incr_sp stealing=%d sleeping=%d surplus=%d\n",s.stealing,s.sleeping,s.surplus);
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
    //aprintf("decr_sp stealing=%d sleeping=%d surplus=%d\n",s.stealing,s.sleeping,s.surplus);
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
    //aprintf("incr_st stealing=%d sleeping=%d surplus=%d\n",s.stealing,s.sleeping,s.surplus);
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
    //aprintf("decr_st stealing=%d sleeping=%d surplus=%d\n",s.stealing,s.sleeping,s.surplus);
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
    //aprintf("incr_sl stealing=%d sleeping=%d surplus=%d\n",s.stealing,s.sleeping,s.surplus);
  }
  
  static
  auto decr_sleeping_global() {
    auto s = update_global_counters([&] (counters_type s) {
      s.sleeping--;
      s.stealing++;
      return s;
    });
    //aprintf("decr_sl_gl stealing=%d sleeping=%d surplus=%d\n",s.stealing,s.sleeping,s.surplus);
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
  auto try_to_sleep(size_t) {
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
  
/*---------------------------------------------------------------------*/
/* Elastic work stealing (driven by concurrent tree, based on tracking surplus) */

template <typename Stats, typename Logging, typename Semaphore=dflt_semaphore,
          size_t max_lg_P=perworker::default_max_nb_workers_lg>
class elastic_tree {
public:
  
  static constexpr
  char node_locked = 1;
  static constexpr
  char node_unlocked = 0;
  
  class gamma {
  public:
    int a; // asleep
    int s; // surplus
    gamma() noexcept : a(0), s(0) { }
  };
  
  class delta {
  public:
    unsigned int l : 1;  // lock bit (node_locked/node_unlocked)
    int a : 31;          // asleep
    int s;               // surplus
    delta() noexcept : l(node_unlocked), a(0), s(0) { }
  };

  using node = struct node_struct {
    int i = -1;
    int id = -1; // worker id
    std::atomic<gamma> g;
    std::atomic<delta> d;
  };
  
  static
  int lg_P;
  
  static
  int nb_to_wake_on_surplus_increase;

  static
  cache_aligned_fixed_capacity_array<node, 1 << max_lg_P> nodes_array;
  
  static
  perworker::array<std::vector<node*>> paths;
  
  static
  perworker::array<Semaphore> semaphores;

  static
  perworker::array<std::atomic<int64_t>> epochs;
  
  static
  perworker::array<size_t> random_number_generator;
  
  static
  auto is_delta_empty(delta d) -> bool {
    return ((d.s == 0) && (d.a == 0));
  }
  
  static
  auto combine_deltas(delta d_o, delta d_n) -> delta {
    d_o.a += d_n.a;
    d_o.s += d_n.s;
    return d_o;
  }
  
  static
  auto apply_delta(gamma g, delta d) -> gamma {
    auto a = g.a + d.a;
    auto s = g.s + d.s;
    gamma g2;
    g2.a = a;
    g2.s = s;
    assert(g2.s >= 0);
    assert(g2.s <= perworker::nb_workers());
    assert(g2.a >= 0);
    assert(g2.a <= perworker::nb_workers());
    return g2;
  }

  static
  auto apply_changes(node* n, delta d) {
    n->g.store(apply_delta(n->g.load(), d));
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
  auto is_root(int n) -> bool {
    return n == 0;
  }
  
  static
  auto parent_of(int n) -> int {
    assert(! is_root(n));
    return (n - 1) / 2;
  }
  
  static
  auto tree_index_of_first_leaf() -> int {
    return nb_nodes(lg_P);
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
  auto leaf_node_at(int i) -> node* {
    return &nodes_array[tree_index_of_leaf_node_at(i)];
  }
  
  static
  auto is_leaf_node(int i) -> bool {
    return i >= tree_index_of_first_leaf();
  }
  
  static
  auto path_size() -> int {
    assert((int)paths.mine().size() == (lg_P + 1));
    return lg_P + 1;
  }
  
  using update_gamma_result_type = enum update_gamma_result_enum {
    update_gamma_aborted,
    update_gamma_succeeded,
    update_gamma_failed
  };
  
  template <typename Should_exit, typename Update>
  static
  auto try_to_update_gamma(std::atomic<gamma>& g,
                           const Should_exit& should_exit,
                           const Update& update) -> update_gamma_result_type {
    auto v = g.load();
    if (should_exit(v)) {
      return update_gamma_aborted;
    }
    auto orig = v;
    auto next = update(orig);
    auto b = g.compare_exchange_strong(orig, next);
    return b ? update_gamma_succeeded : update_gamma_failed;
  }
  
  template <typename Should_exit, typename Update>
  static
  auto update_gamma_or_abort(std::atomic<gamma>& g,
                             const Should_exit& should_exit,
                             const Update& update) -> update_gamma_result_type {
    auto r = try_to_update_gamma(g, should_exit, update);
    while (r == update_gamma_failed) {
      r = try_to_update_gamma(g, should_exit, update);
    }
    return r;
  }
  
  template <typename Should_exit, typename Update>
  static
  auto update_gamma(std::atomic<gamma>& g,
                    const Should_exit& should_exit,
                    const Update& update) -> void {
    auto r = try_to_update_gamma(g, should_exit, update);
    while (r != update_gamma_succeeded) {
      r = try_to_update_gamma(g, should_exit, update);
    }
  }
  
  template <typename Update>
  static
  auto update_gamma(std::atomic<gamma>& g,
                    const Update& update) -> void {
    return update_gamma(g, [] (gamma) { return false; }, update);
  }

  static
  auto update_path_to_root(size_t id, delta d) {
    auto update_root = [id] (delta d) {
      update_gamma(paths[id][0]->g, [=] (gamma g) {
        return apply_delta(g, d);
      });
    };
    auto up = [id, update_root] (int i, delta d) -> int {
      int j;
      for (j = i; j > 0; j--) {
        auto n = paths[id][j];
        delta d_o;
        while (true) {
          d_o = n->d.load();
          delta d_n;
          d_n.l = node_locked;
          if (d_o.l == node_unlocked) { // no delegation
            if (n->d.compare_exchange_strong(d_o, d_n)) {
              apply_changes(n, d);
              break;
            }
          } else { // delegation
            assert(d_o.l == node_locked);
            d_n = combine_deltas(d_o, d);
            if (n->d.compare_exchange_strong(d_o, d_n)) {
              // delegate the update of d_n to the worker holding the lock on d_o
              assert(j >= 1);
              assert(d_n.l == node_locked);
              //aprintf("up/delegation n=%d j=%d d_o=(%d %d) d_n=(%d %d) d=(%d %d) \n",n->i,j,
              //        d_o.a,d_o.s,d_n.a,d_n.s,d.a,d.s);
              return j;
            }
          }
        }
      }
      assert(j == 0);
      update_root(d);
      return j;
    };
    auto down = [id] (int i) -> std::pair<int, delta> {
      int j;
      delta d_n;
      for (j = (i + 1); j < path_index_of_leaf(); j++) {
        auto n = paths[id][j];
        delta d_o;
        while (true) {
          d_o = n->d.load();
          assert(d_o.l == node_locked);
          if (is_delta_empty(d_o)) { // no delegation to handle
            d_n.l = node_unlocked;
            if (n->d.compare_exchange_strong(d_o, d_n)) {
              //aprintf("down/nodelegation n=%d j=%d d_o=(%d %d) d_n=(%d %d)\n",n->i,j,
              //        d_o.a,d_o.s,d_n.a,d_n.s);
              break;
            }
          } else { // handle delegation
            d_n.l = node_locked;
            if (n->d.compare_exchange_strong(d_o, d_n)) {
              //aprintf("down/delegation n=%d j=%d d_o=(%d %d) d_n=(%d %d)\n",n->i,j,
              //        d_o.a,d_o.s,d_n.a,d_n.s);
              apply_changes(n, d_o);
              return std::make_pair(j - 1, d_o);
            }
          }
        }
      }
      assert(is_delta_empty(d_n));
      assert(j == path_index_of_leaf());
      return std::make_pair(j, d_n);
    };
    if (perworker::nb_workers() == 1) {
      return;
    }
    if (perworker::nb_workers() == 2) {
      update_root(d);
      return;
    }
    //print_tree("before");
    int i = path_index_of_leaf() - 1;
    assert(0 < i);
    assert((i + 1) < path_size());
    while (true) {
      i = up(i, d);
      auto [_i, _d] = down(i);
      i = _i;
      d = _d;
      if (i == path_index_of_leaf()) {
        assert(is_delta_empty(d));
        break;
      }
      assert(! is_delta_empty(d));
      assert(i < path_index_of_leaf());
    }
    assert(i == path_index_of_leaf());
    //print_tree("after");
  }

  static
  auto wake_children() { }

  static
  auto on_enter_acquire() {
    after_surplus_decrease();
  }
  
  static
  auto print_tree(const char* s0) {
    std::string s;
    for (size_t i = 0; i < nb_nodes(); i++) {
      auto g = nodes_array[i].g.load();
      auto d = nodes_array[i].d.load();
      s += "[n=" + std::to_string(i) + " id=" + std::to_string(nodes_array[i].id)
      + " g=(a=" + std::to_string(g.a) + " s=" + std::to_string(g.s) + ")"
      + " d=(a=" + std::to_string(d.a) + " s=" + std::to_string(d.s) + ")]\n";
    }
    aprintf("%s\n%s\n", s0, s.c_str());
  }

  static
  auto initialize() {
    if (const auto env_p = std::getenv("TASKPARTS_ELASTIC_ALPHA")) {
      nb_to_wake_on_surplus_increase = std::stoi(env_p);
    } else {
      nb_to_wake_on_surplus_increase = 2;
    }
    // find the nearest setting of lg_P s.t. lg_P is the smallest
    // number for which 2^{lg_P} < perworker::nb_workers()
    lg_P = 0;
    while ((1 << lg_P) < perworker::nb_workers()) {
      lg_P++;
    }
    // initialize tree nodes
    for (int i = 0; i < nb_nodes(); i++) {
      new (&nodes_array[i]) node;
      nodes_array[i].i = i;
    }
    { // write ids of workers in their leaf nodes
      auto j = tree_index_of_first_leaf();
      for (auto i = 0; i < nb_leaves(); i++, j++) {
        nodes_array[j].id = i;
      }
    }
    for (auto i = 0; i < perworker::nb_workers(); i++) {
      epochs[i].store(0);
    }
    // initialize each array in the paths structure s.t. each such array stores its
    // leaf-to-root path in the tree
    // n: index of a leaf node in the heap array
    auto mk_path = [] (int n) -> std::vector<node*> {
      std::vector<node*> r;
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
      paths[i][path_size() - 1]->id = i;
    }
  }
  
  static
  auto before_surplus_increase() -> int64_t {
    auto my_id = perworker::my_id();
    // increment the epoch of the calling worker
    auto& my_epoch = epochs[my_id];
    auto e = my_epoch.load() + 1;
    my_epoch.store(e);
    auto i = path_index_of_leaf();
    auto n = paths[my_id][i];
    //aprintf("s++ t=%d n=%d\n", my_id,n->i);
    // update the leaf node (of the calling worker)
    auto r = update_gamma_or_abort(n->g, [&] (gamma g) {
      assert(g.a == 0);
      return g.s == 1; // abort if the surplus count is already positive
    }, [&] (gamma g) {
      g.s = 1;
      return g;
    });
    if (r == update_gamma_aborted) {
      // surplus was already positive => skip updating the tree
      //aprintf("s++/abort n=%d\n",n->i);
      return e;
    }
    assert(r == update_gamma_succeeded);
    // update the interior nodes
    delta d;
    d.s = +1;
    update_path_to_root(my_id, d);
    return e;
  }
  
  static
  auto after_surplus_increase() -> void {
    try_to_wake_others();
  }
  
  static
  auto after_surplus_decrease(size_t target_id = perworker::my_id()) {
    auto i = path_index_of_leaf();
    auto n = paths[target_id][i];
    //aprintf("s-- t=%d n=%d\n",target_id,n->i);
    // update the leaf node
    auto r = update_gamma_or_abort(n->g, [&] (gamma g) {
      return g.s == 0;
    }, [] (gamma g) {
      g.s = 0;
      return g;
    });
    if (r == update_gamma_aborted) {
      //aprintf("s--/abort t=%d n=%d\n",target_id,n->i);
      return;
    }
    assert(r == update_gamma_succeeded);
    // send the update up the tree
    delta d;
    d.s = -1;
    update_path_to_root(target_id, d);
  }

  static
  auto try_to_wake_others(size_t my_id = perworker::my_id()) -> void {
    auto try_to_wake = [&] (size_t target_id) {
      assert(target_id != my_id);
      auto i = path_index_of_leaf();
      auto n = paths[target_id][i];
      //aprintf("a-- t=%d n=%d\n",target_id,n->i);
      auto r = update_gamma_or_abort(n->g, [&] (gamma g) {
        if (g.a == 1) { assert(g.s == 0); }
        //return (g.a == 0) || (g.s > 0);
        return g.a == 0;
      }, [&] (gamma g) {
        g.a = 0;
        assert(g.s == 0);
        return g;
      });
      if (r != update_gamma_succeeded) {
        //aprintf("a--/failed t=%d n=%d\n",target_id,n->i);
        return false;
      }
      semaphores[target_id].post();
      return true;
    };
    int nb_to_wake = nb_to_wake_on_surplus_increase;
    while (nb_to_wake > 0) {
      // exit early if no workers need to wake up
      auto g = nodes_array[0].g.load();
      if (! ((g.a > 0) && (g.s > 0))) {
        break;
      }
      // try to wake up an worker that is asleep
      auto i = sample_by_sleeping(my_id);
      if ((i == not_found) || (i == my_id)) {
        // failed to wake up
        continue;
      }
      if (try_to_wake(i)) {
        nb_to_wake--;
      }
    }
  }
  
  static
  auto surplus_exists() -> bool {
    return nodes_array[0].g.load().s > 0;
  }
  
  static
  auto path_index_of_leaf() -> int {
    return path_size() - 1;
  }
  
  static
  auto try_to_sleep(size_t) {
    auto my_id = perworker::my_id();
    auto i = path_index_of_leaf();
    auto n = paths[my_id][i];
    //aprintf("a++ n=%d\n",n->i);
    auto r = update_gamma_or_abort(n->g, [&] (gamma) {
      return surplus_exists();
    }, [&] (gamma g) {
      g.a = 1;
      return g;
    });
    if (r != update_gamma_succeeded) {
      //aprintf("a++/failed1 n=%d\n", n->i);
      return;
    }
    {
      delta d;
      d.a = +1;
      update_path_to_root(my_id, d);
    }
    if (surplus_exists()) {
      //aprintf("a++/failed2 n=%d\n",n->i);
      semaphores[my_id].post();
    }
    Stats::on_exit_acquire();
    Logging::log_enter_sleep(my_id, 0l, 0l);
    Stats::on_enter_sleep();
    semaphores[my_id].wait();
    Stats::on_exit_sleep();
    Logging::log_event(exit_sleep);
    Stats::on_enter_acquire();
    //aprintf("a-- n=%d\n",n->i);
    update_gamma_or_abort(n->g, [&] (gamma g) {
      // needed b/c the wait() above may return as a consequence
      // of either (1) another worker successfully woke up the calling
      // worker or (2) the semaphore had a counter > 1 (possible anymore?)
      return g.a == 0;
    }, [&] (gamma g) {
      g.a = 0;
      return g;
    });
    // send the update up the tree
    delta d;
    d.a = -1;
    update_path_to_root(my_id, d);
  }
  
  static constexpr
  int first_child = 1;
  static constexpr
  int second_child = 2;
  
  static constexpr
  int not_found = -1;
  
  static
  auto child_of(int n, int d) -> int {
    assert((d == first_child) || (d == second_child));
    assert(n < nb_nodes());
    auto c = n * 2 + d;
    assert(c < nb_nodes());
    return c;
  }
  
  template <typename F>
  static
  auto sample_path(size_t my_id, const F& f) -> int {
    auto random_number = [=] (size_t hi) -> size_t {
      assert(hi > 0);
      auto& rng = random_number_generator[my_id];
      auto n = hash(my_id) + hash(rng);
      rng = n;
      return n % hi;
    };
    auto flip = [=] (int w1, int w2) -> int {
      auto w = w1 + w2;
      auto n = (int)random_number(w);
      return (n < w1) ? first_child : second_child;
    };
    auto sample_node_at = [&] (int i) -> int {
      return f(nodes_array[i].g.load());
    };
    int i, d;
    for (i = 0; ! is_leaf_node(i); i = child_of(i, d)) {
      auto w1 = sample_node_at(child_of(i, first_child));
      auto w2 = sample_node_at(child_of(i, second_child));
      if ((w1 + w2) < 1) {
        return not_found;
      }
      d = flip(w1, w2);
    }
    if (sample_node_at(i) <= 0) {
      return not_found;
    }
    return nodes_array[i].id;
  }
  
  // returns either:
  // (1) the id of some worker that the tree sees as asleep
  // (2) -1 if there are no workers that the tree sees as asleep
  static
  auto sample_by_sleeping(size_t my_id = perworker::my_id()) -> int {
    return sample_path(my_id, [&] (gamma g) {
      return g.a;
    });
  }
  
#if defined(TASKPARTS_ELASTIC_TREE_VICTIM_SELECTION_BY_TREE)
  
  static constexpr
  bool override_rand_worker = true;
  
  static
  auto sample_by_surplus(size_t my_id = perworker::my_id()) -> int {
    return sample_path(my_id, [&] (gamma g) {
      return g.s;
    });
  }
  
  static
  auto random_other_worker(size_t& nb_sa, size_t nb_workers, size_t my_id) -> size_t {
    assert(nb_workers != 1);
    auto id_maybe = sample_by_surplus(my_id);
    if (id_maybe == not_found) {
      auto id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
      if (id >= my_id) {
        id++;
      }
      nb_sa++;
      return id;
    }
    return (size_t)id_maybe;
  }
#else
  
  static constexpr
  bool override_rand_worker = false;
  
#endif
  
};
  
template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
int elastic_tree<Stats, Logging, Semaphore, max_lg_P>::lg_P;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
int elastic_tree<Stats, Logging, Semaphore, max_lg_P>::nb_to_wake_on_surplus_increase;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
cache_aligned_fixed_capacity_array<typename elastic_tree<Stats, Logging, Semaphore, max_lg_P>::node, 1 << max_lg_P> elastic_tree<Stats, Logging, Semaphore, max_lg_P>::nodes_array;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
perworker::array<std::vector<typename elastic_tree<Stats, Logging, Semaphore, max_lg_P>::node*>> elastic_tree<Stats, Logging, Semaphore, max_lg_P>::paths;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
perworker::array<Semaphore> elastic_tree<Stats, Logging, Semaphore, max_lg_P>::semaphores;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
perworker::array<std::atomic<int64_t>> elastic_tree<Stats, Logging, Semaphore, max_lg_P>::epochs;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
perworker::array<size_t> elastic_tree<Stats, Logging, Semaphore, max_lg_P>::random_number_generator;

} // end namespace
