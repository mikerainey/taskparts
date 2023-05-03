#include <taskparts/taskparts.hpp>

#include <cstdlib>
#include <memory>
#include <new>
#include <iostream>
#include <cstdarg>
#include <string>
#include <sys/resource.h>
#if defined(TASKPARTS_DARWIN)
#include <mach/mach_time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <deque>
#include <vector>
#include <unordered_map>
#include <array>

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <semaphore.h> // to be replaced in C++20
#ifdef TASKPARTS_USE_HWLOC
#include <hwloc.h>    
#endif    

/* Taskparts compiler flags:
 *   Platform (required):
 *   - TASKPARTS_POSIX boolean
 *   - TASKPARTS_DARWIN boolean
 *   Architecture (required):
 *   - TASKPARTS_ARM64 boolean
 *   - TASKPARTS_X64 boolean
 *   CPU pinning:
 *   - TASKPARTS_USE_HWLOC boolean (defaultly, false)
 *   Elastic scheduling
 *   - TASKPARTS_DISABLE_ELASTIC boolean (defaultly, false)
 *   Instrumentation:
 *   - TASKPARTS_STATS boolean (defaultly, false)
 *   - TASKPARTS_LOGGING boolean (defaultly, false)
 *   Diagnostics:
 *   - TASKPARTS_META_SCHEDULER_SERIAL_RANDOM boolean (defaultly, false)
 *   - TASKPARTS_USE_VALGRIND boolean (defaultly, false)
 *   - TASKPARTS_RUN_UNIT_TESTS boolean (defaultly, false)
 */

/* FIXMEs
 * - The "variable workload" experiment randomly crashes in XCode on my Mac Air M2.
 */

/* TODOs
 * - Get benchmarking to work with logging.
 * - Find a way to make CPU frequency detection either succeed always or be optional.
 */

#if !defined(TASKPARTS_POSIX) && !defined(TASKPARTS_DARWIN)
#error "need to specify platform for taskparts"
#endif
#if !defined(TASKPARTS_ARM64) && !defined(TASKPARTS_X64)
#error "need to specify CPU architecture for taskparts"
#endif

