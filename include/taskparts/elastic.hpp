#pragma once

#include <assert.h>
#include <optional>
#include <atomic>
#include <vector>

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
#include "combtree.hpp"

namespace taskparts {

#if defined(TASKPARTS_USE_SPINNING_SEMAPHORE) || defined(TASKPARTS_DARWIN)
using dflt_semaphore = spinning_counting_semaphore;
#else
using dflt_semaphore = semaphore;
#endif

/*---------------------------------------------------------------------*/
/* Atomic status word for elastic work stealing */
  
// A status word that can be operated on atomically
// 1) clear() will always success in bounded number of steps.
// 2) set_busy_bit() uses atomic fetch_and_AND. It is guaranteed to
//    succeed in bounded number of steps.
// 3) updateHead() may fail. It's upto the caller to verify that the
//    operations succeeded. This is to ensure that the operation completes
//    in bounded number of steps.
// Invariant: If a worker is busy, its head field points to itself
class atomic_status_word {
private:

  // Status word. 64-bits wide
  union status_word_union {
    uint64_t as_uint64; // The order of fields is significant 
    // Always initializes the first member
    struct {
      uint8_t  busybit  : 1 ;
      uint64_t priority : 56;
      uint8_t  head     : 7 ;  // Supports at most 128 processors
    } bits;
  };
  
  std::atomic<uint64_t> status_word;

public:
  // Since no processor can be a child of itself, the thread_id of the 
  // processor itself can be used as the nullary value of the head

  atomic_status_word() : status_word(UINT64_C(0)) {}

  atomic_status_word(uint64_t prio, uint8_t nullary_head) {
    clear(prio, nullary_head);
  }

  // 1) Unsets the busy bit
  // 2) Hashes and obtain a new priority
  // 3) Resets the head value
  auto clear(uint64_t prio, uint8_t nullary_head, bool is_busy=false) -> void {
    status_word_union word = {UINT64_C(0)};
    word.bits.busybit  = is_busy;   // Not busy
    word.bits.priority = prio; 
    word.bits.head     = nullary_head;
    status_word.store(word.as_uint64);
  }

  // Sets busy bit and returns the old status word
  auto set_busy_bit() -> status_word_union {
    status_word_union word = {UINT64_C(0)};
    word.bits.busybit = 1u; // I'm going to be busy
    word = {status_word.fetch_or(word.as_uint64)};
    return word;
  }

  // Update the head field while preserving all other fields
  auto cas_head(status_word_union word, uint8_t newHead) -> bool {
    uint64_t expected = word.as_uint64;
    auto word2 = word;
    word2.bits.head = newHead; // Update only the head field
    return status_word.compare_exchange_weak(expected, word2.as_uint64);
  }

  auto load() -> status_word_union {
    return status_word_union{status_word.load()};
  }
};

/*---------------------------------------------------------------------*/
/* Elastic work stealing (lifeline based) */

template <typename Stats, typename Logging, typename Semaphore=dflt_semaphore>
class elastic {
public:

  static constexpr bool override_rand_worker = false;

  // Grouping fields for elastic scheduling together for potentiallly
  // better cache behavior and easier initialization.
  using elastic_fields_type = struct elastic_fields_struct {
    atomic_status_word status;
    Semaphore sem;
    size_t next;    // Next pointer for the wake-up list
    hash_value_type rng;
  };

  static
  perworker::array<elastic_fields_type> fields;

  // Busybit is set separately, this function only traverses and wakes people up
  static
  auto wake_children() {
    auto my_id = perworker::my_id();
    fields[my_id].status.set_busy_bit();
    auto status = fields[my_id].status.load();
    auto idx = status.bits.head;
    while (idx != my_id) {
      Logging::log_wake_child(idx);
      // IMPORTANT!
      // We must first access the next field before sem_post()
      // other wise it's possible for the waken up processor to sleep
      // on yet another processor before we access its next field, 
      // changing its next field.
      auto nextIdx = fields[idx].next;
      fields[idx].sem.post();
      idx = nextIdx;
    }
  }

