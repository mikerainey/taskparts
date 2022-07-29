#pragma once

#include <cstdio>
#include <assert.h>
#include <pthread.h>
#include <vector>
#ifdef TASKPARTS_HAVE_HWLOC
#include <hwloc.h>
#endif

#include "../perworker.hpp"
#include "diagnostics.hpp"

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Detect CPU frequency */

static
auto detect_cpu_frequency_khz() -> uint64_t {
  unsigned long cpu_frequency_khz = 0;
  if (const auto env_p = std::getenv("TASKPARTS_CPU_BASE_FREQUENCY_KHZ")) {
    cpu_frequency_khz = std::stoi(env_p);
    return (uint64_t)cpu_frequency_khz;
  }
  // later: find a way to get cpu frequency in mac/darwin
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

/*---------------------------------------------------------------------*/
/* Binding worker threads to cores or NUMA nodes */

#ifdef TASKPARTS_HAVE_HWLOC

using hwloc_coordinate_type = struct hwloc_coordinate_struct {
  int depth;
  int position;
};

static constexpr
hwloc_coordinate_type empty_coordinate = { .depth = -1, .position = -1 };

perworker::array<hwloc_coordinate_type> worker_hwloc_coordinates;

perworker::array<hwloc_cpuset_t> hwloc_cpusets;
  
hwloc_topology_t topology;
  
hwloc_cpuset_t all_cpus;

auto hwloc_assign_cpusets(size_t nb_workers,
                          pinning_policy_type _pinning_policy,
                          resource_packing_type resource_packing,
                          resource_binding_type resource_binding) {
  pinning_policy = _pinning_policy;
  all_cpus = hwloc_bitmap_dup(hwloc_topology_get_topology_cpuset(topology));
  std::vector<size_t> max_nb_workers_by_resource;
  std::vector<hwloc_coordinate_type> resource_ids;
  if (resource_binding == resource_binding_by_core) {
    auto core_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);
    size_t nb_cores = hwloc_get_nbobjs_by_depth(topology, core_depth);
    if (nb_workers > nb_cores) {
      taskparts_die("hwloc_assign_cpusets: Requested %d workers but also requested to pin each worker to one of %d available cores\n", nb_workers, nb_cores);
    }
    max_nb_workers_by_resource.resize(nb_cores);
    resource_ids.resize(nb_cores, empty_coordinate);
    for (int core_id = 0; core_id < nb_cores; core_id++) {
      hwloc_obj_t core = hwloc_get_obj_by_depth(topology, core_depth, core_id);
      max_nb_workers_by_resource[core_id] =
        hwloc_get_nbobjs_inside_cpuset_by_type(topology, core->cpuset, HWLOC_OBJ_CORE);
      resource_ids[core_id] = { .depth = core_depth, .position = core_id };
    }
  } else if (resource_binding == resource_binding_by_numa_node) {
    taskparts_die("todo");
  } else {
    assert(resource_binding == resource_binding_all);
    for (size_t worker_id = 0; worker_id != nb_workers; ++worker_id) {
      hwloc_cpusets[worker_id] = hwloc_bitmap_dup(all_cpus);
    }
    return;
  }
  auto assignments = assign_workers_to_resources(resource_packing,
                                                 nb_workers,
                                                 max_nb_workers_by_resource,
                                                 resource_ids);
  for (size_t worker_id = 0; worker_id != nb_workers; ++worker_id) {
    hwloc_cpuset_t cpuset;
    auto assignment = assignments[worker_id];
    if (assignment.depth == -1) {
      cpuset = all_cpus;
    } else {
      cpuset = hwloc_get_obj_by_depth(topology, assignment.depth, assignment.position)->cpuset;
    }
    hwloc_cpusets[worker_id] = hwloc_bitmap_dup(cpuset);
  }
}

auto initialize_hwloc(size_t nb_workers, bool numa_alloc_interleaved) {
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  if (numa_alloc_interleaved) {
    hwloc_cpuset_t all_cpus =
      hwloc_bitmap_dup(hwloc_topology_get_topology_cpuset(topology));
    int err = hwloc_set_membind(topology, all_cpus, HWLOC_MEMBIND_INTERLEAVE, 0);
    if (err < 0) {
      taskparts_die("Failed to set NUMA round-robin allocation policy\n");
    }
  }
}

auto hwloc_pin_calling_worker() {
  if (pinning_policy == pinning_policy_disabled) {
    return;
  }
  auto& cpuset = hwloc_cpusets.mine();
  int flags = HWLOC_CPUBIND_STRICT | HWLOC_CPUBIND_THREAD;
  if (hwloc_set_cpubind(topology, cpuset, flags)) {
    char *str;
    int error = errno;
    hwloc_bitmap_asprintf(&str, cpuset);
    taskparts_die("Couldn't bind to cpuset %s: %s\n", str, strerror(error));
  }
}
  
#endif

auto pin_calling_worker() {
#ifdef TASKPARTS_HAVE_HWLOC
  hwloc_pin_calling_worker();
#endif
}

auto initialize_machine() {
  auto nb_workers = perworker::nb_workers();
  pthread_setconcurrency((int)nb_workers);
  bool requested_pinning_policy = false;
  bool numa_alloc_interleaved = false;
  if (const auto env_p = std::getenv("TASKPARTS_NUMA_ALLOC_INTERLEAVED")) {
    requested_pinning_policy = true;
    numa_alloc_interleaved = std::stoi(env_p) == 1;
  }
  if (const auto env_p = std::getenv("TASKPARTS_PIN_WORKER_THREADS")) {
    requested_pinning_policy = true;
    pinning_policy = (std::stoi(env_p) == 1) ? pinning_policy_enabled : pinning_policy_disabled;
  }
  resource_packing_type resource_packing = resource_packing_dense;
  if (const auto env_p = std::getenv("TASKPARTS_RESOURCE_PACKING")) {
    requested_pinning_policy = true;
    if (std::string(env_p) == "sparse") {
      resource_packing = resource_packing_sparse;
    } else if (std::string(env_p) != "dense") {
      taskparts_die("Bogus setting for environment variable TASKPARTS_RESOURCE_PACKING");
    }
  }
  resource_binding_type resource_binding = resource_binding_by_core;
  if (const auto env_p = std::getenv("TASKPARTS_RESOURCE_BINDING")) {
    requested_pinning_policy = true;
    if (std::string(env_p) == "all") {
      resource_binding = resource_binding_all;
    } else if (std::string(env_p) == "by_numa_node") {
      resource_binding = resource_binding_by_numa_node;
    } else if (std::string(env_p) != "by_core") {
      taskparts_die("Bogus setting for environment variable TASKPARTS_RESOURCE_BINDING");
    }
  }
#if defined(TASKPARTS_HAVE_HWLOC)
  initialize_hwloc(nb_workers, numa_alloc_interleaved);
  hwloc_assign_cpusets(nb_workers, pinning_policy, resource_packing, resource_binding);
#else
  if (requested_pinning_policy) {
    taskparts_die("Requested pinning policy, but need hwloc to realize it");
  }
#endif
}

auto teardown_machine() {
#ifdef TASKPARTS_HAVE_HWLOC
  auto nb_workers = perworker::nb_workers();
  hwloc_bitmap_free(all_cpus);
  for (std::size_t worker_id = 0; worker_id != nb_workers; ++worker_id) {
    hwloc_bitmap_free(hwloc_cpusets[worker_id]);
  }
  hwloc_topology_destroy(topology);
#endif
}

} // end namespace