namespace taskparts {

inline
uint64_t hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

/*---------------------------------------------------------------------*/
/* Diagnostics */

std::mutex print_mutex;

static
void afprintf(FILE* stream, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  { std::unique_lock<std::mutex> lk(print_mutex);
    vfprintf(stream, fmt, ap);
    fflush(stream);
  }
  va_end(ap);
}

auto get_my_id() -> size_t;

static
void aprintf(const char *fmt, ...) {
  { std::unique_lock<std::mutex> lk(print_mutex);
    fprintf(stdout, "[%lu] ", get_my_id());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
    va_end(ap);
  }
}

static
void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  { std::unique_lock<std::mutex> lk(print_mutex);
    fprintf(stderr, "Fatal error -- ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
  }
  va_end(ap);
  assert(false);
  exit(-1);
}

/*---------------------------------------------------------------------*/
/* Environment variables */

std::unordered_map<std::string, std::tuple<std::string, std::string>> environment_variables;

template <typename T = int>
class environment_variable {
public:
  T x;
  environment_variable(std::string s,
                       std::function<T()> dflt,
                       std::string description = "",
                       std::function<T(const char*)> parse = [] (const char* s) { return std::stoi(s); },
                       std::function<std::string(T)> to_string = [] (T x) { return std::to_string(x); }) {
    if (const auto env_p = std::getenv(s.c_str())) {
      x = parse(env_p);
    } else {
      x = dflt();
    }
    if (environment_variables.find(s) != environment_variables.end()) {
      die("duplicate environment variable %s", s.c_str());
    }
    environment_variables.insert(std::make_pair(s, std::make_tuple(description, to_string(x))));
  }
  auto get() -> T {
    return x;
  }
};

auto print_taskparts_help_message() -> void {
  bool should_print = false;
  bool should_exit = false;
  if (const auto env_p = std::getenv("TASKPARTS_HELP")) {
    should_print = true;
    should_exit = std::stoi(env_p);
  }
  if (! should_print) {
    return;
  }
  printf("Environment variables used by taskparts:\n");
  for (auto& d : environment_variables) {
    printf("\t%s = %s:\t%s\n",d.first.c_str(), std::get<1>(d.second).c_str(), std::get<0>(d.second).c_str());
  }
  if (should_exit) {
    exit(0);
  }
}

auto report_taskparts_configuration() -> void {
  std::string config_outfile = "";
  if (const auto env_p = std::getenv("TASKPARTS_CONFIG_OUTFILE")) {
    config_outfile = std::string(env_p);
  }
  if (config_outfile == "") {
    return;
  }
  FILE* f = (config_outfile == "stdout") ? stdout : fopen(config_outfile.c_str(), "w");
  fprintf(f, "{\n");
  auto n = environment_variables.size();
  for (auto& d : environment_variables) {
    fprintf(f, "\"%s\": %s", d.first.c_str(), std::get<1>(d.second).c_str());
    if (--n > 0) {
      fprintf(f, ",");
    }
    fprintf(f, "\n");
  }
  fprintf(f, "}\n");
  if (f != stdout) {
    fclose(f);
  }
}

auto in_quotes(std::string s) -> std::string {
  return "\"" + s + "\"";
}

/*---------------------------------------------------------------------*/
/* CPU frequency */

auto detect_cpu_frequency_khz() -> uint64_t {
  unsigned long cpu_frequency_khz = 0;
#if defined(TASKPARTS_DARWIN)
  die("detection of cpu frequency not yet supported on darwin");
#endif
  FILE *f;
  f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/base_frequency", "r");
  if (f == nullptr) {
    f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/bios_limit", "r");
  }
  if (f != nullptr) {
    char buf[1024];
    while (fgets(buf, sizeof(buf), f) != 0) {
      sscanf(buf, "%lu", &(cpu_frequency_khz));
    }
    fclose(f);
  }
  return (uint64_t)cpu_frequency_khz;
}

environment_variable<uint64_t> cpu_frequency_khz("TASKPARTS_CPU_BASE_FREQUENCY_KHZ",
                                                 [] { return detect_cpu_frequency_khz(); },
                                                 "cpu frequency in kiloherz ");

/*---------------------------------------------------------------------*/
/* Cycle counter */

static inline
auto cyclecounter() -> uint64_t {
#if defined(TASKPARTS_X64) && defined(TASKPARTS_POSIX)
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return  ((uint64_t) lo) | (((uint64_t) hi) << 32);
#elif defined(TASKPARTS_DARWIN)
  return mach_absolute_time() * 100;
#endif
}

static inline
auto nanoseconds_of(uint64_t cycles) -> uint64_t {
  return (1000000l * cycles) / cpu_frequency_khz.get();
}

static inline
auto diff(uint64_t start, uint64_t finish) -> uint64_t {
  return finish - start;
}

static inline
auto since(uint64_t start) -> uint64_t {
  return diff(start, cyclecounter());
}

static inline
auto seconds_of_nanoseconds(uint64_t ns) -> double {
  return (double)ns / 1.0e9;
}

static inline
auto seconds_of_cycles(uint64_t cycles) -> double {
  return seconds_of_nanoseconds(nanoseconds_of(cycles));
}

static inline
auto busywait_pause() {
#if defined(TASKPARTS_X64) && ! defined(TASKPARTS_OVERRIDE_PAUSE_INSTR)
  //_mm_pause();
  __builtin_ia32_pause();
#elif defined(TASKPARTS_ARM64)
  //__builtin_arm_yield();
#else
#error need to declare platform (e.g., TASKPARTS_X64)
#endif
}

static inline
auto wait(uint64_t n) {
  const uint64_t start = cyclecounter();
  while (cyclecounter() < (start + n)) {
    busywait_pause();
  }
}

static inline
auto spin_for(uint64_t nb_cycles) {
  wait(nb_cycles);
}

/*---------------------------------------------------------------------*/
/* Semaphore */

class semaphore {
  sem_t sem;
public:
  semaphore()  { sem_init(&sem, 0, 0); }
  ~semaphore() { sem_destroy(&sem);}
  void post()  { sem_post(&sem); }
  void wait()  { sem_wait(&sem); }
};

class spinning_semaphore { // for performance-debugging purposes
public:
  std::atomic<int32_t> count;
  spinning_semaphore() : count(0) { }
  auto post() {
    count++;
  }
  auto wait() {
    assert(count.load() >= 0);
    auto c = --count;
    do {
      if (c >= 0) {
        break;
      }
      spin_for(2000);
      c = count.load();
    } while (true);
    assert(count.load() >= 0);
  }
};

#ifndef TASKPARTS_SPINNING_SEMAPHORE
using default_semaphore = semaphore;
#else
using default_semaphore = spinning_semaphore;
#endif

/*---------------------------------------------------------------------*/
/* Compare-and-exchange instruction */

template <typename T, typename Update>
static
auto update_atomic(std::atomic<T>& c, const Update& u,
                   size_t my_id) -> T {
#ifndef TASKPARTS_ELASTIC_OVERRIDE_ADAPTIVE_BACKOFF
  while (true) {
    auto v = c.load();
    auto orig = v;
    auto next = u(orig);
    if (c.compare_exchange_strong(orig, next)) {
      return next;
    }
  }
#else
  // We mitigate contention on the atomic cell by using Adaptive
  // Feedback (as proposed in Ben-David's dissertation)
  uint64_t max_delay = 1;
  while (true) {
    auto v = c.load();
    auto orig = v;
    auto next = u(orig);
    if (c.compare_exchange_strong(orig, next)) {
      return next;
    }
    max_delay *= 2;
    spin_for(hash(my_id + max_delay) % max_delay);
  }
#endif
}

/*---------------------------------------------------------------------*/
/* ARM64 CPU context */

#if defined(TASKPARTS_ARM64)
extern "C"
void* context_save(char*);
__asm__
(
 "_context_save:\n"
 "    str x19, [x0, 0]\n"
 "    str x20, [x0, 8]\n"
 "    str x21, [x0, 16]\n"
 "    str x22, [x0, 24]\n"
 "    str x23, [x0, 32]\n"
 "    str x24, [x0, 40]\n"
 "    str x25, [x0, 48]\n"
 "    str x26, [x0, 56]\n"
 "    str x27, [x0, 64]\n"
 "    str x28, [x0, 72]\n"
 "    str fp, [x0, 80]\n"
 "    str lr, [x0, 88]\n"
 "    mov x1, sp\n"
 "    str x1, [x0, 96]\n"
 "    str d8, [x0, 104]\n"
 "    str d9, [x0, 112]\n"
 "    str d10, [x0, 120]\n"
 "    str d11, [x0, 128]\n"
 "    str d12, [x0, 136]\n"
 "    str d13, [x0, 144]\n"
 "    str d14, [x0, 152]\n"
 "    str d15, [x0, 160]\n"
 "    mov x0, #0\n"
 "    ret\n"
 );
extern "C"
void context_restore(char* ctx, void* t);
__asm__
(
 "_context_restore:\n"
 "    ldr d15, [x0, 160]\n"
 "    ldr d14, [x0, 152]\n"
 "    ldr d13, [x0, 144]\n"
 "    ldr d12, [x0, 136]\n"
 "    ldr d11, [x0, 128]\n"
 "    ldr d10, [x0, 120]\n"
 "    ldr d9, [x0, 112]\n"
 "    ldr d8, [x0, 104]\n"
 "    ldr x2, [x0, 96]\n"
 "    mov sp, x2\n"
 "    ldr lr, [x0, 88]\n"
 "    ldr fp, [x0, 80]\n"
 "    ldr x28, [x0, 72]\n"
 "    ldr x27, [x0, 64]\n"
 "    ldr x26, [x0, 56]\n"
 "    ldr x25, [x0, 48]\n"
 "    ldr x24, [x0, 40]\n"
 "    ldr x23, [x0, 32]\n"
 "    ldr x22, [x0, 24]\n"
 "    ldr x21, [x0, 16]\n"
 "    ldr x20, [x0, 8]\n"
 "    ldr x19, [x0, 0]\n"
 "    mov x2, #1\n"
 "    cmp x1, #0\n"
 "    csel x0, x2, x1, eq\n"
 "    ret\n"
 );
#endif

/*---------------------------------------------------------------------*/
/* X64 CPU context */

#if defined(TASKPARTS_X64)
extern "C"
void* context_save(char*);
asm(R"(
.globl context_save
        .type context_save, @function
        .align 16
context_save:
        .cfi_startproc
        movq %rbx, 0(%rdi)
        movq %rbp, 8(%rdi)
        movq %r12, 16(%rdi)
        movq %r13, 24(%rdi)
        movq %r14, 32(%rdi)
        movq %r15, 40(%rdi)
        leaq 8(%rsp), %rdx
        movq %rdx, 48(%rdi)
        movq (%rsp), %rax
        movq %rax, 56(%rdi)
        xorq %rax, %rax
        ret
        .size context_save, .-context_save
        .cfi_endproc
)");
extern "C"
void context_restore(char* ctx, void* t);
asm(R"(
.globl context_restore
        .type context_restore, @function
        .align 16
context_restore:
        .cfi_startproc
        movq 0(%rdi), %rbx
        movq 8(%rdi), %rbp
        movq 16(%rdi), %r12
        movq 24(%rdi), %r13
        movq 32(%rdi), %r14
        movq 40(%rdi), %r15
        test %rsi, %rsi
        mov $01, %rax
        cmove %rax, %rsi
        mov %rsi, %rax
        movq 56(%rdi), %rdx
        movq 48(%rdi), %rsp
        jmpq *%rdx
        .size context_restore, .-context_restore
        .cfi_endproc
)");
#endif

#if defined(TASKPARTS_ARM64)

auto new_continuation(native_continuation& c, thunk f) -> void* {
  static constexpr
  size_t arm64_stack_alignb = 16;
  static constexpr
  size_t arm64_stackszb = arm64_stack_alignb * (1<<12);
  static constexpr
  int arm64_sp_offsetb = 12;
  c.f = f;
  native_continuation* cp;
  if ((cp = (native_continuation*)context_save(&c.gprs[0]))) {
    cp->f(); // only cp is for sure live at this point
    return nullptr;
  }
  c.action = continuation_finish;
  char* stack = (char*)std::malloc(arm64_stackszb);
  char* stack_end = &stack[arm64_stackszb];
  stack_end -= (size_t)stack_end % arm64_stack_alignb;
  void** _ctx = (void**)&c.gprs[0];
  _ctx[arm64_sp_offsetb] = stack_end;
  c.stack = stack;
  return nullptr;
}

#endif

#if defined(TASKPARTS_X64)

auto new_continuation(native_continuation& c, thunk f) -> void* {
  c.f = f;
  native_continuation* cp;
  if ((cp = (native_continuation*)context_save(&c.gprs[0]))) {
    cp->f();  // only cp is for sure live at this point
    return nullptr;
  }
  c.action = continuation_finish;
  static constexpr
  size_t thread_stack_alignb = 16L;
  static constexpr
  size_t thread_stack_szb = thread_stack_alignb * (1<<12);
  static constexpr
  int _X86_64_SP_OFFSET = 6;
  char* stack = (char*)std::malloc(thread_stack_szb);
  char* sp = &stack[thread_stack_szb];
  sp = (char*)((uintptr_t)sp & (-thread_stack_alignb));  // align stack pointer on 16-byte boundary
  sp -= 128; // for red zone
  void** _ctx = (void**)&c.gprs[0];
  _ctx[_X86_64_SP_OFFSET] = sp;
  c.stack = stack;
#ifdef TASKPARTS_USE_VALGRIND
  c.valgrind_id = VALGRIND_STACK_REGISTER(stack, stack + thread_stack_szb);
#endif
  return nullptr;
}

#endif

// LATER: see about using this function
/*
auto save(native_continuation& c) -> bool {
  return context_save(&c.gprs[0]) != nullptr;
} */

auto throw_to(native_continuation& c) -> void {
  context_restore(&c.gprs[0], &c);
}

auto swap(native_continuation& current, native_continuation& next) -> void {
  if (context_save(&current.gprs[0])) {
    return;
  }
  context_restore(&next.gprs[0], &next);
}

auto get_action(native_continuation& c) -> continuation_action& {
  return c.action;
}

/*---------------------------------------------------------------------*/
/* Worker threads */

environment_variable<> nb_workers("TASKPARTS_NUM_WORKERS",
                                  [] { return std::thread::hardware_concurrency(); },
                                  "controls the number of worker threads used by taskparts "
                                  "(defaultly, number of cores)");

auto get_nb_workers() -> size_t {
  return (size_t)nb_workers.get();
}

static constexpr
int not_a_worker_id = -1;

thread_local
int my_id = not_a_worker_id;

__attribute__((constructor))
auto _init_worker0() -> void {
  my_id = 0;
}

auto get_my_id() -> size_t {
  assert(my_id != not_a_worker_id);
  return my_id;
}

auto after_worker_group_teardown(thunk f) -> void;

template <typename T, size_t cache_align_szb = cache_line_szb>
class perworker_array {
public:
  using aligned_item = typename std::aligned_storage<sizeof(T), cache_align_szb>::type;
  aligned_item* items = nullptr;
  perworker_array() {
    auto a = aligned_alloc(cache_align_szb, get_nb_workers() * sizeof(aligned_item));
    if (a == nullptr) {
      die("failed allocation");
    }
    items = reinterpret_cast<aligned_item*>(a);
    for (size_t i = 0; i < get_nb_workers(); ++i) {
      ::new (&at(i)) T();
    }
    after_worker_group_teardown([this, a] {
      for (size_t i = 0; i < get_nb_workers(); ++i) {
        std::destroy_at(std::launder(&at(i)));
      }
      std::free(a);
    });
  }
  // destructor is called by worker group
  inline
  T& at(size_t i) {
    assert(items != nullptr);
    return *reinterpret_cast<T*>(items + i);
  }
  T& operator[](size_t i) {
    assert(i < get_nb_workers());
    return at(i);
  }
  auto mine() -> T& {
    return at(get_my_id());
  }
};

auto ping_all_workers() -> void;
auto pin_calling_worker() -> void;
auto initialize_machine() -> void;
auto teardown_machine() -> void;

class pthread_worker_group {
public:
  using status_type = enum status_enum { active, teardown, finished };
  bool launched;
  std::mutex exit_mut;
  std::condition_variable exit_cv;
  size_t nb_workers_exited;
  std::vector<std::thread> threads;
  thunk deallocate_scheduler;
  std::vector<thunk> after_finish;
  std::vector<thunk> reports;
  std::vector<thunk> teardowns;
  std::atomic<status_type> status = active;
  
