#pragma once

#include <atomic>
#include <memory>
#include <assert.h>

#include "perworker.hpp"
#include "fixedcapacity.hpp"
#include "scheduler.hpp"
#include "hash.hpp"
#if defined(TASKPARTS_USE_CHASELEV_DEQUE)
#include "chaselev.hpp"
namespace taskparts {
template <typename Fiber>
using deque = chaselev<Fiber>;
}
#elif defined(TASKPARTS_USE_YWRA_DEQUE)
#include "ywra.hpp"
namespace taskparts {
template <typename Fiber>
using deque = ywra<Fiber>;
}
#else
#include "abp.hpp"
namespace taskparts {
template <typename Fiber>
using deque = abp<Fiber>;
}
#endif
#include "elastic.hpp"
#if defined(TASKPARTS_ELASTIC_SURPLUS)
namespace taskparts {
template <typename Stats, typename Logging>
using Elastic = elastic_surplus<Stats, Logging>;
}
#else
namespace taskparts {
template <typename Stats, typename Logging>
using Elastic = minimal_elastic<Stats, Logging>;
}
#endif

namespace taskparts {
  
/*---------------------------------------------------------------------*/
/* Work-stealing scheduler  */
  
template <typename Scheduler,
          template <typename> typename Fiber=minimal_fiber,
          typename Stats=minimal_stats, typename Logging=minimal_logging,
          typename Worker=minimal_worker,
          typename Interrupt=minimal_interrupt>
class work_stealing_scheduler {
public:

  using fiber_type = Fiber<Scheduler>;

  using deque_type = deque<fiber_type>;

  using buffer_type = ringbuffer<fiber_type*>;

  using elastic_type = Elastic<Stats, Logging>;

  static
  perworker::array<buffer_type> buffers;

  static
  perworker::array<deque_type> deques;
  
  static
  auto push(deque_type& d, fiber_type* f) {
    auto r = d.push(f);
    if (r == deque_surplus_up) {
      elastic_type::incr_surplus();
    }
  }
  
  static
  auto pop(size_t my_id) -> fiber_type* {
    auto& d = deques[my_id];
    auto r = d.pop();
    auto f = r.first;
    if (r.second == deque_surplus_down) {
      elastic_type::decr_surplus(my_id);
    }
    return f;
  }
  
  static
  auto steal(size_t target_id) -> fiber_type* {
    auto& d = deques[target_id];
    auto r = d.steal();
    auto f = r.first;
    if (r.second == deque_surplus_down) {
      elastic_type::decr_surplus(target_id);
    }
    return f;
  }

  static
  auto flush_buffer() {
    auto& my_buffer = buffers.mine();
    auto& my_deque = deques.mine();
    while (! my_buffer.empty()) {
      auto f = my_buffer.front();
      my_buffer.pop_front();
      push(my_deque, f);
    }
    assert(my_buffer.empty());
  }
  
  static
  auto flush() -> fiber_type* {
    auto& my_buffer = buffers.mine();
    fiber_type* current = nullptr;
    if (my_buffer.empty()) {
      return nullptr;
    }
    current = my_buffer.back();
    my_buffer.pop_back();
    flush_buffer();
    return current;
  }

