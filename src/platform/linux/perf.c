#include <predef/predef/os.h>

#ifdef PREDEF_OS_LINUX

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#include "../../perf.h"
#include "../../timespec.h"
#include "../../timing.h"

// perf counter syscall
fd perf_event_open(struct perf_event_attr *hw, pid_t pid, int cpu, int group, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw, pid, cpu, group, flags);
}

perf_state perf_start(void) {
  perf_state res;
  {
    struct perf_event_attr params = {
      .type = PERF_TYPE_HARDWARE,
      .size = sizeof(struct perf_event_attr),
      .config = PERF_COUNT_HW_CPU_CYCLES,
      .exclude_kernel = 1,
      .exclude_callchain_kernel = true,
      .exclude_hv = true,
      .read_format = PERF_FORMAT_ID|PERF_FORMAT_GROUP,
    };
    res.group_leader = perf_event_open(&params, 0, -1, -1, 0);
    res.descriptors[0] = res.group_leader;
    if (res.group_leader == -1) {
      perror("Couldn't create lead performance counter");
      abort();
    }
    ioctl(res.group_leader, PERF_EVENT_IOC_ID, &res.hw_cpu_cycles_id);
  }

  int hw_events[] = {
    PERF_COUNT_HW_BRANCH_MISSES,
    PERF_COUNT_HW_CACHE_MISSES,
    PERF_COUNT_HW_INSTRUCTIONS,
    PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
  };
  uint64_t *hw_ids[] = {
    &res.hw_branch_mispredictions_id,
    &res.hw_cache_misses_id,
    &res.hw_retired_instructions_id,
    &res.hw_retired_branches_id,
  };

  for (int i = 0; i < NUM_EVENTS - 1; i++) {
    struct perf_event_attr params = {
      .type = PERF_TYPE_HARDWARE,
      .size = sizeof(struct perf_event_attr),
      .config = hw_events[i],
      .exclude_kernel = 1,
      .exclude_callchain_kernel = true,
      .exclude_hv = true,
      .read_format = PERF_FORMAT_ID|PERF_FORMAT_GROUP,
    };
    int fd = perf_event_open(&params, 0, -1, res.group_leader, 0);
    if (fd == -1) {
      perror("Couldn't create subsequent performance counter");
      abort();
    }
    res.descriptors[i + 1] = fd;
    ioctl(fd, PERF_EVENT_IOC_ID, hw_ids[i]);
  }

  res.start_time = get_monotonic_time();
  ioctl(res.group_leader, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
  ioctl(res.group_leader, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  return res;
}

typedef struct {
  uint64_t value;     /* The value of the event */
  uint64_t id;        /* if PERF_FORMAT_ID */
} read_format_event;

typedef struct {
   uint64_t nr;            /* The number of events */
   // uint64_t time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
   // uint64_t time_running;  /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
   read_format_event values[NUM_EVENTS];
} read_format;

perf_values perf_end(perf_state state) {
  ioctl(state.group_leader, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
  perf_values res;
  res.time_taken = time_since_monotonic(state.start_time);
  read_format results;
  {
    ssize_t res = read(state.group_leader, &results, sizeof(read_format));
    if (res == -1) {
      perror("Couldn't read performance counters");
      abort();
    }
  }
  if (results.nr != NUM_EVENTS) {
    give_up("Wrong number of perf events returned");
  }

  // TODO figure out whether order is consistent, so we can remove this loop
  // and the branches.
  for (int i = 0; i < NUM_EVENTS; i++) {
    if (close(state.descriptors[i]) == -1) {
      perror("Couldn't close performance counter");
      exit(1);
    }
    read_format_event event = results.values[i];
    if (event.id == state.hw_cpu_cycles_id) {
      res.hw_cpu_cycles = event.value;
    } else if (event.id == state.hw_retired_instructions_id) {
      res.hw_retired_instructions = event.value;
    } else if (event.id == state.hw_retired_branches_id) {
      res.hw_retired_branches = event.value;
    } else if (event.id == state.hw_branch_mispredictions_id) {
      res.hw_branch_mispredictions = event.value;
    } else if (event.id == state.hw_cache_misses_id) {
      res.hw_cache_misses = event.value;
    }
  }
  return res;
}

perf_values perf_add(perf_values a, perf_values b) {
  perf_values res = {
    .time_taken = timespec_add(a.time_taken, b.time_taken),
    .hw_cpu_cycles = a.hw_cpu_cycles + b.hw_cpu_cycles,
    .hw_retired_instructions = a.hw_retired_instructions + b.hw_retired_instructions,
    .hw_retired_branches = a.hw_retired_branches + b.hw_retired_branches,
    .hw_branch_mispredictions = a.hw_branch_mispredictions + b.hw_branch_mispredictions,
    .hw_cache_misses = a.hw_cache_misses + b.hw_cache_misses,
  };
  return res;
}

perf_values perf_zero = {
  .time_taken = {0},
  .hw_cpu_cycles = 0,
  .hw_retired_instructions = 0,
  .hw_retired_branches = 0,
  .hw_branch_mispredictions = 0,
  .hw_cache_misses = 0,
};

#endif