  pthread_worker_group()
  : launched(false), deallocate_scheduler([] { }), nb_workers_exited(0) { }
  ~pthread_worker_group() {
    print_taskparts_help_message();
    report_taskparts_configuration();
    status.store(teardown);
    ping_all_workers();
    status.store(finished);
    for (auto& f : after_finish) {
      f();
    }
    { // join with worker threads;
      // we cannot simply use t.join() b/c worker threads are detatched
      std::unique_lock<std::mutex> lk(exit_mut);
      ++nb_workers_exited;
      exit_cv.wait(lk, [&] {
        return nb_workers_exited == get_nb_workers();
      });
    }
    teardown_machine();
    for (auto& f : reports) {
      f();
    }
    for (auto& f : teardowns) {
      f();
    }
    deallocate_scheduler();
  }
  auto launch(thunk worker_loop, thunk deallocate_scheduler) -> void {
    if (launched) {
      die("already launched worker group\n");
    }
    pthread_setconcurrency((int)get_nb_workers());
    this->deallocate_scheduler = deallocate_scheduler;
    initialize_machine();
    pin_calling_worker();
    threads.resize(get_nb_workers() - 1);
    for (size_t id = 1; id < get_nb_workers(); id++) {
      threads.emplace_back([&, id, worker_loop] () {
        my_id = (int)id;
        pin_calling_worker();
        worker_loop();
        std::unique_lock<std::mutex> lk(exit_mut);
        if (++nb_workers_exited == get_nb_workers()) {
          exit_cv.notify_one();
        }
      });
      threads.back().detach();
    }
  }
};

pthread_worker_group worker_group;

auto after_worker_group_finish(thunk f) -> void {
  worker_group.after_finish.push_back(f);
}
auto worker_group_report(thunk f) -> void {
  worker_group.reports.push_back(f);
}
auto after_worker_group_teardown(thunk f) -> void {
  worker_group.teardowns.push_back(f);
}

/*---------------------------------------------------------------------*/
/* CPU pinning */

#ifdef TASKPARTS_USE_HWLOC
  
using pinning_policy_type = enum pinning_policy_enum {
  pinning_policy_enabled,
  pinning_policy_disabled
};
  
using resource_packing_type = enum resource_packing_enum {
  resource_packing_sparse,
  resource_packing_dense
};

using resource_binding_type = enum resource_binding_enum {
  resource_binding_all,
  resource_binding_by_core,
  resource_binding_by_numa_node
};

environment_variable<bool> numa_alloc_interleaved("TASKPARTS_NUMA_ALLOC_INTERLEAVED",
						  [] { return true; },
						  "use the round-robin policy for allocating pages to numa nodes");
environment_variable<pinning_policy_type> pinning_policy("TASKPARTS_PIN_WORKER_THREADS",
							 [] { return pinning_policy_disabled; },
							 "pin worker threads to according to a specified policy (either 'enabled' or 'disabled'; default: disabled)",
							 [] (const char* s) { return (std::string(s) == "disabled") ? pinning_policy_disabled : pinning_policy_enabled; },
							 [] (pinning_policy_type r) {
							   return in_quotes((r == pinning_policy_disabled) ? "disabled" : "enabled"); });
environment_variable<resource_packing_type> resource_packing("TASKPARTS_RESOURCE_PACKING",
							     [] { return resource_packing_sparse; },
							     "how to pin worker threads to system resources (either 'sparse' or 'dense'; default: sparse)",
							     [] (const char* s) { return (std::string(s) == "dense") ? resource_packing_dense : resource_packing_sparse; },
							     [] (resource_packing_type r) {
							       return in_quotes((r == resource_packing_dense) ? "dense" : "sparse");
							     });
environment_variable<resource_binding_type> resource_binding("TASKPARTS_RESOURCE_BINDING",
							     [] { return resource_binding_all; },
							     "how to pin worker threads to system resources (either 'all', 'by_core', or 'by_numa_node'; default: all)",
							     [] (const char* s) {
							       auto r = resource_binding_all;
							       if (std::string(s) == "all") { ; }
							       else if (std::string(s) == "by_core") { r = resource_binding_by_core; }
							       else if (std::string(s) == "by_numa_node") { r = resource_binding_by_numa_node; }
							       else { die("bogus argument for resource_binding"); }
							       return r; },
							     [] (resource_binding_type r) {
							       std::string s = "";
							       if (r == resource_binding_all) { s = "all"; }
							       else if (r == resource_binding_by_core) { s = "by_core"; }
							       else if (r == resource_binding_by_numa_node) { s = "by_numa_node"; }
							       else { die("bogus argument for resource binding"); }
							       return in_quotes(s);
							     });
  
using hwloc_obj_type = hwloc_obj_type_t;

perworker_array<hwloc_cpuset_t> hwloc_cpusets;
  
hwloc_topology_t topology;
  
hwloc_cpuset_t all_cpus;

auto hwloc_assign_cpusets(hwloc_obj_type resource_binding) -> void {
  auto depth = hwloc_get_type_or_below_depth(topology, resource_binding);
  if (depth < 0) {
    die("requested resource that is unavailable on the calling machine");
  }
  auto topology_depth = hwloc_topology_get_depth(topology);
  if (depth >= topology_depth) {
    die("bogus topology depth");
  }
  auto nb_objects = hwloc_get_nbobjs_by_depth(topology, depth);
  if (nb_objects == 0) {
    die("request to bind taskpartsworker threads to a nonexistent hardware resource\n");
  }
  if (resource_packing.get() == resource_packing_sparse) {
    for (size_t worker_id = 0, object_id = 0; worker_id < get_nb_workers(); worker_id++, object_id++) {
      object_id = (object_id >= nb_objects) ? 0 : object_id;
      //printf("nbo = %d depth = %d oid=%d wid=%d\n",nb_objects,depth,object_id, worker_id);
      auto cpuset = hwloc_get_obj_by_depth(topology, depth, object_id)->cpuset;
      hwloc_cpusets[worker_id] = hwloc_bitmap_dup(cpuset);
    }
  } else if (resource_packing.get() == resource_packing_dense) {
    for (size_t worker_id = 0, object_id = 0, i = 0; worker_id < get_nb_workers(); worker_id++) {
      object_id = (object_id >= nb_objects) ? 0 : object_id;
      auto cpuset = hwloc_get_obj_by_depth(topology, depth, object_id)->cpuset;
      auto nb_in_obj = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, cpuset, depth + 1);
      //printf("i=%d nbo2 = %d depth = %d oid=%d wid=%d nbio=%d\n",i,nb_objects,depth,object_id, worker_id,nb_in_obj);
      if (nb_in_obj == 0) {
        die("bogus request");
      }
      if (++i >= nb_in_obj) {
        i = 0;
        object_id++;
      }
      hwloc_cpusets[worker_id] = hwloc_bitmap_dup(cpuset);
    }
  } else {
    die("impossible");
  }
}

auto initialize_hwloc(bool numa_alloc_interleaved) {
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  if (numa_alloc_interleaved) {
    hwloc_cpuset_t all_cpus =
      hwloc_bitmap_dup(hwloc_topology_get_topology_cpuset(topology));
    int err = hwloc_set_membind(topology, all_cpus, HWLOC_MEMBIND_INTERLEAVE, 0);
    if (err < 0) {
      die("Failed to set NUMA round-robin allocation policy\n");
    }
  }
}
  
#endif

auto pin_calling_worker() -> void {
#if defined(TASKPARTS_USE_HWLOC)
  if (pinning_policy.get() == pinning_policy_disabled) {
    return;
  }
  auto& cpuset = hwloc_cpusets.mine();
  int flags = HWLOC_CPUBIND_STRICT | HWLOC_CPUBIND_THREAD;
  if (hwloc_set_cpubind(topology, cpuset, flags)) {
    char *str;
    int error = errno;
    hwloc_bitmap_asprintf(&str, cpuset);
    die("Couldn't bind to cpuset %s: %s\n", str, strerror(error));
  }
#endif
}
  
auto initialize_machine() -> void {
#if defined(TASKPARTS_USE_HWLOC)
  initialize_hwloc(numa_alloc_interleaved.get());
  auto rb = HWLOC_OBJ_MACHINE;
  if (resource_binding.get() == resource_binding_by_core) {
    rb = HWLOC_OBJ_CORE;
  } else if (resource_binding.get() == resource_binding_by_numa_node) {
    rb = HWLOC_OBJ_NUMANODE;
  }
  hwloc_assign_cpusets(rb);
#endif
}

auto teardown_machine() -> void {
#ifdef TASKPARTS_USE_HWLOC
  hwloc_bitmap_free(all_cpus);
  for (size_t id = 0; id != get_nb_workers(); ++id) {
    hwloc_bitmap_free(hwloc_cpusets[id]);
  }
  hwloc_topology_destroy(topology);
#endif
}