  static
  auto try_to_sleep(size_t target) {
    // For whatever reason we failed to steal from our victim
    // It is possible that we are in this branch because the steal failed
    // due to contention instead of empty queue. However we are still safe 
    // because of the busy bit.
    auto my_id = perworker::my_id();
    assert(target != my_id);
    auto target_status = fields[target].status.load();
    auto my_status = fields[my_id].status.load();
    if ((! target_status.bits.busybit) && 
        (target_status.bits.priority > my_status.bits.priority)){
      fields[my_id].next = target_status.bits.head;
      // It's safe to just leave it in the array even if the following
      // CAS fails because it will never be referenced in case of failure.
      if (fields[target].status.cas_head(target_status, my_id)) {
        // Wait on my own semaphore
	Stats::on_exit_acquire();
        Logging::log_enter_sleep(target, target_status.bits.priority, my_status.bits.priority);
	Stats::on_enter_sleep(); 
        fields[my_id].sem.wait();
	Stats::on_exit_sleep(); 
        // Must not set busybit here, because it will go back to stealing
        Logging::log_event(exit_sleep);
	Stats::on_enter_acquire();
        // TODO: Add support for CRS
      } // Otherwise we just give up
    } else {
      // Logging::log_failed_to_sleep(k, target_status.bits.busybit, target_status.bits.priority, my_status.bits.priority);
    }
  }

  static
  auto on_enter_acquire() {
    // 1) Clear the children list 
    // 2) Start to accept lifelines by unsetting busy bit
    // 3) Randomly choose a new priority
    auto my_id = perworker::my_id();
    auto& rn = fields[my_id].rng;
    rn = hash(uint64_t(rn)); 
    fields[my_id].status.clear(rn, my_id, false);
  }

  static
  auto initialize() {
    for (size_t i = 0; i < fields.size(); ++i) {
      // We need to start off by setting everyone as busy
      // Using the first processor's rng to initialize everyone's prio seems fine
      auto rn = hash(i + 31);
      fields[i].rng = rn;
      fields[i].status.clear(rn, i, true);
      // We don't really care what next points to at this moment
    }
  }
  
  static
  auto try_to_wake_one_worker() -> bool { return false; }
};

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<typename elastic<Stats,Logging,Semaphore>::elastic_fields_type> elastic<Stats,Logging,Semaphore>::fields;

/*---------------------------------------------------------------------*/
/* Elastic work stealing (without lifelines) */
  
template <typename Stats, typename Logging, typename semaphore=dflt_semaphore, typename spinlock=spinlock>
class elastic_flat {
public:
  static constexpr bool override_rand_worker = true;

  // Processor status
  enum class status_t {
    Working, Stealing, Sleeping
  };

  // Compacted for cache friendly-ness
  struct fields {
    // Rng seed
    hash_value_type rng __attribute__ ((aligned (TASKPARTS_CACHE_LINE_SZB)));
    // Processor status words
    std::atomic<status_t> status __attribute__ ((aligned (TASKPARTS_CACHE_LINE_SZB)));
    // Spinlocks for concurrency control
    spinlock lock __attribute__ ((aligned (TASKPARTS_CACHE_LINE_SZB)));
    // Semaphore for putting processors to sleep;
    semaphore sem __attribute__ ((aligned (TASKPARTS_CACHE_LINE_SZB)));
  };

  static perworker::array<fields> field;

  static constexpr struct status_proxy {
    auto& operator[](size_t i) const { return field[i].status; }
  } status{};
  static constexpr struct rng_proxy {
    auto& operator[](size_t i) const { return field[i].rng; }
  } rng{};
  static constexpr struct lock_proxy {
    auto& operator[](size_t i) const { return field[i].lock; }
  } lock{};
  static constexpr struct sem_proxy    {
    auto& operator[](size_t i) const { return field[i].sem; }
  } sem{};


  // concurrent random set for tracking awake processors
#ifndef TASKPARTS_USE_CRS_COMBTREE
  class crs {
  public:

    // Init, Add and Remove are dummy methods as it is backed by status words
    static 
    void initialize() { }

    static
    void add(size_t p) {  }

    static
    void remove(size_t p) { }

    static
    size_t sample(size_t& nb_sa, size_t nb_workers, size_t my_id) {
      assert(nb_workers != 1);
      auto id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
      if (id >= my_id) {
        id++;
      }
      nb_sa++;
      return id;
    }
  };

#else
  class crs {
  public:

    static
    combtree* ct;

    //static constexpr
    //    int nb_per_leaf = 2;

    // Init, Add and Remove are dummy methods as it is backed by status words
    static 
    void initialize() {
      size_t nb_workers = perworker::id::get_nb_workers();
      int height = 0;
      {
	while (nb_workers >>= 1) ++height;
      }
      height = std::max(1, height /*- nb_per_leaf*/);
      ct = new combtree(height);
    }

