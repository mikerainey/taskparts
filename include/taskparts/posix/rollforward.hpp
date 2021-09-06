#pragma once

#include <sys/signal.h>

using register_type = greg_t;

template <class T>
void try_to_initiate_rollforward(const T& t, register_type* rip);

void heartbeat_interrupt_handler(int, siginfo_t*, void* uap) {
  //  stats::increment(taskparts::stats_configuration::nb_heartbeats);
  mcontext_t* mctx = &((ucontext_t *)uap)->uc_mcontext;
  register_type* rip = &mctx->gregs[16];
  try_to_initiate_rollforward(rollforward_table, rip);
}