/*---------------------------------------------------------------------*/
/* Random-number generation */

perworker_array<uint64_t> random_seed;

inline
auto random_number(size_t my_id = get_my_id()) -> uint64_t {
  auto& nb = random_seed[my_id];
  nb = hash(my_id) + hash(nb);
  return nb;
}

inline
auto random_other_worker(size_t my_id = get_my_id()) -> size_t {
  auto nb_workers = get_nb_workers();
  assert(nb_workers > 1);
  size_t id = (size_t)(random_number(my_id) % (nb_workers - 1));
  if (id >= my_id) {
    id++;
  }
  assert(id != my_id);
  assert((id >= 0) && (id < nb_workers));
  return id;
}

/*---------------------------------------------------------------------*/
/* Work-stealing deques */

using deque_surplus_result_type = enum deque_surplus_result_enum {
  deque_surplus_stable, deque_surplus_up, deque_surplus_down,
  deque_surplus_unknown
};

// Deque from Arora, Blumofe, and Plaxton (SPAA, 1998).
template <typename Vertex_handle>
struct abp {
  using qidx = unsigned int;
  using tag_t = unsigned int;
  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    tag_t tag;
    qidx top;
  };
  // align to avoid false sharing
  struct alignas(64) padded_fiber {
    std::atomic<Vertex_handle> f;
  };
  
  static constexpr int q_size = 10000;
  std::atomic<qidx> bot;
  std::atomic<age_t> age;
  std::array<padded_fiber, q_size> deq;
  
  abp() : bot(0), age(age_t{0, 0}) {}
  auto size() -> unsigned int {
    return bot.load() - age.load().top;
  }
  auto empty() -> bool {
    return size() == 0;
  }
  auto push(Vertex_handle f) -> deque_surplus_result_type {
    auto local_bot = bot.load(std::memory_order_acquire);      // atomic load
    deq[local_bot].f.store(f, std::memory_order_release);  // shared store
    local_bot += 1;
    if (local_bot == q_size) {
      die("internal error: scheduler queue overflow\n");
    }
    bot.store(local_bot, std::memory_order_seq_cst);  // shared store
    return deque_surplus_unknown;
  }
  auto pop() -> std::pair<Vertex_handle, deque_surplus_result_type> {
    Vertex_handle result = nullptr;
    auto local_bot = bot.load(std::memory_order_acquire);  // atomic load
    if (local_bot != 0) {
      local_bot--;
      bot.store(local_bot, std::memory_order_release);  // shared store
      std::atomic_thread_fence(std::memory_order_seq_cst);
      auto f = deq[local_bot].f.load(std::memory_order_acquire);  // atomic load
      auto old_age = age.load(std::memory_order_acquire);      // atomic load
      if (local_bot > old_age.top) {
        result = f;
      } else {
        bot.store(0, std::memory_order_release);  // shared store
        auto new_age = age_t{old_age.tag + 1, 0};
        if ((local_bot == old_age.top) &&
            age.compare_exchange_strong(old_age, new_age)) {
          result = f;
        } else {
          age.store(new_age, std::memory_order_seq_cst);  // shared store
          result = nullptr;
        }
      }
    }
    return std::make_pair(result, deque_surplus_unknown);
  }
  auto steal() -> std::pair<Vertex_handle, deque_surplus_result_type> {
    Vertex_handle result = nullptr;
    auto old_age = age.load(std::memory_order_acquire);    // atomic load
    auto local_bot = bot.load(std::memory_order_acquire);  // atomic load
    if (local_bot > old_age.top) {
      auto f = deq[old_age.top].f.load(std::memory_order_acquire);  // atomic load
      auto new_age = old_age;
      new_age.top = new_age.top + 1;
      if (age.compare_exchange_strong(old_age, new_age)) {
        result = f;
      } else {
        result = nullptr;
      }
    }
    return std::make_pair(result, deque_surplus_unknown);
  }
};

// Deque from Yue Yao, Sam Westrick, Mike Rainey, and Umut Acar (2022)
template <typename Vertex_handle>
struct ywra {
  using qidx = uint16_t;
  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    uint32_t tag;
    qidx bot;
    qidx top;
  };
  // align to avoid false sharing
  struct alignas(64) padded_fiber {
    std::atomic<Vertex_handle> f;
  };
  
  static constexpr
  int max_sz = (1 << 14); // can in principle be up to 2^16
  
  std::atomic<age_t> age;
  std::array<padded_fiber, max_sz> deq;
  std::array<Vertex_handle, max_sz> backup_deq;
  
  ywra() : age(age_t{0, 0, 0}) { }
  auto size(age_t a) -> unsigned int {
    assert(a.bot >= a.top);
    return a.bot - a.top;
  }
  auto size() -> unsigned int {
    return size(age.load());
  }
  auto empty() -> bool {
    return size() == 0;
  }
  auto relocate() -> age_t {
    auto freeze = [&] () -> age_t {
      auto orig = age.load();
      auto next = orig;
      while (true) {
        next.top = 0;
        next.bot = 0;
        next.tag = orig.tag + 1;
        if (age.compare_exchange_strong(orig, next)) {
          break;
        }
      }
      return orig;
    };
    auto backup = [&] (age_t orig) -> age_t {
      qidx j = 0;
      for (qidx i = orig.top; i < orig.bot; i++) {
        backup_deq[j++] = deq[i].f.load(std::memory_order_acquire);
      }
      assert(j == size(orig));
      return orig;
    };
    auto restore = [&] (age_t orig) -> age_t {
      auto n = size(orig);
      for (qidx i = 0; i < n; i++) {
        deq[i].f.store(backup_deq[i], std::memory_order_seq_cst);
      }
      orig.top = 0;
      orig.bot = n;
      orig.tag++;
      return orig;
    };
    return restore(backup(freeze()));
  }
  auto reset_on_sz_zero(age_t orig) -> age_t {
    if (size(orig) > 0) {
      return orig;
    }
    auto next = orig;
    next.top = 0;
    next.bot = 0;
    next.tag = orig.tag + 1;
    if (! age.compare_exchange_strong(orig, next)) {
      die("bogus");
    }
    return next;
  }
  auto push(Vertex_handle f) -> deque_surplus_result_type {
    auto orig = reset_on_sz_zero(age.load());
    auto next = orig;
    next.bot++;
    if (next.bot == max_sz) {
      if (size(orig) == max_sz) {
        die("internal error: scheduler queue overflow\n");
      }
      orig = relocate();
    }
    next.tag = orig.tag + 1;
    deq[orig.bot].f.store(f, std::memory_order_release);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
      next.tag = orig.tag + 1;
    }
    assert(size(orig) + 1 == size(next));
    return (size(orig) == 0) ? deque_surplus_up : deque_surplus_stable;
  }
  auto pop() -> std::pair<Vertex_handle, deque_surplus_result_type> {
    auto orig = reset_on_sz_zero(age.load());
    if (size(orig) == 0) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto next = orig;
    next.bot--;
    next.tag = orig.tag + 1;
    auto f = deq[next.bot].f.load(std::memory_order_acquire);
    assert(f != nullptr);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
      next.tag = orig.tag + 1;
      assert((orig.bot - 1) == next.bot);
      if (size(orig) == 0) {
        return std::make_pair(nullptr, deque_surplus_stable);
      }
    }
    auto r = (size(orig) == 1) ? deque_surplus_down : deque_surplus_stable;
    return std::make_pair(f, r);
  }
  auto steal() -> std::pair<Vertex_handle, deque_surplus_result_type> {
    auto orig = age.load();
    if (size(orig) == 0) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto next = orig;
    auto f = deq[next.top].f.load(std::memory_order_acquire);
    next.top++;
    next.tag = orig.tag + 1;
    if (! age.compare_exchange_strong(orig, next)) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto r = (size(orig) == 1) ? deque_surplus_down : deque_surplus_stable;
    return std::make_pair(f, r);
  }
};

/*---------------------------------------------------------------------*/
/* Meta scheduling */

class minimal_meta_scheduler {
public:
  template <typename F>
  auto tick(const F& is_finished) -> void { }
  class worker_instance {
  public:
    worker_instance(minimal_meta_scheduler&) { }
  };
  static
  auto yield() {
    std::this_thread::yield();
  }
};

class serial_random_meta_scheduler {
public:
  std::mutex scheduler_mut;
  std::condition_variable scheduler_cond;
  perworker_array<std::unique_lock<std::mutex>*> locks;
  size_t next_worker = 0;
  
  serial_random_meta_scheduler() {
    after_worker_group_finish([this] {
      scheduler_cond.notify_all();
    });
  }
  template <typename F>
  auto tick(const F& is_finished) -> void {
    next_worker = random_number() % get_nb_workers();
    scheduler_cond.notify_all();
    scheduler_cond.wait(*locks.mine(), [&] {
      return (next_worker == get_my_id()) || is_finished();
    });
  };
  
  class worker_instance {
  public:
    std::unique_lock<std::mutex> lock;
    worker_instance(serial_random_meta_scheduler& p) : lock(p.scheduler_mut) {
      p.locks.mine() = &lock;
    }
  };
  static
  auto yield() {
    minimal_meta_scheduler::yield();
  }
};