    static
    size_t leaf_of(size_t p) {
      return p; // % nb_per_leaf;
    }

    static
    void add(size_t p) {
      ct->increment_leaf_counter(leaf_of(p), 1);
    }

    static
    void remove(size_t p) {
      ct->increment_leaf_counter(leaf_of(p), -1);
    }

    static
    size_t sample(size_t& nb_sa, size_t nb_workers, size_t my_id) {
      assert(nb_workers != 1);
      auto f = [&] (size_t n) {
	if (ct->is_leaf(n)) {
	  return n;
	}
	auto sl = ct->read_counter(ct->left_child_of(n));
	auto sr = ct->read_counter(ct->right_child_of(n));
	auto s = sl + sr;
	auto c = (size_t)((hash(my_id) + hash(nb_sa) + hash(n)) % s);
	if (c < sl) {
	  return f(ct->left_child_of(n));
	} else {
	  return f(ct->right_child_of(n));
	}
      };
      auto id0 = f(0);
      auto id = id0 - ct->get_first_leaf() % (nb_workers - 1);
      if (id >= my_id) {
        id++;
      }
      nb_sa++;
      return id;
    }
  };
#endif
  
  // worker pool for tracking asleep processors
  class pool {
  public:
    // Init/Add is dummy it is backed by status words
    static 
    void initialize() { }

    static
    void add(size_t p) { }

    static
    std::optional<size_t> choose() {
      auto size = field.size();
      auto p = perworker::my_id();
      // No need to wake up one's self
      for (size_t i = 1u; i < size; ++i){
        auto v = (i + p) % size;
        auto expected = status_t::Sleeping;
        if (status[v].load() == expected) {
          if (status[v].compare_exchange_strong(expected, status_t::Stealing)) {
            return v; 
          }
        }
      }
      return std::nullopt;
    }
  };

  static
  auto initialize() {
    crs::initialize();
    assert(status[0].is_lock_free());
    // Semaphore and spinlocks are initialized on construction
    for (size_t i = 0; i < field.size(); ++i) {
      status[i].store(status_t::Working);
    }
  }

  static
  auto random_other_worker(size_t& nb_sa, size_t nb_workers, size_t my_id) -> size_t {
    assert(nb_workers != 1);
    assert(status[my_id].load() == status_t::Stealing);
    size_t id;
    do {
      id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
      if (id >= my_id) {
	id++;
      }
      nb_sa++;
      // fprintf(stderr, "[%ld] Stealing: Attempting %ld. \n", my_id, id);
    } while(status[id].load() == status_t::Sleeping);
    //fprintf(stderr, "[%ld] Stealing: Chosen %ld. \n", my_id, id);
    return id;
  }

  static 
  bool lock2(size_t l1, size_t l2) {
    assert(l1 != l2);
    auto [low, high] = l1 < l2 ? std::pair{l1, l2} : std::pair{l2, l1};
    auto& l_lock = lock[low];
    auto& h_lock = lock[high];
    if (l_lock.try_lock()) {
      if (h_lock.try_lock()) {
        // Both locks went through
        return true;
      } else {
        // High lock failed, bail out
        l_lock.unlock();
        return false;
      }
    } else {
      return false;
    }
  }

  static
  void unlock2(size_t l1, size_t l2) {
    lock[l1].unlock();
    lock[l2].unlock();
  }

  // This is called when a steal attempt failed
  static
  auto try_to_sleep(size_t v) {
    auto p = perworker::my_id();
    //fprintf(stderr, "[%ld] Try to sleep %ld.\n", p, v);
    if (lock2(v, p)) {
      if (status[v].load() == status_t::Stealing) {
        status[p].store(status_t::Sleeping);
        //fprintf(stderr, "[%ld] Sleeping on %ld.\n", p, v);
        pool::add(p);
        unlock2(v, p);
        crs::remove(p);
        Stats::on_exit_acquire();
        Logging::log_enter_sleep(p, 0l, 0l);
        Stats::on_enter_sleep();
        sem[p].wait();
        Stats::on_exit_sleep();
        Logging::log_event(exit_sleep);
        Stats::on_enter_acquire();
        status[p].store(status_t::Stealing);
        //fprintf(stderr, "[%ld] Stealing: Woken up.\n", p);
        crs::add(p);
      } else {
        unlock2(v, p);
      }
    }
  }

