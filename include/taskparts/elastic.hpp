#pragma once

#include <assert.h>
#include <optional>
#include <atomic>
#include <vector>
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
          size_t max_lg_tree_sz=perworker::default_max_nb_workers_lg>
class elastic {
public:
  
  using cdata_type = struct cdata_struct {
    int32_t surplus = 0;
    int16_t suspended = 0;
    int16_t stealers = 0;
    auto needs_sentinel() -> bool {
      return (surplus >= 1) && (stealers == 0) && (suspended >= 1);
    }
  };

  static constexpr
  char node_locked = 1, node_unlocked = 0;

  using cdelta_type = struct cdelta_struct {
    unsigned int locked : 1;
    int surplus : 31;
    int16_t stealers;
    int16_t suspended;
    struct cdelta_struct operator+(struct cdelta_struct other) {
      cdelta_type d = *this;
      d.surplus += other.surplus;
      d.stealers += other.stealers;
      d.suspended += other.suspended;
      return d;
    }
    auto is_inert() -> bool {
      return (surplus == 0) && (stealers == 0) && (suspended == 0);
    }
    cdata_type apply(cdata_type c) const {
      c.surplus += surplus;
      c.stealers += stealers;
      c.suspended += suspended;
      return c;
    }
  };

  using cnode_type = struct cnode_struct {
    std::atomic<cdata_type> counter;
    std::atomic<cdelta_type> delta;
    auto update_counter(cdelta_type d) {
      if (d.is_inert()) {
        return;
      }
      while (true) {
        auto orig = counter.load();
        auto next = orig;
        next.suspended += d.suspended;
        next.stealers += d.stealers;
        next.surplus += d.surplus;
        if (counter.compare_exchange_strong(orig, next)) {
          //aprintf("node updated ni=%p ste=%d sus=%d sur=%d\n", this,next.stealers,next.suspended,next.surplus);
          break;
        }
      }
    }
  };

  static
  int alpha, beta;
  
  static
  size_t tree_height;

  static
  cnode_type* tree;

  static
  perworker::array<std::vector<cnode_type*>> paths;

  static
  perworker::array<Semaphore> semaphores;

  static
  perworker::array<std::atomic_bool> flags;
  
  static
  cnode_type* nr; // root node
  
  static
  auto needs_sentinel() -> bool {
    return nr->counter.load().needs_sentinel();
  }
  
  static
  auto ensure_sentinel() -> void {
    while (needs_sentinel()) {
      if (try_resume(random_suspended_worker())) {
        break;
      }
    }
  }
  
  using node_update_type = enum node_update_enum {
    node_update_finalized, node_update_delegated
  };
  
  static
  auto try_lock_and_update_node(cnode_type* ni, cdelta_type d)
  -> node_update_type {
    while (true) {
      auto orig = ni->delta.load();
      auto next = orig;
      if (orig.locked == node_locked) {
        next.suspended += d.suspended;
        next.stealers += d.stealers;
        next.surplus += d.surplus;
        if (ni->delta.compare_exchange_strong(orig, next)) {
          return node_update_delegated; // delegated d to the worker holding the lock
        }
      } else {
        next.locked = node_locked;
        if (ni->delta.compare_exchange_strong(orig, next)) {
          ni->update_counter(d);
          return node_update_finalized; // updated ni wrt d (no delegation)
        }
      }
    }
  }
  
  static
  auto try_unlock_node(cnode_type* ni)
  -> std::pair<node_update_type, cdelta_type> {
    while (true) {
      auto orig = ni->delta.load();
      auto next = cdelta_type{};
      if (orig.is_inert()) { // nothing delegated to the caller
        assert(next.locked == node_unlocked);
        if (ni->delta.compare_exchange_strong(orig, next)) {
          return std::make_pair(node_update_finalized, orig);
        }
      } else { // delta in orig delegated to the caller
        assert(orig.locked == node_locked);
        if (! ni->delta.compare_exchange_strong(orig, next)) {
          continue;
        }
        ni->update_counter(orig);
        return std::make_pair(node_update_delegated, orig);
      }
    }
  }
  