#ifndef TASKPARTS_META_SCHEDULER_SERIAL_RANDOM
using default_meta_scheduler = minimal_meta_scheduler;
#else
using default_meta_scheduler = serial_random_meta_scheduler;
#endif

/*---------------------------------------------------------------------*/
/* Native fork join */

class minimal_work_stealing_instrumentation {
public:
  auto on_steal() -> void { }
  auto on_create_vertex() -> void { }
  auto on_enter_acquire() -> void { }
  auto on_exit_acquire() -> void { }
  auto on_enter_work() -> void { }
  auto on_exit_work() -> void { }
  auto on_enter_suspend() -> void { }
  auto on_exit_suspend() -> void { }
  auto on_teardown_worker() -> void { }
  auto on_teardown_scheduler() -> void { }
  auto start() -> void { }
  auto reset() -> void { }
  auto capture() -> void { }
  auto report() -> void { }
};

environment_variable<std::string> stats_outfile("TASKPARTS_STATS_OUTFILE",
                                                [] { return std::string(""); },
                                                "file in which to output the taskparts stats, formatted in json; "
						"if given stdout, will print to stdout "
						"(default: empty string)",
                                                [] (const char* s) { return s; },
                                                [] (std::string x) { return x; });

class work_stealing_stats {
public:
  using counter_types = enum counter_enum {
    nb_vertices,
    nb_steals,
    nb_suspends,
    nb_counters
  };
  using summary = struct summary_struct {
    double exectime;
    struct rusage rusage_before;
    struct rusage rusage_after;
    uint64_t total_work_time;
    uint64_t total_idle_time;
    uint64_t total_suspend_time;
    double total_time;
    double utilization;
    uint64_t counters[nb_counters];
  };
  using private_counters = struct private_counters_struct {
    uint64_t counters[nb_counters];
  };
  using private_timers = struct private_timers_struct {
    uint64_t start_work; uint64_t total_work_time;
    uint64_t start_idle; uint64_t total_idle_time;
    uint64_t start_suspend; uint64_t total_suspend_time;
  };
  
  static
  auto name_of(counter_types c) -> const char* {
    switch (c) {
      case nb_steals:          return "nb_steals";
      case nb_vertices:        return "nb_vertices";
      case nb_suspends:        return "nb_suspends";
      default:                 return "unknown_counter";
    }
  }
  
  perworker_array<private_counters> all_counters;
  perworker_array<private_timers> all_timers;
  std::vector<summary> summaries;
  struct rusage ru_launch_time;
  std::chrono::time_point<std::chrono::steady_clock> enter_launch_time;
  
  work_stealing_stats() {
    start();
    worker_group_report([this] {
      capture();
      report();
    });
  }
  inline
  auto increment(counter_types c) -> void {
    all_counters.mine().counters[c]++;
  }
  inline
  auto on_steal() -> void {
    increment(nb_steals);
  }
  inline
  auto on_create_vertex() -> void {
    increment(nb_vertices);
  }
  inline
  auto on_enter_acquire() -> void {
    on_exit_work();
    all_timers.mine().start_idle = cyclecounter();
  }
  inline
  auto on_exit_acquire() -> void {
    auto& t = all_timers.mine();
    t.total_idle_time += since(t.start_idle);
    on_enter_work();
  }
  inline
  auto on_enter_work() -> void {
    all_timers.mine().start_work = cyclecounter();
  }
  inline
  auto on_exit_work() -> void {
    auto& t = all_timers.mine();
    t.total_work_time += since(t.start_work);
  }
  inline
  auto on_enter_suspend() -> void {
    increment(nb_suspends);
    all_timers.mine().start_suspend = cyclecounter();
  }
  inline
  auto on_exit_suspend() -> void {
    auto& t = all_timers.mine();
    t.total_suspend_time += since(t.start_suspend);
  }
  auto on_teardown_worker() -> void { }
  auto on_teardown_scheduler() -> void { }
  auto log_program_point(int line_nb, const char* source_fname, void* ptr) -> void { }
  auto start() -> void {
    for (int c = 0; c < nb_counters; c++) {
      for (size_t i = 0; i < get_nb_workers(); ++i) {
        all_counters[i].counters[c] = 0;
      }
    }
    for (size_t i = 0; i < get_nb_workers(); ++i) {
      auto& t = all_timers[i];
      t.start_work = cyclecounter(); t.total_work_time = 0;
      t.start_idle = cyclecounter(); t.total_idle_time = 0;
      t.start_suspend = cyclecounter(); t.total_suspend_time = 0;
    }
    getrusage(RUSAGE_SELF, &ru_launch_time);
    enter_launch_time = now();
  }
  auto reset() -> void {
    start();
  }
  auto capture() -> void {
    summary s;
    s.exectime = since(enter_launch_time);
    getrusage(RUSAGE_SELF, &(s.rusage_after));
    s.rusage_before = ru_launch_time;
    for (int c = 0; c < nb_counters; c++) {
      uint64_t counter_value = 0;
      for (size_t i = 0; i < get_nb_workers(); ++i) {
        counter_value += all_counters[i].counters[c];
      }
      s.counters[c] = counter_value;
    }
    double total_time = s.exectime * get_nb_workers();
    s.total_work_time = 0;
    s.total_idle_time = 0;
    s.total_suspend_time = 0;
    for (size_t i = 0; i < get_nb_workers(); ++i) {
      auto& t = all_timers[i];
      s.total_work_time += t.total_work_time;
      s.total_idle_time += t.total_idle_time;
      s.total_suspend_time += t.total_suspend_time;
    }
    double relative_idle = seconds_of_cycles(s.total_idle_time) / total_time;
    s.total_time = total_time;
    s.utilization = 1.0 - relative_idle;
    summaries.push_back(s);
  }
  auto report() -> void {
    auto outfile = stats_outfile.get();
    if (outfile == "") {
      return;
    }
    FILE* f = (outfile == "stdout") ? stdout : fopen(outfile.c_str(), "w");
    auto output_before = [&] {
      fprintf(f, "{");
    };
    auto output_after = [&] (bool not_last) {
      if (not_last) {
        fprintf(f, ",\n");
      } else {
        fprintf(f, "}");
      }
    };
    auto output_uint64_value = [&] (const char* n, uint64_t v, bool not_last = true) {
      fprintf(f, "\"%s\": %lu", n, (unsigned long)v);
      output_after(not_last);
    };
    auto output_double_value = [&] (const char* n, double v, bool not_last = true) {
      fprintf(f, "\"%s\": %.3f", n, v);
      output_after(not_last);
    };
    auto output_rusage_tv = [&] (const char* n, struct timeval before, struct timeval after,
                                 bool not_last = true) {
      auto double_of_tv = [] (struct timeval tv) {
        return ((double) tv.tv_sec) + ((double) tv.tv_usec)/1000000.;
      };
      output_double_value(n, double_of_tv(after) - double_of_tv(before), not_last);
    };
    auto output_cycles_in_seconds = [&] (const char* n, uint64_t cs, bool not_last = true) {
      double s = seconds_of_cycles(cs);
      output_double_value(n, s, not_last);
    };
    fprintf(f, "[");
    size_t i = 0;
    for (auto s : summaries) {
      output_before();
      for (int c = 0; c < nb_counters; c++) {
        output_uint64_value(name_of((counter_types)c), s.counters[c]);
      }
#ifndef NDEBUG
      output_uint64_value("cpufreq_khz", cpu_frequency_khz.get());
#endif
      output_double_value("exectime", s.exectime);
      output_rusage_tv("usertime", s.rusage_before.ru_utime, s.rusage_after.ru_utime);
      output_rusage_tv("systime", s.rusage_before.ru_stime, s.rusage_after.ru_stime);
      output_uint64_value("nvcsw", s.rusage_after.ru_nvcsw - s.rusage_before.ru_nvcsw);
      output_uint64_value("nivcsw", s.rusage_after.ru_nivcsw - s.rusage_before.ru_nivcsw);
      output_uint64_value("maxrss", s.rusage_after.ru_maxrss);
      output_uint64_value("nsignals", s.rusage_after.ru_nsignals - s.rusage_before.ru_nsignals);
      output_cycles_in_seconds("total_work_time", s.total_work_time);
      output_cycles_in_seconds("total_idle_time", s.total_idle_time);
      output_cycles_in_seconds("total_suspend_time", s.total_suspend_time);
      output_double_value("total_time", s.total_time);
      output_double_value("utilization", s.utilization, false);
      if (i + 1 != summaries.size()) {
        fprintf(f, ",");
      }
      i++;
    }
    fprintf(f, "]\n");
    if (f != stdout) {
      fclose(f);
    }
    summaries.clear();
  }
};

environment_variable<bool> logging_realtime("TASKPARTS_LOGGING_REALTIME",
                                            [] { return false; },
                                            "print logging events in real time");
environment_variable<bool> logging_phases("TASKPARTS_LOGGING_PHASES",
                                          [] { return true; },
                                          "log phases");
environment_variable<bool> logging_vertices("TASKPARTS_LOGGING_VERTICES",
                                            [] { return false; },
                                            "log vertex events");
environment_variable<bool> logging_migration("TASKPARTS_LOGGING_MIGRATION",
                                             [] { return false; },
                                             "log migration events");
