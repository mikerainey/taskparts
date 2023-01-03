#pragma once

#include <thread>
#include <condition_variable>

#include "../machine.hpp"

namespace taskparts {

template <typename Body>
auto minimal_launch_worker_thread(size_t id, const Body& b) {
  if (id == 0) {
    b();
    return;
  }
  auto t = std::thread(b);
  t.detach();
}

auto minimal_worker_initialize(size_t) { }
  
auto minimal_worker_destroy() { }

class minimal_worker_exit_barrier {
private:

  size_t nb_workers;
  size_t nb_workers_exited = 0;
  std::mutex exit_lock;
  std::condition_variable exit_condition_variable;

public:

  minimal_worker_exit_barrier(size_t nb_workers) : nb_workers(nb_workers) { }

  void wait(size_t my_id)  {
    std::unique_lock<std::mutex> lk(exit_lock);
    auto nb = ++nb_workers_exited;
    if (my_id == 0) {
      exit_condition_variable.wait(
	lk, [&] { return nb_workers_exited == nb_workers; });
    } else if (nb == nb_workers) {
      exit_condition_variable.notify_one();
    }
  }

};

auto worker_yield() {
#if defined(TASKPARTS_MULTIPROGRAMMED) || defined(TASKPARTS_ELASTIC_S3)
  std::this_thread::yield();
#else
  cycles::spin_for(1000);
#endif
}

} // end namespace