  static
  auto update_tree(cdelta_type d,
                   size_t id = perworker::my_id(),
                   size_t h = tree_height) -> void {
    auto i = h + 1;
    while (true) {
      while (true) {
        // lock along path until seeing the root node or a delegation
        i--;
        //aprintf("update1 ni=%p i=%d h=%d ste=%d sus=%d sur=%d\n", paths[id][i],i,h,d.stealers,d.suspended,d.surplus);
        if (i == 0) {
          nr->update_counter(d);
          break;
        }
        if (try_lock_and_update_node(paths[id][i], d) ==
            node_update_delegated) {
          //aprintf("delegate i=%d h=%d ste=%d sus=%d sur=%d\n",i,h,d.stealers,d.suspended,d.surplus);
          break;
        }
      }
      while (true) {
        // unlock along path until seeing a leaf or a delegation
        i++;
        if (i > h) {
          //aprintf("update exit i=%d\n",i);
          return;
        }
        auto [delegated, _d] = try_unlock_node(paths[id][i]);
        if (delegated == node_update_delegated) {
          d = _d;
          //aprintf("delegated1 i=%d ste=%d sus=%d sur=%d\n",i,d.stealers,d.suspended,d.surplus);
          break;
        }
        //aprintf("update4 i=%d ste=%d sus=%d sur=%d\n",i,d.stealers,d.suspended,d.surplus);
      }
    }
  }

  static
  auto incr_surplus(size_t my_id = perworker::my_id()) -> void {
    update_tree(cdelta_type{.surplus = +1}, my_id);
    Stats::increment(Stats::configuration_type::nb_surplus_transitions);
    ensure_sentinel();
  }

  static
  auto decr_surplus(size_t my_id = perworker::my_id()) -> void {
    update_tree(cdelta_type{.surplus = -1}, my_id);
    ensure_sentinel();
  }

  static
  auto incr_stealing(size_t my_id = perworker::my_id()) -> void {
    update_tree(cdelta_type{.stealers = +1}, my_id);
    ensure_sentinel();
  }

  static
  auto decr_stealing(size_t my_id = perworker::my_id()) -> void {
    update_tree(cdelta_type{.stealers = -1}, my_id);
    // scale up
    auto n = alpha;
    while (n > 0) {
      auto c = nr->counter.load();
      if ((! c.needs_sentinel()) || c.suspended == 0) {
        break;
      }
      if (try_resume(random_suspended_worker())) {
        n--;
      }
    }
    ensure_sentinel();
  }