environment_variable<bool> logging_program("TASKPARTS_LOGGING_PROGRAM",
                                           [] { return true; },
                                           "log program events");
environment_variable<std::string> logging_outpath("TASKPARTS_LOGGING_OUTPATH",
                                                  [] { return std::string(""); },
                                                  "path to output logging files",
                                                  [] (const char* s) { return s; },
                                                  [] (std::string x) { return x; });

class work_stealing_logger {
public:
  using event_kind = enum event_kind_enum {
    phases = 0, vertices, migration, program, nb_kinds
  };
  using event_tag = enum event_tag_type_enum {
    enter_launch = 0,   exit_launch,
    enter_algo,         exit_algo,
    enter_wait,         exit_wait,
    worker_communicate, interrupt,
    algo_phase,
    enter_suspend,      exit_suspend,
    worker_exit,        initiate_teardown,
    program_point,
    nb_events
  };
  static
  auto name_of(event_tag e) -> std::string {
    switch (e) {
      case enter_launch:      return "enter_launch ";
      case exit_launch:       return "exit_launch ";
      case enter_algo:        return "enter_algo ";
      case exit_algo:         return "exit_algo ";
      case enter_wait:        return "enter_wait ";
      case exit_wait:         return "exit_wait ";
      case enter_suspend:     return "enter_suspend ";
      case exit_suspend:      return "exit_suspend ";
      case worker_exit:       return "worker_exit ";
      case initiate_teardown: return "initiate_teardown";
      case algo_phase:        return "algo_phase ";
      case program_point:     return "program_point ";
      default:                return "unknown_event ";
    }
  }
  static inline
  auto kind_of(event_tag e) -> event_kind {
    switch (e) {
      case enter_launch:
      case exit_launch:
      case enter_algo:
      case exit_algo:
      case enter_wait:
      case exit_wait:
      case enter_suspend:
      case exit_suspend:
      case algo_phase:                return phases;
      case worker_exit:
      case initiate_teardown:
      case program_point:             return program;
      default:                        return nb_kinds;
    }
  }
  using program_point_type = struct program_point_struct {
    int line_nb;
    const char* source_fname;
    void* ptr;
  };
  
  class event {
  public:
    uint64_t cycle_count;
    event_tag tag;
    size_t worker_id;
    union extra_union {
      program_point_type ppt;
      size_t child_id;
      struct failed_to_sleep_struct {
        size_t parent_id;
        size_t busy_child;
        size_t prio_child;
        size_t prio_parent;
      } failed_to_sleep;
    } extra;
    
    event() { }
    event(event_tag tag) : tag(tag) { }
    auto print_text(FILE* f) -> void {
      auto ns = nanoseconds_of(diff(base_time, cycle_count)) / 1000;
      fprintf(f, "%.9f\t%ld\t%s\t", ((double)ns) / 1.0e6, worker_id, name_of(tag).c_str());
      switch (tag) {
        case program_point: {
          fprintf(f, "%s \t %d \t %p",
                  extra.ppt.source_fname,
                  extra.ppt.line_nb,
                  extra.ppt.ptr);
          break;
        }
        default: {
          // nothing to do
        }
      }
      fprintf (f, "\n");
    }
    auto print_json(FILE* f, bool last) -> void {
      auto print_json_string = [&] (const char* l, const char* s, bool last_row=false) {
        fprintf(f, "\"%s\": \"%s\"%s", l, s, (last_row ? "" : ","));
      };
      auto print_json_number = [&] (const char* l, size_t n, bool last_row=false) {
        fprintf(f, "\"%s\": %lu%s", l, n, (last_row ? "" : ","));
      };
      auto print_hdr = [&] {
        fprintf(f, "{");
      };
      auto print_ftr = [&] {
        fprintf(f, "}%s", (last ? "\n" : ",\n"));
      };
      // we report in microseconds because it's the unit assumed by Chrome's tracer utility
      auto ns = nanoseconds_of(diff(base_time, cycle_count)) / 1000;
      auto print_end = [&] {
        print_json_number("pid", 0);
        print_json_number("tid", worker_id);
        print_json_number("ts", ns, true);
        print_ftr();
      };
      switch (tag) {
        case enter_wait:
        case exit_wait: {
          print_hdr();
          print_json_string("name", "idle");
          print_json_string("cat", "SCHED");
          print_json_string("ph", (tag == enter_wait) ? "B" : "E");
          print_end();
          break;
        }
        case enter_launch:
        case exit_launch: {
          print_hdr();
          print_json_string("name", (tag == enter_launch) ? "launch_begin" : "launch_end");
          print_json_string("cat", "SCHED");
          print_json_string("ph", "i");
          print_json_string("s", "g");
          print_end();
          break;
        }
        case enter_suspend:
        case exit_suspend: {
          print_hdr();
          print_json_string("name", "sleep");
          print_json_string("cat", "SCHED");
          print_json_string("ph", (tag == enter_suspend) ? "B" : "E");
          print_end();
          break;
        }
        case program_point: {
          print_hdr();
          std::string const cstr = extra.ppt.source_fname;
          auto n = cstr + ":" + std::to_string(extra.ppt.line_nb) + " " + std::to_string((size_t)extra.ppt.ptr);
          print_json_string("cat", "PPT");
          print_json_string("name", n.c_str());
          print_json_string("ph", "i");
          print_json_string("s", "t");
          print_end();
          break;
        }
        default: {
          break;
        }
      }
    }
  };
  
  static
  uint64_t base_time;
  
  using buffer = std::deque<event>;
  
  size_t nb_output = 0;
  perworker_array<buffer> buffers;
  bool tracking_kind[nb_kinds];
  
  work_stealing_logger() {
    tracking_kind[phases] = logging_phases.get();
    tracking_kind[vertices] = logging_vertices.get();
    tracking_kind[migration] = logging_migration.get();
    tracking_kind[program] = logging_program.get();
    start();
    worker_group_report([this] {
      report();
    });
  }
  auto push(event e) -> void {
    auto k = kind_of(e.tag);
    assert(k != nb_kinds);
    if (! tracking_kind[k]) {
      return;
    }
    e.cycle_count = cyclecounter();
    e.worker_id = get_my_id();
    if (logging_realtime.get()) {
      std::unique_lock<std::mutex> lk(print_mutex);
      e.print_text(stdout);
    }
    buffers.mine().push_back(e);
  }
  auto log_event(event_tag tag) -> void {
    push(event(tag));
  }
  auto on_enter_acquire() -> void { log_event(enter_wait); }
  auto on_exit_acquire() -> void { log_event(exit_wait); }
  auto on_teardown_worker() -> void { log_event(worker_exit); }
  auto on_teardown_scheduler() -> void { log_event(initiate_teardown); }
  auto on_enter_suspend() -> void { log_event(enter_suspend); }
  auto on_exit_suspend() -> void {
    log_event(exit_suspend);
  }
  auto log_program_point(int line_nb, const char* source_fname, void* ptr) -> void {
    program_point_type ppt = { .line_nb = line_nb, .source_fname = source_fname, .ptr = ptr };
    event e(program_point);
    e.extra.ppt = ppt;
    push(e);
  }
  auto start() -> void {
    reset();
  }
  auto reset() -> void {
    for (auto id = 0; id != get_nb_workers(); id++) {
      buffers[id].clear();
    }
    base_time = cyclecounter();
    push(event(enter_launch));
  }
  auto report() -> void {
    push(event(exit_launch));
    buffer b;
    for (auto id = 0; id != get_nb_workers(); id++) {
      buffer& b_id = buffers[id];
      for (auto e : b_id) {
        b.push_back(e);
      }
    }
    std::stable_sort(b.begin(), b.end(), [] (const event& e1, const event& e2) {
      return e1.cycle_count < e2.cycle_count;
    });
    auto path = logging_outpath.get();
    if (path == "") {
      return;
    }
    struct stat st = {0};
    if (stat(path.c_str(), &st) == -1) {
      mkdir(path.c_str(), 0700);
    }
    auto n = nb_output++;
    auto json_fname = path + "/" + "log" + std::to_string(n) + ".json";
    FILE* f = fopen(json_fname.c_str(), "w");
    fprintf(f, "{ \"traceEvents\": [\n");
    size_t i = b.size();
    for (auto e : b) {
      e.print_json(f, (--i == 0));
    }
    fprintf(f, "],\n");
    fprintf(f, "\"displayTimeUnit\": \"ns\"}\n");
    fclose(f);
  }
};

uint64_t work_stealing_logger::base_time;

