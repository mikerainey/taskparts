#pragma once

#include <sys/signal.h>
#include <cassert>>

namespace taskparts {

void heartbeat_interrupt_handler(int, siginfo_t*, void* uap) {
#if defined(TASKPARTS_TPALRTS)
  //  stats::increment(taskparts::stats_configuration::nb_heartbeats);
  mcontext_t* mctx = &((ucontext_t *)uap)->uc_mcontext;
  void** rip = (void**)&mctx->gregs[16];
  try_to_initiate_rollforward(rip);
#else
  assert(false);
#endif
}

} // end namespace