  static
  auto try_suspend(size_t) -> void {
    auto my_id = perworker::my_id();
    auto flip = [&] () -> bool {
      auto n = random_number(my_id);
      // LATER: replace the coin flip below b/c currently the
      // calculation inconsistent with the defintion of beta given in
      // the paper
      return (n % beta) == 0;
      //return (beta == 1) ? true : (n % beta) < (beta - 1);
    };
    if (! flip()) {
      return;
    }
    // scale down
    flags[my_id].store(true);
    update_tree(cdelta_type{.stealers = -1, .suspended = +1}, my_id);
    if (needs_sentinel()) {
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
    update_tree(cdelta_type{.stealers = +1, .suspended = -1}, my_id);
    ensure_sentinel();
  }
  
  static
  auto try_resume(int id) -> bool {
    if (id == -1) {
      return false;
    }
    auto orig = true;
    if (flags[id].compare_exchange_strong(orig, false)) {
      semaphores[id].post();
      return true;
    }
    return false;
  }
  
#ifdef TASKPARTS_ELASTIC_OVERRIDE_RAND_WORKER
  static constexpr
  bool override_rand_worker = true;
#else
  static constexpr
  bool override_rand_worker = false;
#endif
  
  static
  auto random_in_range(std::pair<size_t, size_t> r) -> int {
    if (r.second <= r.first) {
      return -1;
    }
    auto i = random_number() % (r.second - r.first);
    return i + r.first;
  }
  
  // nd: index of a leaf node in tree
  static
  auto workers(int nd) -> std::pair<size_t, size_t> {
    auto i = nd - tree_index_of_first_leaf();
    auto lo = i * nb_workers_per_leaf();
    auto hi = std::min(nb_workers_per_leaf() + lo,
                       perworker::nb_workers());
    return std::make_pair(lo, hi);
  }
  
  static
  auto random_suspended_worker(size_t my_id = perworker::my_id()) -> int {
    int id;
    if (override_rand_worker) {
      id = random_in_range(random_worker_group([] (cdata_type c) { return c.suspended; }));
    } else {
      id = random_other_worker(my_id);
    }
    auto is_suspended = flags[id].load();
    return is_suspended ? id : -1;
  }
  
  template <typename Is_deque_empty>
  static
  auto random_worker_with_surplus(const Is_deque_empty& is_deque_empty,
                                  size_t my_id = perworker::my_id()) -> int {
    int id;
    if (override_rand_worker) {
      id = random_in_range(random_worker_group([] (cdata_type c) { return c.surplus; }));
    } else {
      id = (int)random_other_worker(my_id);
    }
    auto has_surplus = ! is_deque_empty(id);
    return has_surplus ? id : -1;
  }
  
  template <typename F>
  static
  auto random_worker_group(const F& f,
                           size_t h = tree_height) -> std::pair<size_t, size_t> {
    std::pair<size_t, size_t> ps;
    auto nd = 0;
    auto l = 0;
    while (true) {
      auto ni = &tree[nd];
      auto wi = f(ni->counter.load());
      if (wi == 0) {
        break;
      }
      if ((l + 1) == h) {
        if (wi > 0) {
          ps = workers(nd);
        }
        break;
      }
      auto il = 2 * nd + 1;
      auto ir = 2 * nd + 2;
      auto wl = f(tree[il].counter.load());
      auto wr = f(tree[ir].counter.load());
      nd = ((random_number() % (wl + wr)) < wl) ? il : ir;
      l++;
    }
    return ps;
  }
  
  static
  auto nb_nodes(int lg) -> int {
    assert(lg >= 0);
    return (1 << (lg + 1)) - 1;
  }
  
  static
  auto nb_nodes() -> int {
    return nb_nodes(tree_height);
  }
  
  static
  auto tree_index_of_first_leaf() -> int {
    return nb_nodes(std::max(0, (int)tree_height - 1));
  }
  
  static
  auto nb_leaves() -> int {
    return 1 << tree_height;
  }
  
  static
  auto nb_workers_per_leaf() -> size_t {
    return perworker::nb_workers() / nb_leaves();
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
    if (const auto env_p = std::getenv("TASKPARTS_ELASTIC_TREE_HEIGHT")) {
      tree_height = std::max(0, std::stoi(env_p));
    } else {
      tree_height = 0;
      while ((1 << tree_height) < perworker::nb_workers()) {
        tree_height++;
      }
      tree_height = std::max((size_t)0, tree_height - 1);
    }
    tree = new cnode_type[nb_nodes()];
    for (size_t i = 0; i < nb_nodes(); i++) {
      auto& d = tree[i].delta;
      cdelta_type d0{
        .locked = node_unlocked, .stealers = 0,
        .suspended = 0, .surplus = 0
      };
      d.store(d0);
    }
    auto is_root = [&] (int n) -> bool {
      return n == 0;
    };
    auto parent_of = [&] (int n) -> int {
      assert(! is_root(n));
      return (n - 1) / 2;
    };
    // initialize each array in the paths structure s.t. each such array stores its
    // leaf-to-root path in the tree
    // n: index of a leaf node in the heap array
    auto mk_path = [&] (int n) -> std::vector<cnode_type*> {
      std::vector<cnode_type*> r;
      if (tree_height == 0) {
        r.push_back(&tree[0]);
        return r;
      }
      do {
        r.push_back(&tree[n]);
        n = parent_of(n);
      } while (! is_root(n));
      r.push_back(&tree[n]);
      std::reverse(r.begin(), r.end());
      return r;
    };
    // create a path for each worker
    for (size_t i = 0, j = 0; i < perworker::nb_workers(); i++) {
      auto nd = tree_index_of_first_leaf() + j;
      j += (((i + 1) % nb_workers_per_leaf()) == 0);
      paths[i] = mk_path((int)nd);
      assert(paths[i].size() == (tree_height + 1));
    }
    nr = paths[0][0];
  }

};

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
int elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::alpha;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
int elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::beta;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
typename elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::cnode_type* elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::tree;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
typename elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::cnode_type* elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::nr;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
perworker::array<std::vector<typename elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::cnode_type*>> elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::paths;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
perworker::array<Semaphore> elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::semaphores;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
perworker::array<std::atomic_bool> elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::flags;

template <typename Stats, typename Logging, typename Semaphore, size_t max_lg_tree_sz>
size_t elastic<Stats, Logging, Semaphore, max_lg_tree_sz>::tree_height;

/*---------------------------------------------------------------------*/
/* Elastic work stealing (with S3 counter) */

template <typename Stats, typename Logging, typename Semaphore=dflt_semaphore>
class elastic_s3 {
public:
  