  // This is called when a steal attempt succeeded
  static
  auto wake_children() {
    auto p = perworker::my_id();
    lock[p].lock(); // This have to go through
    status[p].store(status_t::Working);
    //fprintf(stderr, "[%ld] Working: Waking up other procs. \n", p);
    lock[p].unlock();
    auto w = pool::choose();
    if (w) {
      Logging::log_wake_child(*w);
      sem[*w].post();
      //fprintf(stderr, "[%ld] Working: Waking up %ld. \n", p, *w);
    }
  }

  // This is called once every time processor start to acquire work
  static
  auto on_enter_acquire() {
    auto p = perworker::my_id();
    status[p].store(status_t::Stealing);
    //fprintf(stderr, "[%ld] Stealing: Attempting to acquire work.\n", p);
  }
  
};

template <typename Stats, typename Logging, typename Semaphore, typename Spinlock>
perworker::array<typename elastic_flat<Stats,Logging,Semaphore,Spinlock>::fields>
elastic_flat<Stats,Logging,Semaphore,Spinlock>::field;

#ifdef TASKPARTS_USE_CRS_COMBTREE
template <typename Stats, typename Logging, typename Semaphore, typename Spinlock>
combtree*
elastic_flat<Stats,Logging,Semaphore,Spinlock>::crs::ct;
#endif

/*---------------------------------------------------------------------*/
/* Elastic work stealing (driven by surplus) */

template <typename Stats, typename Logging, typename Semaphore=dflt_semaphore>
class elastic_surplus {
public:
  
  using counters_type = struct counters_struct {
    int32_t surplus;
    int32_t sleeping;
  };
  
  static
  perworker::array<std::atomic<counters_type>> worker_status;

  static
  std::atomic<counters_type> global_status;
  
  static
  perworker::array<Semaphore> semaphores;

  static
  perworker::array<std::atomic<int64_t>> epochs;

  using update_counters_result_type = enum update_counters_result_enum {
    update_counters_exited_early,
    update_counters_succeeded,
    update_counters_failed
  };

  static
  size_t nb_to_wake_on_surplus_increase;

  template <typename Update, typename Early_exit>
  static
  auto try_to_update_counters(std::atomic<counters_type>& status,
                              const Update& update,
                              const Early_exit& should_exit) -> update_counters_result_type {
    auto v = status.load();
    if (should_exit(v)) {
      return update_counters_exited_early;
    }
    auto orig = v;
    auto next = update(orig);
    auto b = status.compare_exchange_strong(orig, next);
    return b ? update_counters_succeeded : update_counters_failed;
  }

  template <typename Update, typename Early_exit>
  static
  auto update_counters_or_exit_early(std::atomic<counters_type>& status,
                                     const Update& update,
                                     const Early_exit& should_exit) -> update_counters_result_type {
    auto r = try_to_update_counters(status, update, should_exit);
    while (r == update_counters_failed) {
      r = try_to_update_counters(status, update, should_exit);
    };
    return r;
  }

  template <typename Update, typename Early_exit>
  static
  auto update_counters(std::atomic<counters_type>& status,
                       const Update& update,
                       const Early_exit& should_exit) {
    auto r = try_to_update_counters(status, update, should_exit);
    while (r != update_counters_succeeded) {
      r = try_to_update_counters(status, update, should_exit);
    }
  }

  template <typename Update>
  static
  auto update_counters(std::atomic<counters_type>& status, const Update& update) {
    return update_counters(status, update, [] (counters_type) { return false; });
  }
  
  static
  auto before_surplus_increase() -> int64_t {
    auto& my_epoch = epochs.mine();
    auto e = my_epoch.load() + 1;
    my_epoch.store(e);
    auto& my_status = worker_status.mine();
    // the line below seems bogus bc other workers might cause the update below to fail
    auto r1 = try_to_update_counters(my_status, [=] (counters_type s) {
      s.surplus++;
      return s;
    }, [=] (counters_type s) {
      assert(s.sleeping == 0);
      return s.surplus == 1;
    });
    if (r1 == update_counters_exited_early) {
      return e;
    }
    update_counters(global_status, [=] (counters_type s) {
      s.surplus++;
      return s;
    });
    return e;
  }

