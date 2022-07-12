#pragma once

#include <assert.h>
#include <optional>
#include <atomic>

namespace taskparts {
/*---------------------------------------------------------------------*/
/* Spinning binary semaphore (for performance debugging) */

/* An implementation of semaphore that is binary (0 or 1) with spinning wait 
 * - Crashes when POSTing to a non-zero valued semaphore
 * - Crashes when WAITint on a zero     valued semaphore
 * It should be fine for use in the elastic scheduler since 
 * - Only the processor owning the semaphore will ever wait on it
 * - Every WATI is paired with exactly one POST and vice versa.
 */
class spinning_binary_semaphore {
  
   // 0  -> No processor is waiting on the semaphore
   // 1  -> A post has been issued to the semaphore
   // -1 -> A processor is waiting on the semaphore 
   std::atomic<int> flag;
  
public:
  
  spinning_binary_semaphore() : flag(0) {}

  auto post() {
    int old_value = flag++;
    assert(old_value <= 0);
  }

  auto wait() {
    int old_value = flag--;
    assert(old_value >= 0);
    while (flag.load() < 0); // Spinning
  }

  ~spinning_binary_semaphore() {
    assert(flag.load() == 0); // The semaphore has to be balanced
  }
};
} // end namespace

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
using dflt_semaphore = spinning_binary_semaphore;
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
  auto accept_lifelines() {
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
  auto check_for_surplus_increase(bool) { }
  
  static
  auto check_for_surplus_decrease(bool) { }
  
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
  auto accept_lifelines() {
    auto p = perworker::my_id();
    status[p].store(status_t::Stealing);
    //fprintf(stderr, "[%ld] Stealing: Attempting to acquire work.\n", p);
  }
  
  static
  auto check_for_surplus_increase(bool) { }
  
  static
  auto check_for_surplus_decrease(bool) { }
  
  static
  auto try_to_wake_one_worker() -> bool { return false; }
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

template <typename Stats, typename Logging, typename semaphore=dflt_semaphore>
class elastic_surplus {
public:
  
  using cell_type = struct cell_struct {
    int32_t surplus;
    int32_t sleeping;
  };
  
  static
  perworker::array<std::atomic<cell_type>> worker_status;

  static
  std::atomic<cell_type> global_status;
  
  static
  perworker::array<semaphore> semaphores;
  
  static
  auto incr_surplus() {
    auto& my_status = worker_status.mine();
    do {
      auto my_status_val = my_status.load();
      if (my_status_val.surplus == 1) {
        return;
      }
      assert(my_status_val.surplus == 0);
      assert(my_status_val.sleeping == 0);
      auto my_orig = my_status_val;
      auto my_nxt = my_orig;
      my_nxt.surplus = 1;
      if (my_status.compare_exchange_strong(my_orig, my_nxt)) {
        do {
          auto gc_orig = global_status.load();
          auto gc_nxt = gc_orig;
          gc_nxt.surplus++;
          if (global_status.compare_exchange_strong(gc_orig, gc_nxt)) {
            return;
          }
        } while (true);
      }
    } while (true);
  }
  
  // returns true iff the decrement was needed
  static
  auto decr_surplus(size_t target_id) -> bool {
    auto& tgt_status = worker_status[target_id];
    auto tgt_status_val = tgt_status.load();
    if (tgt_status_val.surplus == 0) {
      return false;
    }
    assert(tgt_status_val.surplus == 1);
    assert(tgt_status_val.sleeping == 0);
    auto tgt_orig = tgt_status_val;
    auto tgt_nxt = tgt_orig;
    tgt_nxt.surplus = 0;
    if (tgt_status.compare_exchange_strong(tgt_orig, tgt_nxt)) {
      do {
        auto gc_orig = global_status.load();
        auto gc_nxt = gc_orig;
        gc_nxt.surplus--;
        if (global_status.compare_exchange_strong(gc_orig, gc_nxt)) {
          return true;
        }
      } while (true);
    }
    return false;
  }
  
  // returns true iff the increment was needed
  static
  auto incr_sleeping() -> bool {
    auto& my_status = worker_status.mine();
    do {
      auto my_status_val = my_status.load();
      assert(my_status_val.surplus == 0);
      assert(my_status_val.sleeping == 0);
      auto my_orig = my_status_val;
      auto my_nxt = my_orig;
      my_nxt.sleeping = 1;
      if (my_status.compare_exchange_strong(my_orig, my_nxt)) {
        do {
          auto gc_orig = global_status.load();
          if (gc_orig.surplus > 0) {
            my_nxt.sleeping = 0;
            my_status.store(my_nxt);
            return false;
          }
          auto gc_nxt = gc_orig;
          gc_nxt.sleeping++;
          if (global_status.compare_exchange_strong(gc_orig, gc_nxt)) {
            return true;
          }
        } while (true);
        return true;
      }
    } while (true);
  }
  
  static
  auto decr_sleeping() {
    do {
      auto gc_orig = global_status.load();
      auto gc_nxt = gc_orig;
      gc_nxt.sleeping--;
      if (global_status.compare_exchange_strong(gc_orig, gc_nxt)) {
        return;
      }
    } while (true);
  }
  
  static
  auto try_to_wake_target(size_t target_id) -> bool {
    auto& tgt_status = worker_status[target_id];
    auto tgt_status_val = tgt_status.load();
    do {
      if (tgt_status_val.sleeping != 1) {
        return false;
      }
      auto tgt_orig = tgt_status_val;
      auto tgt_nxt = tgt_orig;
      tgt_nxt.sleeping = 0;
      if (tgt_status.compare_exchange_strong(tgt_orig, tgt_nxt)) {
        break;
      }
    } while (true);
    semaphores[target_id].post();
    return true;
  }
  
  static
  auto try_to_wake_one_worker() -> bool {
    size_t target_id = 0;
    do {
      if (global_status.load().sleeping == 0) {
        return false;
      }
      target_id++;
      if (try_to_wake_target(target_id)) {
        return true;
      }
    } while (true);
  }

  static constexpr
  bool override_rand_worker = true;
  
  static
  auto random_other_worker(size_t& nb_sa, size_t nb_workers, size_t my_id) -> size_t {
    assert(nb_workers != 1);
    size_t id;
    do {
      id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
      if (id >= my_id) {
        id++;
      }
      nb_sa++;
    } while(true);
    return id;
  }

  static
  auto wake_children() { }

  static
  auto try_to_sleep(size_t my_id) {
    auto& my_status = worker_status[my_id];
    auto my_status_val = my_status.load();
    assert(my_status_val.surplus == 0);
    assert(my_status_val.sleeping == 0);
    decr_sleeping();
    semaphores[my_id].wait();
  }

  static
  auto check_for_surplus_increase(bool is_soon_nonempty) {
    if (! is_soon_nonempty) {
      return;
    }
    incr_surplus();
  }
  
  static
  auto check_for_surplus_decrease(bool is_soon_empty) {
    if (! is_soon_empty) {
      return;
    }
    auto my_id = perworker::my_id();
    decr_surplus(my_id);
  }
  
  static
  auto accept_lifelines() { }

  static
  auto initialize() { }
  
};

template <typename Stats, typename Logging, typename semaphore>
perworker::array<std::atomic<typename elastic_surplus<Stats, Logging, semaphore>::cell_type>> elastic_surplus<Stats, Logging, semaphore>::worker_status;

template <typename Stats, typename Logging, typename semaphore>
std::atomic<typename elastic_surplus<Stats, Logging, semaphore>::cell_type> elastic_surplus<Stats, Logging, semaphore>::global_status;

template <typename Stats, typename Logging, typename semaphore>
perworker::array<semaphore> elastic_surplus<Stats, Logging, semaphore>::semaphores;

} // end namespace
