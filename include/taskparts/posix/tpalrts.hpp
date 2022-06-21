#pragma once

#include <thread>
#include <condition_variable>
#include <sys/timerfd.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <cstring>

#include "../scheduler.hpp"
#include "../perworker.hpp"
#include "../rollforward.hpp"
#include "../diagnostics.hpp"

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Interrupt-based worker-launch routine */

static
perworker::array<pthread_t> pthreads;

static
ping_thread_status_type ping_thread_status = ping_thread_status_active;

static
std::mutex ping_thread_lock;

static
std::condition_variable ping_thread_condition_variable;

template <typename Body,
	  typename Initialize_worker,
	  typename Destroy_worker>
void launch_interrupt_worker_thread(size_t id,
				    const Body& b,
				    const Initialize_worker& initialize_worker,
				    const Destroy_worker& destroy_worker) {
  // we need this exact initializer list to avoid a segfault
  auto b2 = [id, b, initialize_worker, destroy_worker] {
    perworker::id::initialize_worker(id);
    pin_calling_worker();
    initialize_worker();
    b(id);
    destroy_worker();
  };
  if (id == 0) {
    b2();
    return;
  }
  auto t = std::thread(b2);
  pthreads[id] = t.native_handle();
  t.detach();
}

auto dflt_signal_workers(size_t nb_workers) {
  for (size_t i = 0; i < nb_workers; ++i) {
    pthread_kill(pthreads[i], SIGUSR1);
  }
}

auto initialize_signal_handler0() {
  pthreads[0] = pthread_self();
}

template <typename Signal_workers = decltype(dflt_signal_workers)>
auto launch_ping_thread0(size_t nb_workers,
			 Signal_workers signal_workers = dflt_signal_workers) {
  auto kappa_usec = get_kappa_usec();
  auto pthreadsp = &pthreads;
  auto statusp = &ping_thread_status;
  auto ping_thread_lockp = &ping_thread_lock;
  auto cvp = &ping_thread_condition_variable;
  auto t = std::thread([=] {
    perworker::array<pthread_t>& pthreads = *pthreadsp;
    ping_thread_status_type& status = *statusp;
    std::mutex& ping_thread_lock = *ping_thread_lockp;
    std::condition_variable& cv = *cvp;
    unsigned int ns;
    unsigned int sec;
    struct itimerspec itval;
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1) {
      taskparts_die("failed to properly initialize timerfd");
    }
    {
      auto one_million = 1000000;
      sec = kappa_usec / one_million;
      ns = (kappa_usec - (sec * one_million)) * 1000;
      itval.it_interval.tv_sec = sec;
      itval.it_interval.tv_nsec = ns;
      itval.it_value.tv_sec = sec;
      itval.it_value.tv_nsec = ns;
    }
    timerfd_settime(timerfd, 0, &itval, nullptr);
    while (status == ping_thread_status_active) {
      unsigned long long missed;
      int ret = read(timerfd, &missed, sizeof(missed));
      if (ret == -1) {
	taskparts_die("read timer");
	return;
      }
      signal_workers(nb_workers);
    }
    std::unique_lock<std::mutex> lk(ping_thread_lock);
    status = ping_thread_status_exited;
    cv.notify_all();
  });
  t.detach();
}

static
auto wait_to_terminate_ping_thread0() {
  std::unique_lock<std::mutex> lk(ping_thread_lock);
  if (ping_thread_status == ping_thread_status_active) {
    ping_thread_status = ping_thread_status_exit_launch;
  }
  auto f = [&] {
    return ping_thread_status == ping_thread_status_exited;
  };
  ping_thread_condition_variable.wait(lk, f);
}
  
/*---------------------------------------------------------------------*/
/* Ping-thread scheduler configuration */

class ping_thread_worker {
public:

  static
  void initialize(size_t nb_workers) { }

  static
  void destroy() { }

  static
  void initialize_worker() {
    sigset_t mask, prev_mask;
    if (pthread_sigmask(SIG_SETMASK, NULL, &prev_mask)) {
      exit(0);
    }
    struct sigaction sa, prev_sa;
    sa.sa_sigaction = heartbeat_interrupt_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_mask = prev_mask;
    sigdelset(&sa.sa_mask, SIGUSR1);
    if (sigaction(SIGUSR1, &sa, &prev_sa)) {
      exit(0);
    }
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
  }

  template <typename Body>
  static
  void launch_worker_thread(size_t id, const Body& b) {
    launch_interrupt_worker_thread(id, b, [] { initialize_worker(); }, [] { });
  }
  
  using worker_exit_barrier = typename minimal_worker::worker_exit_barrier;
  
  using termination_detection_type = minimal_termination_detection;

};

class ping_thread_interrupt {
public:

  static
  void initialize_signal_handler() {
    initialize_signal_handler0();
  }
  
  static
  void wait_to_terminate_ping_thread() {
    wait_to_terminate_ping_thread0();
  }
  