  static
  auto try_to_wake_others(size_t my_id = perworker::my_id()) {
    size_t nb_sa = 0;
    auto random_other_worker = [&] (size_t my_id) -> size_t {
      auto nb_workers = perworker::nb_workers();
      size_t id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
      if (id >= my_id) {
        id++;
      }
      nb_sa++;
      return id;
    };
    auto try_to_wake_target = [] (size_t target_id) -> bool {
      auto& tgt_status = worker_status[target_id];
      auto r = update_counters_or_exit_early(tgt_status, [=] (counters_type s) {
        s.sleeping--;
        assert(s.sleeping == 0);
        assert(s.surplus == 0);
        return s;
      }, [=] (counters_type s) {
        return (s.sleeping < 1) || (s.surplus > 0);
      });
      if (r != update_counters_succeeded) {
        return false;
      }
      assert(tgt_status.load().sleeping == 0);
      semaphores[target_id].post();
      return true;
    };
    int nb_to_wake = nb_to_wake_on_surplus_increase;
    while (nb_to_wake > 0) {
      if (global_status.load().sleeping == 0) {
        return;
      }
      if (try_to_wake_target(random_other_worker(my_id))) {
        nb_to_wake--;
      }
    }
  }

  static
  auto after_surplus_increase() {
    try_to_wake_others();
  }

  static
  auto after_surplus_decrease(size_t target_id = perworker::my_id(), int64_t epoch = -1l) {
    auto is_thief = (target_id != perworker::my_id());
    auto& tgt_status = worker_status[target_id];
    auto r1 = update_counters_or_exit_early(tgt_status, [=] (counters_type s) {
      assert(s.surplus > 0);
      s.surplus--;
      return s;
    }, [=] (counters_type s) {
      return (is_thief && (epoch != epochs[target_id].load())) || (s.surplus == 0);
    });
    if (r1 == update_counters_exited_early) {
      return;
    }
    update_counters(global_status, [=] (counters_type s) {
      assert(s.surplus > 0);
      s.surplus--;
      return s;
    });
  }
  
  static
  auto on_enter_acquire() {
    after_surplus_decrease();
#ifndef NDEBUG
    auto my_status_val = worker_status.mine().load();
    assert(my_status_val.surplus == 0);
    assert(my_status_val.sleeping == 0);
#endif
  }
  
  static
  auto try_to_sleep(size_t) {
    auto my_id = perworker::my_id();
    auto& my_status = worker_status[my_id];
    auto r1 = try_to_update_counters(my_status, [=] (counters_type s) {
      s.sleeping++;
      return s;
    }, [=] (counters_type s) {
      assert(s.surplus == 0);
      assert(s.sleeping == 0);
      return false;
    });
    if (r1 == update_counters_failed) {
      return;
    }
    auto r2 = try_to_update_counters(global_status, [=] (counters_type s) {
      s.sleeping++;
      return s;
    }, [=] (counters_type s) {
      return s.surplus > 0;
    });
    if (r2 != update_counters_succeeded) {
      update_counters_or_exit_early(my_status, [=] (counters_type s) {
        assert(s.sleeping == 1);
        s.sleeping--;
        return s;
      }, [=] (counters_type s) {
        return s.sleeping == 0;
      });
      assert(my_status.load().sleeping == 0);
      return;
    }
    Stats::on_exit_acquire();
    Logging::log_enter_sleep(my_id, 0l, 0l);
    Stats::on_enter_sleep();
    semaphores[my_id].wait();
    Stats::on_exit_sleep();
    Logging::log_event(exit_sleep);
    Stats::on_enter_acquire();
    update_counters_or_exit_early(my_status, [=] (counters_type status) {
      assert(status.sleeping == 1);
      status.sleeping--;
      return status;
    }, [=] (counters_type status) {
      return status.sleeping == 0; // needed because wait() can exit either bc of queued post()s on the semaphore or bc of a worker w/ new surplus
    });
    assert(my_status.load().sleeping == 0);
    update_counters(global_status, [=] (counters_type status) {
      assert(status.sleeping > 0);
      status.sleeping--;
      return status;
    });
  }

  static
  auto initialize() {
    for (size_t i = 0; i < perworker::nb_workers(); i++) {
      assert(worker_status[i].load().surplus == 0);
      assert(worker_status[i].load().sleeping == 0);
      epochs[i].store(0);
    }
    if (const auto env_p = std::getenv("TASKPARTS_NB_TO_WAKE_ON_SURPLUS_INCREASE")) {
      nb_to_wake_on_surplus_increase = std::stoi(env_p);
    } else {
      nb_to_wake_on_surplus_increase = 2;
    }
  }
  
