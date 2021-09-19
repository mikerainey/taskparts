#pragma once

#include <atomic>
#include <memory>
#include <assert.h>

#include "perworker.hpp"
#include "fixedcapacity.hpp"
#include "scheduler.hpp"
#include "hash.hpp"

namespace taskparts {
  
/*---------------------------------------------------------------------*/
/* Chase-Lev Work-Stealing Deque data structure
 * 
 * This implementation is based on
 *   https://gist.github.com/Amanieu/7347121
 */
  
template <typename Fiber>
class chase_lev_deque {

  using index_type = long;
  
  class circular_array {
  private:
    
    cache_aligned_array<std::atomic<Fiber*>> items;
    
    std::unique_ptr<circular_array> previous;

  public:
    
    circular_array(index_type n) : items(n) {}
    
    index_type size() const {
      return items.size();
    }
    
    auto get(index_type index) -> Fiber* {
      return items[index % size()].load(std::memory_order_relaxed);
    }
    
    auto put(index_type index, Fiber* x) {
      items[index % size()].store(x, std::memory_order_relaxed);
    }
    
    auto grow(index_type top, index_type bottom) -> circular_array* {
      circular_array* new_array = new circular_array(size() * 2);
      new_array->previous.reset(this);
      for (index_type i = top; i != bottom; ++i) {
        new_array->put(i, get(i));
      }
      return new_array;
    }

  };

  std::atomic<circular_array*> array;
  
  std::atomic<index_type> top, bottom;

public:
  
  chase_lev_deque()
    : array(new circular_array(64)), top(0), bottom(0) {}
  
  ~chase_lev_deque() {
    circular_array* p = array.load(std::memory_order_relaxed);
    if (p) {
      delete p;
    }
  }

  auto size() -> index_type {
    auto b = bottom.load(std::memory_order_relaxed);
    auto t = top.load(std::memory_order_relaxed);
    return b - t;
  }

  auto empty() -> bool {
    return size() == 0;
  }

  auto push(Fiber* x) {
    auto b = bottom.load(std::memory_order_relaxed);
    auto t = top.load(std::memory_order_acquire);
    circular_array* a = array.load(std::memory_order_relaxed);
    if (b - t > a->size() - 1) {
      a = a->grow(t, b);
      array.store(a, std::memory_order_relaxed);
    }
    a->put(b, x);
    std::atomic_thread_fence(std::memory_order_release);
    bottom.store(b + 1, std::memory_order_relaxed);
  }

  auto pop() -> Fiber* {
    auto b = bottom.load(std::memory_order_relaxed) - 1;
    circular_array* a = array.load(std::memory_order_relaxed);
    bottom.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto t = top.load(std::memory_order_relaxed);
    if (t <= b) {
      auto x = a->get(b);
      if (t == b) {
        if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
          x = nullptr;
        }
        bottom.store(b + 1, std::memory_order_relaxed);
      }
      return x;
    } else {
      bottom.store(b + 1, std::memory_order_relaxed);
      return nullptr;
    }
  }

  auto steal() -> Fiber* {
    auto t = top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto b = bottom.load(std::memory_order_acquire);
    Fiber* x = nullptr;
    if (t < b) {
      circular_array* a = array.load(std::memory_order_relaxed);
      x = a->get(t);
      if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        return nullptr;
      }
    }
    return x;
  }
  
};
  
/*---------------------------------------------------------------------*/
/* Work-stealing scheduler  */
  
template <typename Scheduler,
          template <typename> typename Fiber=minimal_fiber,
          typename Stats=minimal_stats, typename Logging=minimal_logging,
          template <typename, typename> typename Elastic=minimal_elastic,
          typename Worker=minimal_worker,
          typename Interrupt=minimal_interrupt>
class chase_lev_work_stealing_scheduler {
public:

  using fiber_type = Fiber<Scheduler>;

  using deque_type = chase_lev_deque<fiber_type>;

  // warning: can overflow currently if size gets above 1k
  using buffer_type = ringbuffer<fiber_type*>; // later: make it resizeable?

  using elastic_type = Elastic<Stats, Logging>;

  static
  perworker::array<buffer_type> buffers;

  static
  perworker::array<deque_type> deques;

  static
  auto flush() -> fiber_type* {
    auto& my_buffer = buffers.mine();
    auto& my_deque = deques.mine();
    fiber_type* current = nullptr;
    if (my_buffer.empty()) {
      return nullptr;
    }
    current = my_buffer.back();
    my_buffer.pop_back();
    while (! my_buffer.empty()) {
      auto f = my_buffer.front();
      my_buffer.pop_front();
      my_deque.push(f);
    }
    assert(my_buffer.empty());
    return current;
  }

