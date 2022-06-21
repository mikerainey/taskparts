#pragma once

#include <stdarg.h>
#include <pthread.h>
#include <cstdio>
#include <assert.h>

namespace taskparts {

namespace perworker {
static
auto my_id() -> size_t;
} // end namespace
  
static
pthread_mutex_t print_lock;

static
__attribute__((constructor))
void init_print_lock() {
  auto r = pthread_mutex_init(&print_lock, nullptr);
  assert(r == 0);
}

static
__attribute__((destructor))
void destroy_print_lock() {
  auto r = pthread_mutex_destroy(&print_lock);
  assert(r == 0);
}

static
void acquire_print_lock() {
  pthread_mutex_lock(&print_lock);
}

static
void release_print_lock() {
  pthread_mutex_unlock(&print_lock);
}

static
void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  acquire_print_lock(); {
    fprintf(stderr, "Fatal error -- ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
  }
  release_print_lock();
  va_end(ap);
  assert(false);
  exit(-1);
}

static
void afprintf(FILE* stream, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  acquire_print_lock(); {
    vfprintf(stream, fmt, ap);
    fflush(stream);
  }
  release_print_lock();
  va_end(ap);
}

static
void aprintf(const char *fmt, ...) {
  acquire_print_lock(); {
    fprintf(stdout, "[%lu] ", perworker::my_id());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
    va_end(ap);
  }
  release_print_lock();
}
  
} // end namespace
  
#define taskparts_die(f_, ...) \
  taskparts::die((f_), ##__VA_ARGS__);

#define taskparts_printf(f_, ...) \
  taskparts::aprintf((f_), ##__VA_ARGS__);
