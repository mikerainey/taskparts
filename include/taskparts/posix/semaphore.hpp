#pragma once

#include <semaphore.h>

namespace taskparts {

/* A standard semaphore implemented using system semaphore support */
class semaphore {
  sem_t sem;
public:
  semaphore()  { sem_init(&sem, 0, 0); }
  void post()  { sem_post(&sem); }
  void wait()  { sem_wait(&sem); }
  ~semaphore() { sem_destroy(&sem);}
};

} // end namespace