  static
  auto launch(size_t nb_steal_attempts=1) {
    using scheduler_status_type = enum scheduler_status_enum {
      scheduler_status_active,
      scheduler_status_finish
    };

    auto nb_workers = perworker::nb_workers();
    bool should_terminate = false;
    typename Worker::termination_detection_type termination_barrier;
    typename Worker::worker_exit_barrier worker_exit_barrier(nb_workers);
    perworker::array<size_t> nb_steal_attempts_so_far(0);

    auto random_other_worker = [&] (size_t nb_workers, size_t my_id) -> size_t {
      assert(nb_workers != 1);
      auto& nb_sa = nb_steal_attempts_so_far[my_id];
      auto id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
      if (id >= my_id) {
        id++;
      }
      nb_sa++;
      assert(id != my_id);
      assert(id >= 0 && id < nb_workers);
      return id;
    };

    auto acquire = [&] {
      if (nb_workers == 1) {
        termination_barrier.set_active(false);
        return scheduler_status_finish;
      }
      auto my_id = perworker::my_id();
      Logging::log_event(enter_wait);
      Stats::on_enter_acquire();
      termination_barrier.set_active(false);
      elastic_type::accept_lifelines();
      fiber_type *current = nullptr;
      while (current == nullptr) {
        assert(nb_steal_attempts >= 1);
        auto i = nb_steal_attempts;
        auto target = random_other_worker(nb_workers, my_id);
        do {
	  termination_barrier.set_active(true);
	  current = deques[target].steal();
	  if (current == nullptr) {
	    termination_barrier.set_active(false);
	  } else {
	    Stats::increment(Stats::configuration_type::nb_steals);
	    break;
	  }
          i--;
          target = random_other_worker(nb_workers, my_id);
        } while (i > 0);
        if (current == nullptr) {
          elastic_type::try_to_sleep(target);
        } else {
          elastic_type::wake_children();
        }
        if (termination_barrier.is_terminated() || should_terminate) {
          assert(current == nullptr);
          Logging::log_event(worker_exit);
          elastic_type::wake_children();
          Stats::on_exit_acquire();
          Logging::log_event(exit_wait);
          return scheduler_status_finish;
        }
      }
      assert(current != nullptr);
      buffers.mine().push_back(current);
      Stats::on_exit_acquire();
      Logging::log_event(exit_wait);
      return scheduler_status_active;
    };

    auto worker_loop = [&] (size_t my_id) {
      auto &my_deque = deques.mine();
      scheduler_status_type status = scheduler_status_active;
      fiber_type *current = nullptr;
      Stats::on_enter_work();
      while (status == scheduler_status_active) {
        current = flush();
        while ((current != nullptr) || !my_deque.empty()) {
          current = (current == nullptr) ? my_deque.pop() : current;
          if (current != nullptr) {
            auto s = current->exec();
            if (s == fiber_status_continue) {
              buffers.mine().push_back(current);
            } else if (s == fiber_status_pause) {
              // do nothing
            } else if (s == fiber_status_finish) {
              current->finish();
            } else {
              assert(s == fiber_status_exit_launch);
              current->finish();
              status = scheduler_status_finish;
              Logging::log_event(initiate_teardown);
              should_terminate = true;
            }
            current = flush();
          }
        }
        assert((current == nullptr) && my_deque.empty());
        Stats::on_exit_work();
        status = acquire();
        Stats::on_enter_work();
      }
      Stats::on_exit_work();
      Interrupt::wait_to_terminate_ping_thread();
      worker_exit_barrier.wait(my_id);
    };
    
    Worker::initialize(nb_workers);
    elastic_type::initialize();
    Interrupt::initialize_signal_handler();
    termination_barrier.set_active(true);
    for (size_t i = 1; i < nb_workers; i++) {
      Worker::launch_worker_thread(i, [&] (size_t i) {
        termination_barrier.set_active(true);
        worker_loop(i);
      });
    }
    Interrupt::launch_ping_thread(nb_workers);
    Worker::launch_worker_thread(0, [&] (size_t i) {
      worker_loop(i);
    });
    Worker::destroy();
#ifndef NDEBUG
    for (size_t i = 0; i < buffers.size(); i++) {
      assert(buffers[i].empty());
    }
    for (size_t i = 0; i < deques.size(); i++) {
      assert(deques[i].empty());
    }
#endif
  }

  static
  auto take() -> fiber_type* {
    auto& my_buffer = buffers.mine();
    auto& my_deque = deques.mine();
    fiber_type* current = nullptr;
    assert(my_buffer.empty());
    current = my_deque.pop();
    if (current != nullptr) {
      my_buffer.push_back(current);
    }
    return current;
  }
  
  static
  auto schedule(fiber_type* f) {
    assert(f->is_ready());
    buffers.mine().push_back(f);
  }

  static
  auto commit() {
    auto f = flush();
    if (f != nullptr) {
      deques.mine().push(f);
    }
  }

};

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          template <typename, typename> typename Elastic,
          typename Worker,
          typename Interrupt>
perworker::array<typename chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::buffer_type> 
chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::buffers;

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          template <typename, typename> typename Elastic,
          typename Worker,
          typename Interrupt>
perworker::array<typename chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::deque_type>
chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::deques;

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          template <typename, typename> typename Elastic,
          typename Worker,
          typename Interrupt>
Fiber<Scheduler>* take() {
  return chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::take();  
}

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          template <typename, typename> typename Elastic,
          typename Worker,
          typename Interrupt>
void schedule(Fiber<Scheduler>* f) {
  chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::schedule(f);  
}

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          template <typename, typename> typename Elastic,
          typename Worker,
          typename Interrupt>
void commit() {
  chase_lev_work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Elastic,Worker,Interrupt>::commit();
}
  
} // end namespace
