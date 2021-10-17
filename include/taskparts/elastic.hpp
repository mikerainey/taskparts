#pragma once

#include <assert.h>

#include <optional>

#include "perworker.hpp"
#include "atomic.hpp"
#include "hash.hpp"
#if defined(TASKPARTS_POSIX)
#include "posix/semaphore.hpp"
#include "posix/spinlock.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/semaphore.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

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
/* Elastic work stealing */

template <typename Stats, typename Logging, typename Semaphore=semaphore>
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
  
};

template <typename Stats, typename Logging, typename Semaphore>
perworker::array<typename elastic<Stats,Logging,Semaphore>::elastic_fields_type> elastic<Stats,Logging,Semaphore>::fields;

// Elsatic WS without Lifelines 
template <typename Stats, typename Logging, typename semaphore=semaphore, typename spinlock=spinlock>
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
    hash_value_type rng;
    // Processor status words
    std::atomic<status_t> status;
    // Spinlocks for concurrency control
    spinlock lock;
    // Semaphore for putting processors to sleep;
    semaphore sem;
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
  class crs {
  public:

    // Init, Add and Remove are dummy methods as it is backed by status words
    static 
    void initialize() { }

    static
    void add(size_t p) { }

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
            return i; 
          }
        }
      }
      return std::nullopt;
    }
  };

  static
  auto initialize() {
    assert(status[0].is_lock_free());
    // Semaphore and spinlocks are initialized on construction
    for (size_t i = 0; i < field.size(); ++i) {
      status[i].store(status_t::Working);
    }
  }

  static
  size_t random_other_worker(size_t& nb_sa, size_t nb_workers, size_t my_id) {
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
      fprintf(stderr, "[%ld] Stealing: Chosen %ld. \n", my_id, id);
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
    if (lock2(v, p)) {
      if (status[v].load() == status_t::Stealing) {
        status[p].store(status_t::Sleeping);
        fprintf(stderr, "[%ld] Sleeping.\n", p);
        pool::add(p);
        unlock2(v, p);
        crs::remove(p);
        sem[p].wait();
        status[p].store(status_t::Stealing);
        fprintf(stderr, "[%ld] Stealing: Woken up.\n", p);
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
    fprintf(stderr, "[%ld] Working: Waking up other procs. \n", p);
    lock[p].unlock();
    auto w = pool::choose();
    if (w) {
      sem[*w].post();
      fprintf(stderr, "[%ld] Working: Waking up %ld. \n", p, *w);
    }
  }

  // This is called once every time processor start to acquire work
  static
  auto accept_lifelines() {
    auto p = perworker::my_id();
    status[p].store(status_t::Stealing);
    fprintf(stderr, "[%ld] Stealing: Acquire work.\n", p);
  }
};

template <typename Stats, typename Logging, typename Semaphore, typename Spinlock>
perworker::array<typename elastic_flat<Stats,Logging,Semaphore,Spinlock>::fields> 
elastic_flat<Stats,Logging,Semaphore,Spinlock>::field;

} // end namespace