  static
  void launch_ping_thread(size_t nb_workers) {
    launch_ping_thread0(nb_workers);
  }
  
};

/*---------------------------------------------------------------------*/
/* Interrupt/polling hybrid configuration */

static
perworker::array<std::atomic_bool> heartbeat_flags;

auto hardware_alarm_signal_workers(size_t nb_workers) {
  for (size_t i = 0; i < nb_workers; ++i) {
    heartbeat_flags[i].store(true);
  }
}
  
class hardware_alarm_polling_worker {
public:

  static
  void initialize(size_t nb_workers) { }

  static
  void destroy() { }

  static
  void initialize_worker() { }

  template <typename Body>
  static
  void launch_worker_thread(size_t id, const Body& b) {
    launch_interrupt_worker_thread(id, b, [] { }, [] { });
  }
  
  using worker_exit_barrier = typename minimal_worker::worker_exit_barrier;
  
  using termination_detection_type = minimal_termination_detection;

};

class hardware_alarm_polling_interrupt {
public:

  static
  void initialize_signal_handler() {
    initialize_signal_handler0();
  }
  
  static
  void wait_to_terminate_ping_thread() {
    wait_to_terminate_ping_thread0();
  }

  static
  void launch_ping_thread(size_t nb_workers) {
    launch_ping_thread0(nb_workers, [&] (size_t nb_workers) {
      for (size_t i = 0; i < nb_workers; ++i) {
	heartbeat_flags[i].store(true);
      }
    });
  }

  static
  auto my_heartbeat_flag() -> std::atomic_bool* {
    return &(heartbeat_flags.mine());
  }
  
};

/*---------------------------------------------------------------------*/
/* Pthread-direct interrupt configuration (Linux specific) */

std::mutex pthread_init_mutex;
  
class pthread_direct_worker {
public:

  static
  void initialize(size_t nb_workers) { }

  static
  void destroy() { }

  static
  perworker::array<timer_t> timerid;
  static
  perworker::array<struct sigevent> sev;
  static
  perworker::array<struct itimerspec> itval;

  static
  void initialize_worker() {
    auto kappa_usec = get_kappa_usec();
    std::lock_guard<std::mutex> guard(pthread_init_mutex);
    unsigned int ns;
    unsigned int sec;
    auto& my_sev = sev.mine();
    auto& my_itval = itval.mine();
    my_sev.sigev_notify = SIGEV_THREAD_ID;
    my_sev._sigev_un._tid = syscall(SYS_gettid);
    my_sev.sigev_signo = SIGUSR1; 
    my_sev.sigev_value.sival_ptr = &timerid.mine();
    {
      auto one_million = 1000000;
      sec = kappa_usec / one_million;
      ns = (kappa_usec - (sec * one_million)) * 1000;
      my_itval.it_interval.tv_sec = sec;
      my_itval.it_interval.tv_nsec = ns;
      my_itval.it_value.tv_sec = sec;
      my_itval.it_value.tv_nsec = ns;
    }
    if (timer_create(CLOCK_REALTIME, &my_sev, &timerid.mine()) == -1) {
      printf("timer_create failed: %d: %s\n", errno, strerror(errno));
    }
    timer_settime(timerid.mine(), 0, &my_itval, NULL);
  }
  
  template <typename Body>
  static
  void launch_worker_thread(size_t id, const Body& b) {
    launch_interrupt_worker_thread(id, b,
				   [] { initialize_worker(); },
				   [] { timer_delete(timerid.mine()); });
  }

  using worker_exit_barrier = typename minimal_worker::worker_exit_barrier;
  
  using termination_detection_type = minimal_termination_detection;

};

perworker::array<timer_t> pthread_direct_worker::timerid;
perworker::array<struct sigevent> pthread_direct_worker::sev;
perworker::array<struct itimerspec> pthread_direct_worker::itval;
  
class pthread_direct_interrupt {
public:

  static
  sigset_t mask;

  static
  sigset_t prev_mask;

  static
  struct sigaction sa;

  static
  struct sigaction prev_sa;

  static
  void initialize_signal_handler() {
    if (pthread_sigmask(SIG_SETMASK, NULL, &prev_mask)) {
      exit(0);
    }
    sa.sa_sigaction = heartbeat_interrupt_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_mask = prev_mask;
    sigdelset(&sa.sa_mask, SIGUSR1);
    if (sigaction(SIGUSR1, &sa, &prev_sa)) {
      exit(0);
    }
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);

    return;
    
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = taskparts::heartbeat_interrupt_handler;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGUSR1, &act, NULL);
  }

  static
  void wait_to_terminate_ping_thread() { }
  
  static
  void launch_ping_thread(size_t nb_workers) { }
  
};

sigset_t pthread_direct_interrupt::mask;
sigset_t pthread_direct_interrupt::prev_mask;
struct sigaction pthread_direct_interrupt::sa;
struct sigaction pthread_direct_interrupt::prev_sa;

/*---------------------------------------------------------------------*/
/* PAPI interrupt configuration */

