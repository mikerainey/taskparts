#pragma once

#include <stdarg.h>
#include <pthread.h>
#include <cstdio>

namespace taskparts {
  
pthread_mutex_t print_lock;
  
void init_print_lock() {
  pthread_mutex_init(&print_lock, nullptr);
}

void acquire_print_lock() {
  pthread_mutex_lock (&print_lock);
}

void release_print_lock() {
  pthread_mutex_unlock (&print_lock);
}

void die(const char *fmt, ...) {
  va_list	ap;
  va_start (ap, fmt);
  acquire_print_lock(); {
    fprintf (stderr, "Fatal error -- ");
    vfprintf (stderr, fmt, ap);
    fprintf (stderr, "\n");
    fflush (stderr);
  }
  release_print_lock();
  va_end(ap);
  assert(false);
  exit (-1);
}

void afprintf(FILE* stream, const char *fmt, ...) {
  va_list	ap;
  va_start (ap, fmt);
  acquire_print_lock(); {
    vfprintf (stream, fmt, ap);
    fflush (stream);
  }
  release_print_lock();
  va_end(ap);
}

void aprintf(const char *fmt, ...) {
  va_list	ap;
  va_start (ap, fmt);
  acquire_print_lock(); {
    vfprintf (stdout, fmt, ap);
    fflush (stdout);
  }
  release_print_lock();
  va_end(ap);
}
  
} // end namespace
  
#define taskparts_die(f_, ...) \
  taskparts::die((f_), ##__VA_ARGS__);

#define taskparts_printf(f_, ...) \
  taskparts::aprintf((f_), ##__VA_ARGS__);