template <
typename Stats = work_stealing_stats,
typename Logger = work_stealing_logger>
class work_stealing_instrumentation {
public:
  Stats stats;
  Logger logger;
  auto on_steal() -> void { stats.on_steal(); }
  auto on_create_vertex() -> void { stats.on_create_vertex(); }
  auto on_enter_acquire() -> void {
    logger.on_enter_acquire(); stats.on_enter_acquire();
  }
  auto on_exit_acquire() -> void {
    stats.on_exit_acquire(); logger.on_exit_acquire();
  }
  auto on_enter_work() -> void { stats.on_enter_work(); }
  auto on_exit_work() -> void { stats.on_exit_work(); }
  auto on_enter_suspend() -> void {
    logger.on_enter_suspend();
    stats.on_exit_acquire(); stats.on_enter_suspend();
  }
  auto on_exit_suspend() -> void {
    stats.on_exit_suspend(); stats.on_enter_acquire();
    logger.on_exit_suspend();
  }
  auto on_teardown_worker() -> void { logger.on_teardown_worker(); stats.on_exit_work(); }
  auto on_teardown_scheduler() -> void { logger.on_teardown_scheduler(); }
  auto log_program_point(int line_nb, const char* source_fname, void* ptr) -> void {
    logger.log_program_point(line_nb, source_fname, ptr);
  }
  auto start() -> void { logger.start(); stats.start(); }
  auto reset() -> void { logger.reset(); stats.reset(); }
  auto capture() -> void { stats.capture(); }
  auto report() -> void { stats.report(); logger.report(); }
};

#if defined(TASKPARTS_LOGGING)
using default_work_stealing_instrumentation = work_stealing_instrumentation<>;
#elif defined(TASKPARTS_STATS)
using default_work_stealing_instrumentation = work_stealing_stats;
#else
using default_work_stealing_instrumentation = minimal_work_stealing_instrumentation;
#endif

class minimal_elastic {
public:
  static constexpr
  bool override_rand_worker = false;

  auto try_to_sleep() {
    minimal_meta_scheduler::yield();
  }
  auto incr_stealing() { }
  auto decr_stealing() { }
  auto incr_surplus() { }
  auto decr_surplus(size_t id) { }
  template <typename Instrumentation, typename Meta_scheduler>
  auto try_suspend(Instrumentation& instrumentation,
                   Meta_scheduler& meta_scheduler) { }
  template <typename Is_deque_empty>
  auto random_worker_with_surplus(const Is_deque_empty& is_deque_empty) -> int { return -1; }
  auto scale_up() -> void { }
  auto exists_imbalance() -> bool { return false; }
  auto prepare_end_of_phase() -> void { }
  auto end_phase() -> void { }
};

environment_variable<uint64_t> elastic_alpha("TASKPARTS_ELASTIC_ALPHA",
                                             [] { return 2; },
                                             "scale up factor for elastic scheduler");
environment_variable<uint64_t> elastic_beta("TASKPARTS_ELASTIC_BETA", // LATER: express properly as a faction, i.e., beta = 1/128
                                            [] { return 128; },
                                            "scale down rate for elastic scheduler");

class scale_up_vertex
: public vertex_family<fork_join_edges, native_continuation> {
public:
  auto run() -> void { assert(false); }
  auto deallocate() -> void { assert(false); }
};

scale_up_vertex scale_up_token;

template <typename Semaphore = default_semaphore>
class elastic_s3 {
public:
  using cdata = struct cdata_struct {
    int32_t surplus = 0;
    int16_t suspended = 0;
    int16_t stealers = 0;
    auto exists_imbalance() -> bool {
      return (surplus >= 1) && (suspended >= 1);
    }
    auto needs_sentinel() -> bool {
      return (stealers == 0) && exists_imbalance();
    }
  };
  using counter = struct alignas(int64_t) counter_struct {
    std::atomic<cdata> ounter;
  };
  
  counter c;
  perworker_array<std::atomic_bool> flags;
  perworker_array<Semaphore> semaphores;
  static constexpr
  bool override_rand_worker = false;
  static
  scale_up_vertex scale_up_token;
  std::atomic<bool> no_suspend;
  
  elastic_s3() : no_suspend(false) {
    for (size_t i = 0; i < get_nb_workers(); i++) {
      flags[i].store(false);
    }
  }
  template <typename Update>
  auto update_counters(const Update& u) -> cdata {
    return update_atomic(c.ounter, u, get_my_id());
  }
  auto try_resume(size_t id) -> bool {
    auto orig = true;
    if (flags[id].compare_exchange_strong(orig, false)) {
      semaphores[id].post();
      return true;
    }
    return false;
  }
  auto try_resume_random() -> bool {
    return try_resume(random_number(get_my_id()) % get_nb_workers());
  }
  auto incr_surplus() {
    auto next = update_counters([] (cdata d) {
      d.surplus++;
      return d;
    });
    ensure_sentinel(next);
  }
  auto decr_surplus(size_t id) {
    update_counters([] (cdata d) {
      d.surplus--;
      return d;
    });
  }
  auto incr_stealing() {
    update_counters([] (cdata d) {
      d.stealers++;
      return d;
    });
  }
  auto decr_stealing() {
    auto next = update_counters([] (cdata d) {
      d.stealers--;
      return d;
    });
    ensure_sentinel(next);
  }
  auto ensure_sentinel(cdata next) {
    while (next.needs_sentinel()) {
      try_resume_random();
      next = c.ounter.load();
    }
  }
  auto scale_up(cdata next) -> void {
    auto n = elastic_alpha.get();
    while (n > 0) {
      if (! next.exists_imbalance()) {
        break;
      }
      if (try_resume_random()) {
        n--;
      }
      next = c.ounter.load();
    }
  }
  auto scale_up() {
    scale_up(c.ounter.load());
  }
  auto exists_imbalance(cdata cd) -> bool {
    return cd.exists_imbalance();
  }
  auto exists_imbalance() -> bool {
    return exists_imbalance(c.ounter.load());
  }
  template <typename Instrumentation, typename Meta_scheduler>
  auto try_suspend(Instrumentation& instrumentation,
                   Meta_scheduler& meta_scheduler) {
    if (no_suspend.load() || (worker_group.status != pthread_worker_group::active)) {
      return;
    }
    auto flip = [&] () -> bool {
      auto n = random_number(get_my_id());
      return (n % elastic_beta.get()) == 0;
    };
    if (! flip()) {
      Meta_scheduler::yield();
      return;
    }
    flags.mine().store(true);
    auto next = update_counters([] (cdata d) {
      d.stealers--;
      d.suspended++;
      return d;
    });
    if (next.needs_sentinel()) {
      try_resume(get_my_id());
    } { instrumentation.on_enter_suspend(); }
    semaphores.mine().wait(); { instrumentation.on_exit_suspend(); }
    update_counters([=] (cdata d) {
      d.stealers++;
      d.suspended--;
      return d;
    });
    Meta_scheduler::yield();
  }
  auto prepare_end_of_phase() -> void {
    no_suspend.store(true);
    semaphores[0].post();
  }
  auto end_phase() -> void {
    no_suspend.store(false);
  }
};

#if !defined(TASKPARTS_DISABLE_ELASTIC)
template <typename Vertex_handle>
using default_native_fork_join_deque = ywra<Vertex_handle>;
using default_elastic = elastic_s3<>;
#else
template <typename Vertex_handle>
using default_native_fork_join_deque = abp<Vertex_handle>;
using default_elastic = minimal_elastic;
#endif

default_elastic elastic;

template <
typename Vertex_handle,
template <typename> typename Deque = default_native_fork_join_deque>
class native_fork_join_deque_family {
public:
  Deque<Vertex_handle> deque;
  Vertex_handle bottom = nullptr;
  
  auto size() -> size_t {
    return deque.size() + ((bottom == nullptr) ? 0 : 1);
  }
  auto empty() -> bool {
    return size() == 0;
  }
  auto push(Vertex_handle v) -> void { { assert(v != nullptr); }
    if (bottom == nullptr) {
      bottom = v;
      return;
    }
    auto v2 = bottom;
    bottom = v;
    if (deque.push(v2) == deque_surplus_up) {
      elastic.incr_surplus();
    }
  }
  auto pop() -> Vertex_handle {
    if (bottom != nullptr) {
      auto v = bottom;
      bottom = nullptr;
      return v;
    }
    auto [v, status] = deque.pop();
    if (status == deque_surplus_down) {
      elastic.decr_surplus(get_my_id());
    }
    return (v != (Vertex_handle)&scale_up_token) ? v : nullptr;
  }
  template <typename Deques>
  auto steal(Deques& deques, size_t worker_id) -> Vertex_handle { { assert(get_my_id() != worker_id);}
    auto [v, status] = deque.steal();
    if (status == deque_surplus_down) {
      elastic.decr_surplus(worker_id);
    }
    if (v == (Vertex_handle)&scale_up_token) {
      elastic.scale_up();
      v = nullptr;
    }
    if ((v != nullptr) && elastic.exists_imbalance()) {
      assert(deques.mine().empty());
      if (deques.mine().deque.push((Vertex_handle)&scale_up_token) == deque_surplus_up) {
        elastic.incr_surplus();
      }
    }
    return v;
  }
};

template <
typename Instrumentation = default_work_stealing_instrumentation,
typename Meta_scheduler = default_meta_scheduler>
class native_fork_join_scheduler_family {
public:
  using continuation = native_fork_join_continuation;
  using vertex = native_fork_join_vertex;
  using deque = native_fork_join_deque_family<vertex*>;
  template <typename T>
  using thunk_vertex = native_fork_join_thunk_vertex<T>;
  
  perworker_array<deque> deques;
  perworker_array<vertex*> currents;
  perworker_array<continuation> worker_continuations;
  std::atomic<vertex*> sink_vertex;
  Instrumentation instrumentation;
  Meta_scheduler meta_scheduler;
  