#ifdef TASKPARTS_USE_PAPI
#include <papi.h>

static
void papi_interrupt_handler(int, void*, long long, void *context) {
  heartbeat_interrupt_handler(0, nullptr, context);
}

std::mutex papi_init_mutex;

std::mutex papi_destroy_mutex;

// guide: https://icl.cs.utk.edu/papi/docs/dc/d7e/examples_2overflow__pthreads_8c_source.html

class papi_worker {
public:

  static
  perworker::array<int> event_set;

  static
  void initialize(size_t nb_workers) { }

  static
  void destroy() { }

  static
  void initialize_worker() {
    int retval;
    auto kappa_cycles = get_kappa_cycles();
    std::lock_guard<std::mutex> guard(papi_init_mutex);
    if ( (retval = PAPI_create_eventset(&(event_set.mine()))) != PAPI_OK) {
      taskparts_die("papi worker initialization failed");
    }
    if ((retval=PAPI_query_event(PAPI_TOT_CYC)) != PAPI_OK) {
      taskparts_die("papi worker initialization failed");
    }
    if ( (retval = PAPI_add_event(event_set.mine(), PAPI_TOT_CYC)) != PAPI_OK) {
      taskparts_die("papi worker initialization failed");
    }
    if((retval = PAPI_overflow(event_set.mine(), PAPI_TOT_CYC, kappa_cycles, 0, papi_interrupt_handler)) != PAPI_OK) {
      taskparts_die("papi worker initialization failed");
    }
    if((retval = PAPI_start(event_set.mine())) != PAPI_OK) {
      taskparts_die("papi worker initialization failed");
    }
  }

  static
  void destroy_worker() {
    int retval;
    long long values[2];
    std::lock_guard<std::mutex> guard(papi_init_mutex);
    if ((retval = PAPI_stop(event_set.mine(), values))!=PAPI_OK) {
      taskparts_die("papi worker stop failed");
    }
    retval = PAPI_overflow(event_set.mine(), PAPI_TOT_CYC, 0, 0, papi_interrupt_handler);
    if(retval !=PAPI_OK) {
      taskparts_die("papi worker deinitialization1 failed");
    }
    retval = PAPI_remove_event(event_set.mine(), PAPI_TOT_CYC);
    if (retval != PAPI_OK) {
      taskparts_die("papi worker deinitialization failed");
    }
  }

  template <typename Body>
  static
  void launch_worker_thread(size_t id, const Body& b) {
    launch_interrupt_worker_thread(id, b,
				   [] { initialize_worker(); },
				   [] { destroy_worker(); });
  }

  using worker_exit_barrier = typename minimal_worker::worker_exit_barrier;
  
  using termination_detection_type = minimal_termination_detection;

};

perworker::array<int> papi_worker::event_set;

#endif

/*---------------------------------------------------------------------*/
/* Linux kernel module heartbeat interrupts */

// kernel module available at git@github.com:nickwanninger/heartbeat-linux.git
  
#ifdef TASKPARTS_TPALRTS_HBTIMER_KMOD

extern "C" {
#include <heartbeat.h>
}

class hbtimer_kmod_worker {
public:

  static
  auto initialize(size_t nb_workers) { }

  static
  auto destroy() { }
  
  template <typename Body>
  static
  auto launch_worker_thread(size_t id, const Body& b) {
    launch_interrupt_worker_thread(id, b,
				   [=] {
				     hb_init();
				     hbtimer_init_tbl();
				     hb_repeat(get_kappa_usec(), nullptr);
				   },
				   [] { hb_exit(); });
  }

  using worker_exit_barrier = minimal_worker_exit_barrier;
  
  using termination_detection_type = minimal_termination_detection;

};

#endif

/*---------------------------------------------------------------------*/
/* Selection of global heartbeat timer mechanism, disabled by default */

#if defined(TASKPARTS_TPALRTS_HARDWARE_ALARM_POLLING)
using tpalrts_worker = hardware_alarm_polling_worker;
using tpalrts_interrupt = hardware_alarm_polling_interrupt;
#elif defined(TASKPARTS_TPALRTS_PTHREAD_DIRECT)
using tpalrts_worker = pthread_direct_worker;
using tpalrts_interrupt = pthread_direct_interrupt;
#elif defined(TASKPARTS_TPALRTS_PAPI) && defined(TASKPARTS_USE_PAPI)
using tpalrts_worker = papi_worker;
using tpalrts_interrupt = papi_interrupt;
#elif defined(TASKPARTS_TPALRTS_MINIMAL)
using tpalrts_worker = minimal_worker;
using tpalrts_interrupt = minimal_interrupt;
#elif defined(TASKPARTS_TPALRTS_HBTIMER_KMOD)
using tpalrts_worker = hbtimer_kmod_worker;
using tpalrts_interrupt = minimal_interrupt;  
#else
using tpalrts_worker = ping_thread_worker;
using tpalrts_interrupt = ping_thread_interrupt;
#endif
  
} // end namespace