  static constexpr
  bool override_rand_worker = false;
  
  static
  auto wake_children() { }
  
};

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<std::atomic<typename elastic_surplus<Stats, Logging, Semaphore>::counters_type>> elastic_surplus<Stats, Logging, Semaphore>::worker_status;

template <typename Stats, typename Logging, typename Semaphore>
std::atomic<typename elastic_surplus<Stats, Logging, Semaphore>::counters_type> elastic_surplus<Stats, Logging, Semaphore>::global_status;

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<Semaphore> elastic_surplus<Stats, Logging, Semaphore>::semaphores;

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<std::atomic<int64_t>> elastic_surplus<Stats, Logging, Semaphore>::epochs;

template <typename Stats, typename Logging, typename Semaphore>
size_t elastic_surplus<Stats, Logging, Semaphore>::nb_to_wake_on_surplus_increase;

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
    gamma() : a(0), s(0) { }
  };
  
  class delta {
  public:
    unsigned int l : 1;  // lock bit (node_locked/node_unlocked)
    int a : 31;          // asleep
    int s;               // surplus
    delta() : l(node_unlocked), a(0), s(0) { }
  };

  using node = struct node_struct {
    int id = -1;
    std::atomic<gamma> g;
    std::atomic<delta> d;
  };
  
  static
  size_t lg_P;

  static
  cache_aligned_fixed_capacity_array<node, 1 << max_lg_P> heap;
  
  static
  perworker::array<std::vector<node*>> paths;
  
  static
  perworker::array<Semaphore> semaphores;

  static
  perworker::array<std::atomic<int64_t>> epochs;
  
  static
  perworker::array<size_t> random_number_generator;
  
  static
  auto is_empty_delta(delta d) -> bool {
    return ((d.s == 0) && (d.a == 0));
  }
  
  static
  auto combine_changes(delta d_o, delta d_n) -> delta {
    d_o.l = node_locked;
    d_o.a += d_n.a;
    d_o.s += d_n.s;
    return d_o;
  }

  static
  auto apply_changes(node* n, delta d) {
    auto g = n->g.load();
    g.a += d.a;
    g.s += d.s;
    n->g.store(g);
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
  auto position_of_first_leaf() -> size_t {
    return nb_nodes(lg_P);
  }

  static
  auto position_of_leaf_node_at(size_t i) -> size_t {
    if (lg_P == 0) {
      assert(i == 0);
      return 0;
    }
    auto l = position_of_first_leaf();
    auto k = l + (i & (l - 1));
    auto n = nb_nodes();
    assert(k >= 0 && k < n);
    return k;
  }
  
  static
  auto leaf_node_at(size_t i) -> node* {
    return &heap[position_of_leaf_node_at(i)];
  }
  
  static
  auto is_leaf_node(int i) -> bool {
    return i >= position_of_first_leaf();
  }
  
  static
  auto path_size() -> int {
    assert((int)paths.mine().size() == (lg_P + 1));
    return lg_P + 1;
  }
  
  // returns position j s.t.
  //  (1) j = -1, if the call propagated up to and including the root node
  //  (2) j = either i or the position of the parent of the previously updated node, otherwise
  static
  auto up(size_t id, int i, delta d_n) -> int {
    for (; i >= 0; i--) {
      if (is_empty_delta(d_n)) {
        break;
      }
      auto n = paths[id][i];
      delta d_o;
      while (true) {
        d_o = n->d.load();
        auto d = combine_changes(d_o, d_n);
        auto d2 = d;
        if (d_o.l == node_unlocked) {
          d2.s = 0;
          d2.a = 0;
        }
        if (n->d.compare_exchange_strong(d_o, d2)) {
          d_n = d;
          break;
        }
      }
      if (d_o.l == node_locked) {
        break;
      }
      assert(d_o.l == node_unlocked);
      apply_changes(n, d_n);
    }
    return i;
  }

  // returns position j s.t.
  //   (1) j = path_size(), if the call propagated all the way down to the leaf
  //   (2) 0 < j < path_size(), if there's a pending change to the node at
  //       position j in the path
  static
  auto down(size_t id, int i) -> int {
    i++;
    for (; (i + 1) < path_size(); i++) {
      auto n = paths[id][i];
      auto d_o = n->d.load();
      assert(d_o.l == node_locked);
      while (true) {
        if (! is_empty_delta(d_o)) {
          return i;
        }
        auto d_n = d_o;
        d_n.l = node_unlocked;
        if (n->d.compare_exchange_strong(d_o, d_n)) {
          break;
        }
      }
    }
    return i;
  }

  static
  auto update_path_to_root(size_t id, int i, delta d) {
    auto take_delta_from = [] (size_t id, int i) -> delta {
      auto n = paths[id][i];
      auto d_o = n->d.load();
      while (true) {
        assert(d_o.l == node_locked);
        delta d_n;
        d_n.l = node_locked;
        if (n->d.compare_exchange_strong(d_o, d_n)) {
          apply_changes(n, d_o);
          break;
        }
      }
      return d_o;
    };
    assert(i < path_size());
    while (true) {
      i = up(id, i, d);
      i = down(id, i);
      if ((i + 1) >= path_size()) {
        break;
      }
      d = take_delta_from(id, i);
    }
  }
  
  static constexpr
  bool override_rand_worker = false;

  static
  auto wake_children() { }

  static
  auto on_enter_acquire() {
    after_surplus_decrease();
  }

  static
  auto initialize() {
    // write ids of workers in their leaf nodes
    auto j = position_of_first_leaf();
    for (auto i = 0; i < nb_leaves(); i++, j++) {
      heap[j].id = i;
    }
    // find the nearest setting of lg_P s.t. lg_P is the smallest
    // number for which 2^{lg_P} < perworker::nb_workers()
    lg_P = 0;
    while ((1 << lg_P) < perworker::nb_workers()) {
      lg_P++;
    }
    // initialize tree nodes
    for (size_t i = 0; i < nb_nodes(); i++) {
      new (&heap[i]) node;
    }
    // initialize each array in the paths structure s.t. each such array stores its
    // leaf-to-root path in the tree
    // n: index of a leaf node in the heap array
    auto mk_path_from_leaf_to_root = [] (int n) -> std::vector<node*> {
      std::vector<node*> r;
      if (lg_P == 0) {
        assert(nb_nodes() == 1);
        r.push_back(&heap[0]);
        return r;
      }
      do {
        r.push_back(&heap[n]);
        n = parent_of(n);
      } while (! is_root(n));
      assert(is_root(n));
      r.push_back(&heap[n]);
      std::reverse(r.begin(), r.end());
      return r;
    };
    for (auto i = 0; i < nb_leaves(); i++) {
      paths[i] = mk_path_from_leaf_to_root(position_of_leaf_node_at(i));
      assert(paths[i].size() == path_size());
      paths[i][path_size() - 1]->id = i;
    }
  }
  
  static
  auto before_surplus_increase() -> int64_t {
    auto my_id = perworker::my_id();
    auto& my_epoch = epochs[my_id];
    auto e = my_epoch.load() + 1;
    my_epoch.store(e);
    auto i = path_size() - 1;
    { // update the leaf node
      auto n = paths[my_id][i];
      while (true) {
        auto g_o = n->g.load();
        assert(g_o.s == 0);
        auto g_n = g_o;
        g_n.s = 1;
        if (n->g.compare_exchange_strong(g_o, g_n)) {
          break;
        }
      }
    }
    // update interior nodes on the path from the
    // current leaf to the root
    delta d;
    d.s = +1;
    update_path_to_root(my_id, i - 1, d);
    return e;
  }
  
  static
  auto after_surplus_increase() -> void {
    try_to_wake_others();
  }
  
  static
  auto after_surplus_decrease(size_t target_id = perworker::my_id(), int64_t epoch = -1l) {
    auto epoch_expired = [&] {
      auto is_thief = (target_id != perworker::my_id());
      auto new_epoch = (epoch != epochs[target_id].load());
      return is_thief && new_epoch;
    };
    auto i = path_size() - 1;
    auto n = paths[target_id][i];
    { // update the leaf node
      auto g_o = n->g.load();
      while (true) {
        if (epoch_expired() || (g_o.s == 0)) {
          // lost the race
          return;
        }
        assert(g_o.s == 1);
        auto g_n = g_o;
        g_n.s = 0;
        if (n->g.compare_exchange_strong(g_o, g_n)) {
          break;
        }
        g_o = n->g.load();
      }
    }
    // update interior nodes on the path from the
    // current leaf to the root
    delta d;
    d.s = -1;
    update_path_to_root(target_id, i - 1, d);
  }

  static
  auto try_to_wake_others(size_t my_id = perworker::my_id()) -> void {
    auto try_to_wake = [&] (size_t target_id) {
      assert(target_id != my_id);
      auto i = path_size() - 1;
      auto n = paths[target_id][i];
      while (true) {
        auto g_o = n->g.load();
        if (g_o.a != 1) {
          return false;
        }
        auto g_n = g_o;
        g_n.a = 0;
        if (n->g.compare_exchange_strong(g_o, g_n)) {
          break;
        }
      }
      semaphores[target_id].post();
      return true;
    };
    int nb_to_wake = 2;
    while (nb_to_wake > 0) {
      if (heap[0].g.load().a == 0) {
        // no workers are reporting they are asleep now
        return;
      }
      auto i = sample_by_sleeping(my_id);
      if (i == -1) {
        continue;
      }
      if (try_to_wake(i)) {
        nb_to_wake--;
      }
    }
  }
  
  static
  auto try_to_sleep(size_t) {
    auto surplus_exists = [] {
      return heap[0].g.load().s > 0;
    };
    auto my_id = perworker::my_id();
    auto i = path_size() - 1;
    auto n = paths[my_id][i];
    while (true) {
      if (surplus_exists()) {
        return;
      }
      auto g_o = n->g.load();
      assert(g_o.a == 0);
      auto g_n = g_o;
      g_n.a = 1;
      if (n->g.compare_exchange_strong(g_o, g_n)) {
        break;
      }
    }
    {
      delta d;
      d.a = +1;
      update_path_to_root(my_id, i - 1, d);
    }
    Stats::on_exit_acquire();
    Logging::log_enter_sleep(my_id, 0l, 0l);
    Stats::on_enter_sleep();
    semaphores[my_id].wait();
    Stats::on_exit_sleep();
    Logging::log_event(exit_sleep);
    Stats::on_enter_acquire();
    while (true) {
      auto g_o = n->g.load();
      assert(g_o.a == 0);
      auto g_n = g_o;
      g_n.a = 0;
      if (n->g.compare_exchange_strong(g_o, g_n)) {
        break;
      }
    }
    {
      delta d;
      d.a = -1;
      update_path_to_root(my_id, i - 1, d);
    }
  }
  
  static
  auto child_of(int n, int d) -> int {
    assert((d == 1) || (d == 2));
    assert(n < nb_nodes());
    auto c = n * 2 + d;
    assert(c < nb_nodes());
    return c;
  }
  
  static
  auto random_number(size_t my_id, size_t hi) -> size_t {
    auto& rng = random_number_generator[my_id];
    auto n = (hash(my_id) + hash(rng)) % hi;
    rng++;
    return n;
  }
  
  static
  auto flip(size_t my_id, int w1, int w2) -> int {
    auto w = w1 + w2;
    auto n = random_number(my_id, w);
    return (n < w1) ? 0 : 1;
  }
  
  template <typename F>
  static
  auto sample_path(size_t my_id, const F& f) -> int {
    int i, d;
    for (i = 0; ! is_leaf_node(i); i = child_of(i, d)) {
      auto n1 = &heap[child_of(i, 1)];
      auto n2 = &heap[child_of(i, 2)];
      d = flip(my_id, f(n1->g.load()), f(n2->g.load()));
    }
    // reached a leaf node
    assert(is_leaf_node(i));
    auto ni = &heap[i];
    if (f(ni->g.load()) <= 0) {
      return -1;
    }
    assert(ni->id >= 0);
    return ni->id;
  }
  
  static
  auto sample_by_surplus(size_t my_id = perworker::my_id()) -> int {
    return sample_path(my_id, [&] (gamma g) {
      return g.s;
    });
  }
  
  static
  auto sample_by_sleeping(size_t my_id = perworker::my_id()) -> int {
    return sample_path(my_id, [&] (gamma g) {
      return g.a;
    });
  }
  
};
  
template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
size_t elastic_tree<Stats, Logging, Semaphore, max_lg_P>::lg_P;

template <typename Stats, typename Logging, typename Semaphore,
          size_t max_lg_P>
cache_aligned_fixed_capacity_array<typename elastic_tree<Stats, Logging, Semaphore, max_lg_P>::node, 1 << max_lg_P> elastic_tree<Stats, Logging, Semaphore, max_lg_P>::heap;

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