  static
  auto launch() {
    using scheduler_status_type = enum scheduler_status_enum {
      scheduler_status_active,
      scheduler_status_finish
    };

    auto nb_workers = perworker::nb_workers();
    typename Worker::termination_detection_type termination_barrier;
    typename Worker::worker_exit_barrier worker_exit_barrier(nb_workers);
    size_t nb_steal_attempts = (perworker::nb_workers() * 8);
    if (const auto env_p = std::getenv("TASKPARTS_NB_STEAL_ATTEMPTS")) {
      nb_steal_attempts = std::stoi(env_p);
    }
    perworker::array<size_t> nb_steal_attempts_so_far(0);

    auto random_other_worker = [&] (size_t nb_workers, size_t my_id) -> size_t {
      assert(nb_workers != 1);
      auto& nb_sa = nb_steal_attempts_so_far[my_id];
      size_t id;
      if constexpr (elastic_type::override_rand_worker) {
        id = elastic_type::random_other_worker(nb_sa, nb_workers, my_id);
      } else {
        id = (size_t)((hash(my_id) + hash(nb_sa)) % (nb_workers - 1));
        if (id >= my_id) {
          id++;
        }
        nb_sa++;
      }
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
      elastic_type::incr_stealing(my_id);
      fiber_type* current = nullptr;
      while (current == nullptr) {
        assert(nb_steal_attempts >= 1);
        auto i = nb_steal_attempts;
        auto target = random_other_worker(nb_workers, my_id);
        do {
          termination_barrier.set_active(true);
          current = steal(target);
          if (current == nullptr) {
            termination_barrier.set_active(false);
          } else {
            Stats::increment(Stats::configuration_type::nb_steals);
            elastic_type::decr_stealing(my_id);
            break;
          }
          i--;
          target = random_other_worker(nb_workers, my_id);
        } while (i > 0);
        if (termination_barrier.is_terminated()) {
          assert(current == nullptr);
          auto t = new terminal_fiber<Scheduler>();
          t->incounter.store(0);
          current = t;
          elastic_type::decr_stealing(my_id);
          return scheduler_status_active;
        }
        if (current == nullptr) {
          elastic_type::try_to_sleep(target);
        }
      }
      assert(current != nullptr);
      schedule(current);
      Stats::on_exit_acquire();
      Logging::log_event(exit_wait);
      return scheduler_status_active;
    };

    auto worker_loop = [&] (size_t my_id) {
      auto& my_deque = deques.mine();
      scheduler_status_type status = scheduler_status_active;
      fiber_type* current = nullptr;
      Stats::on_enter_work();
      while (status == scheduler_status_active) {
        current = flush();
        while ((current != nullptr) || ! my_deque.empty()) {
          current = (current == nullptr) ? pop(my_id) : current;
          if (current != nullptr) {
            auto s = current->exec();
            if (s == fiber_status_continue) {
              schedule(current);
            } else if (s == fiber_status_pause) {
              // nothing to do
            } else if (s == fiber_status_finish) {
              current->finish();
            } else if (s == fiber_status_exit_worker) {
              current->finish();
              Logging::log_event(worker_exit);
              Stats::on_exit_acquire();
              Logging::log_event(exit_wait);
              status = scheduler_status_finish;
              flush_buffer();
              break;
            } else {
              assert(s == fiber_status_exit_launch);
              current->finish();
              status = scheduler_status_finish;
              Logging::log_event(initiate_teardown);
            }
            current = flush();
          }
        }
        if (status == scheduler_status_finish) {
          continue;
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
#ifndef NDEBUG /*
    for (size_t i = 0; i < buffers.size(); i++) {
      assert(buffers[i].empty());
    }
    for (size_t i = 0; i < deques.size(); i++) {
      assert(deques[i].empty());
      } */
#endif
  }

  static
  auto take() -> fiber_type* {
    auto my_id = perworker::my_id();
#ifndef NDEBUG
    auto& my_buffer = buffers[my_id];
    assert(my_buffer.empty());
#endif
    fiber_type* current = nullptr;
    current = pop(my_id);
    if (current != nullptr) {
      schedule(current);
    }
    return current;
  }
  
  static
  auto schedule(fiber_type* f) {
    assert(f->is_ready());
    auto& my_buffer = buffers.mine();
    my_buffer.push_back(f);
    if (my_buffer.full()) {
      auto f2 = flush();
      assert(f == f2);
      my_buffer.push_back(f);
    }
  }

  static
  auto commit() {
    auto f = flush();
    if (f != nullptr) {
      push(deques.mine(), f);
    }
  }

};

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          typename Worker,
          typename Interrupt>
perworker::array<typename work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::buffer_type> 
work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::buffers;

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          typename Worker,
          typename Interrupt>
perworker::array<typename work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::deque_type>
work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::deques;

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          typename Worker,
          typename Interrupt>
Fiber<Scheduler>* take() {
  return work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::take();  
}

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          typename Worker,
          typename Interrupt>
void schedule(Fiber<Scheduler>* f) {
  work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::schedule(f);  
}

template <typename Scheduler,
          template <typename> typename Fiber,
          typename Stats, typename Logging,
          typename Worker,
          typename Interrupt>
void commit() {
  work_stealing_scheduler<Scheduler,Fiber,Stats,Logging,Worker,Interrupt>::commit();
}
  
} // end namespace
