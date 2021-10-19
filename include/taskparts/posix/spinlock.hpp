#pragma once

#include <pthread.h>
#include <errno.h>

namespace taskparts {

/* A standard semaphore implemented using system semaphore support */
class spinlock {
  pthread_spinlock_t l;
public:
  spinlock()      { pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE); }
  void lock()     { pthread_spin_lock(&l); }
  bool try_lock() { return pthread_spin_trylock(&l) != EBUSY; }
  void unlock()   { pthread_spin_unlock(&l); }
  ~spinlock()     { pthread_spin_destroy(&l);}
};

} // end namespace