  using cdata_type = struct cdata_struct {
    int32_t surplus = 0;
    int16_t suspended = 0;
    int16_t stealers = 0;
    auto needs_sentinel() -> bool {
      return (surplus >= 1) && (stealers == 0) && (suspended >= 1);
    }
  };
  
  static
  std::atomic<cdata_type> counter;
  
  static
  perworker::array<std::atomic_bool> flags;
  
  static
  perworker::array<Semaphore> semaphores;

  static
  int alpha;

  static
  int beta;

  static constexpr
  bool override_rand_worker = false;

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
    for (size_t i = 0; i < perworker::nb_workers(); i++) {
      flags[i].store(false);
    }
  }
    
  template <typename Update>
  static
  auto update_counters(const Update& u) -> cdata_type {
    return update_atomic(counter, u);
  }
    
  static
  auto try_resume(size_t id) -> bool {
    auto orig = true;
    if (flags[id].compare_exchange_strong(orig, false)) {
      semaphores[id].post();
      return true;
    }
    return false;
  }
  
  static
  auto incr_surplus(size_t my_id = perworker::my_id()) {
    auto next = update_counters([] (cdata_type d) {
      d.surplus++;
      return d;
    });
    while (next.needs_sentinel()) {
      auto id = random_number(my_id) % perworker::nb_workers();
      if (try_resume(id)) {
        break;
      }
      next = counter.load();
    }
    Stats::increment(Stats::configuration_type::nb_surplus_transitions);
  }
  
  static
  auto decr_surplus(size_t id) {
    update_counters([] (cdata_type d) {
      d.surplus--;
      return d;
    });
  }
  
  static
  auto incr_stealing(size_t my_id = perworker::my_id()) {
    update_counters([] (cdata_type d) {
      d.stealers++;
      return d;
    });
  }
  
  static
  auto decr_stealing(size_t my_id = perworker::my_id()) {
    auto next = update_counters([] (cdata_type d) {
      d.stealers--;
      return d;
    });
    auto n = alpha;
    while ((n > 0) || (next.needs_sentinel())) {
      if (next.suspended == 0) {
        break;
      }
      auto id = random_number(my_id) % perworker::nb_workers();
      if (try_resume(id)) {
        n--;
      }
      next = counter.load();
    }
  }
  
  static
  auto try_suspend(size_t) {
    auto my_id = perworker::my_id();
    auto flip = [&] () -> bool {
      auto n = random_number(my_id);
      // LATER: replace the coin flip below b/c currently the
      // calculation inconsistent with the defintion of beta given in
      // the paper
      return (n % beta) == 0;
      //return (beta == 1) ? true : (n % beta) < (beta - 1);
    };
    if (! flip()) {
      worker_yield();
      return;
    }
    flags[my_id].store(true);
    auto next = update_counters([] (cdata_type d) {
      d.stealers--;
      d.suspended++;
      return d;
    });
    if (next.needs_sentinel()) {
      try_resume(my_id);
    }
    Stats::on_exit_acquire();
    Logging::log_enter_sleep(my_id, 0l, 0l);
    Stats::on_enter_sleep();
    semaphores[my_id].wait();
    Stats::on_exit_sleep();
    Logging::log_event(exit_sleep);
    Stats::on_enter_acquire();
    update_counters([=] (cdata_type d) {
      d.stealers++;
      d.suspended--;
      return d;
    });
    worker_yield();
  }
  
};

template <typename Stats, typename Logging, typename Semaphore>
std::atomic<typename elastic_s3<Stats, Logging, Semaphore>::cdata_type> elastic_s3<Stats, Logging, Semaphore>::counter;

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<std::atomic_bool> elastic_s3<Stats, Logging, Semaphore>::flags;

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<Semaphore> elastic_s3<Stats, Logging, Semaphore>::semaphores;

template <typename Stats, typename Logging, typename Semaphore>
int elastic_s3<Stats, Logging, Semaphore>::alpha;

template <typename Stats, typename Logging, typename Semaphore>
int elastic_s3<Stats, Logging, Semaphore>::beta;

} // end namespace