  native_fork_join_scheduler_family()
  : sink_vertex(nullptr) {
    worker_group.launch([this] { loop(); }, [this] { delete this; });
  }
  auto is_phase_finished() -> bool {
    return sink_vertex.load() == nullptr;
  }
  auto is_finished() -> bool {
    if (get_my_id() == 0) {
      return is_phase_finished();
    }
    return worker_group.status.load() == pthread_worker_group::finished;
  }
  auto loop() -> void {
    typename Meta_scheduler::worker_instance _meta_scheduler_instance(meta_scheduler);
    auto& current = currents.mine();
    auto acquire = [this] {
      size_t nb_attempts = 0;
      while (! is_finished()) { { meta_scheduler.tick([&] { return is_finished(); }); }
        auto victim = random_other_worker();
        auto v = deques[victim].steal(deques, victim);
        if (v != nullptr) { { assert(get_action(v->continuation) == continuation_initialize); }
          deques.mine().push(v); { instrumentation.on_steal(); }
          return;
        }
        if (++nb_attempts == (8 * get_nb_workers())) {
          elastic.try_suspend(instrumentation, meta_scheduler);
          nb_attempts = 0;
        }
      }
    }; { instrumentation.on_enter_work(); }
    while (true) {
      if (deques.mine().empty() && (get_nb_workers() > 1)) {
        elastic.incr_stealing(); { instrumentation.on_enter_acquire(); }
        acquire(); { instrumentation.on_exit_acquire(); }
        elastic.decr_stealing();
      }
      if (is_finished()) { { instrumentation.on_teardown_worker(); /* ==> on_exit_work */ }
        return;
      }
      current = deques.mine().pop();
      if (current == nullptr) {
        continue;
      } { assert(current->edges.incounter.load() == 0); }
      if (get_action(current->continuation) == continuation_initialize) {
        new_continuation(current->continuation, [this] { enter(); });
      }
      swap(worker_continuation(), current->continuation);
      if (get_action(current->continuation) == continuation_finish) {
        parallel_notify<native_fork_join_scheduler_family>(current);
        if (current == sink_vertex.load()) { // all deques should be empty now
          sink_vertex.store(nullptr);
        }
      } else {
        { assert(get_action(current->continuation) == continuation_continue); }
      } { meta_scheduler.tick([&] { return is_phase_finished() || is_finished(); }); }
    }
  }
  auto fork2join(vertex& v1, vertex& v2) -> void {
    auto& v0 = *self(); // parent task
    { assert(v0.edges.incounter.load() == 0); assert(get_action(v0.continuation) == continuation_finish); }
    if (context_save(&(v0.continuation.gprs[0]))) {
      { assert(self() == &v0); assert(get_action(v0.continuation) == continuation_continue); }
      get_action(v0.continuation) = continuation_finish;
      return; // exit point of parent task if v2 was stolen
    }
    get_action(v0.continuation) = continuation_continue;
    initialize_fork(&v0, &v1, &v2);
    get_action(v1.continuation) = continuation_continue;
    swap(v1.continuation, worker_continuation()); { assert(self() == &v1); }
    get_action(v1.continuation) = continuation_finish;
    v1.run(); { assert(self() == &v1); }
    auto vb = deques.mine().pop();
    if (vb == nullptr) { // v2 was stolen
      { assert(get_action(v1.continuation) == continuation_finish); }
      throw_to(worker_continuation());
    }
    // v2 was not stolen
    { assert(vb == &v2); assert(v2.edges.incounter.load() == 0); assert(v0.edges.incounter.load() == 2); }
    get_action(v2.continuation) = continuation_continue;
    schedule(&v2);
    swap(v2.continuation, worker_continuation());
    get_action(v2.continuation) = continuation_finish;
    { assert(self() == &v2); assert(v0.edges.incounter.load() == 1); }
    v2.run();
    { assert(self() == &v2); assert(get_action(v0.continuation) == continuation_continue); }
    swap(v0.continuation, worker_continuation());
    { assert(self() == &v0); assert(v0.edges.incounter.load() == 0); }
    get_action(v0.continuation) = continuation_finish;
    // exit point of parent task if v2 was not stolen
  }
  auto schedule(vertex* v) -> void { { assert(v->edges.incounter.load() == 0); }
    deques.mine().push(v);
  }
  auto enter() -> void {
    currents.mine()->run();
    throw_to(worker_continuation());
  }
  auto initialize_fork(vertex* parent, vertex* child1, vertex* child2) -> void {
    vertex* vs[] = {child2, child1};
    for (auto v : vs) {
      initialize_vertex(v);
      new_edge(v, parent);
    } { assert(parent->edges.incounter.load() == 2); }
    for (auto v : vs) {
      release(v);
    } { assert(child1->edges.incounter.load() == 0); }
  }
  auto initialize_vertex(vertex* v) -> void {
    // TODO: introduce optimization that eliminates the need for the unnecessary atomic fetch and add in new_counter()
    new_incounter(v); { instrumentation.on_create_vertex(); }
    new_outset(v);
    increment(v);
  }
  auto new_edge(vertex* v, vertex* u) -> void {
    increment(u);
    auto r = add(v, u); { assert(r == outset_add_success); }
  }
  auto release(vertex* v) -> void {
    decrement<native_fork_join_scheduler_family>(v);
  }
  auto self() -> vertex* {
    return currents.mine();
  }
  auto worker_continuation() -> continuation& {
    return worker_continuations.mine();
  }
  auto launch(vertex& source) -> void {
    auto assert_all_deques_empty = [&] {
#ifndef NDEBUG
      for (size_t i = 0; i < get_nb_workers(); i++) {
        assert(deques[i].empty());
      }
#endif
    };
    { assert_all_deques_empty(); assert(get_my_id() == 0); assert(sink_vertex.load() == nullptr); }
    auto f = [] { };
    thunk_vertex<decltype(f)> sink(f);
    sink_vertex.store(&sink);
    auto p = [] { elastic.prepare_end_of_phase(); };
    thunk_vertex<decltype(p)> unsuspend_worker0(p);
    vertex* vertices[] = {&sink, &unsuspend_worker0, &source};
    for (auto v : vertices) {
      initialize_vertex(v);
    }
    new_edge(&source, &unsuspend_worker0);
    new_edge(&unsuspend_worker0, sink_vertex.load());
    for (auto v : vertices) {
      release(v);
    }
    loop(); { instrumentation.on_teardown_scheduler(); }
    { assert(get_my_id() == 0); assert(sink_vertex.load() == nullptr); assert_all_deques_empty(); }
    elastic.end_phase();
  }
  static
  auto Schedule(vertex* v) -> void;
};

using native_fork_join_scheduler = native_fork_join_scheduler_family<>;

native_fork_join_scheduler* scheduler = new native_fork_join_scheduler;

auto deque_size(size_t i) -> size_t { // FOR DEBUGGING
  return scheduler->deques[i].size();
}

template <typename Instrumentation, typename Meta_scheduler>
auto native_fork_join_scheduler_family<Instrumentation, Meta_scheduler>::Schedule(vertex* v) -> void {
  scheduler->schedule(v);
}

auto __fork2join(native_fork_join_vertex& v1, native_fork_join_vertex& v2) -> void {
  scheduler->fork2join(v1, v2);
}

extern
auto __launch(native_fork_join_vertex& source) -> void {
  scheduler->launch(source);
}

bool in_scheduler = false;

auto tick() -> void {
  scheduler->meta_scheduler.tick([&] { return scheduler->is_finished(); });
}

environment_variable<uint64_t> benchmark_nb_repeat("TASKPARTS_BENCHMARK_NUM_REPEAT",
                                                   [] { return 1; },
                                                   "number of times to run the benchmark");
environment_variable<uint64_t> benchmark_warmup_secs("TASKPARTS_BENCHMARK_WARMUP_SECS",
                                                     [] { return 3; },
                                                     "time in seconds to warm up the scheduler"
                                                     "before running the benchmark");
environment_variable<bool> benchmark_verbose("TASKPARTS_BENCHMARK_VERBOSE",
                                             [] { return false; },
                                             "print the progress of benchmarking runs "
                                             "in real time");

auto get_benchmark_warmup_secs() -> double {
  return benchmark_warmup_secs.get();
}
auto get_benchmark_verbose() -> bool {
  return benchmark_verbose.get();
}
auto get_benchmark_nb_repeat() -> size_t {
  return benchmark_nb_repeat.get();
}

auto instrumentation_on_enter_work() -> void {
  scheduler->instrumentation.on_enter_work();
}
auto instrumentation_on_exit_work() -> void {
  scheduler->instrumentation.on_exit_work();
}  
auto instrumentation_start() -> void {
  scheduler->instrumentation.start();
}
auto instrumentation_capture() -> void {
  scheduler->instrumentation.capture();    
}
auto instrumentation_reset() -> void {
  scheduler->instrumentation.reset();
}
auto instrumentation_report() -> void {
  scheduler->instrumentation.report();    
}
auto log_program_point(int line_nb, const char* source_fname, void* ptr) -> void {
  scheduler->instrumentation.log_program_point(line_nb, source_fname, ptr);
}

auto ping_all_workers() -> void {
 reset_scheduler([&] {}, [&] { }, true);
}

} // end namespace taskparts
