#pragma once

#include <stdint.h>
#include <time.h>

#include "../../timespec.h"

typedef int fd;

#define NUM_EVENTS 5

typedef struct perf_state_linux {
  fd group_leader;
  fd descriptors[NUM_EVENTS];
  uint8_t num_successful_subscriptions;
  timespec start_time;
  uint64_t hw_cpu_cycles_id;
  uint64_t hw_branch_mispredictions_id;
  uint64_t hw_cache_misses_id;
  uint64_t hw_retired_instructions_id;
  uint64_t hw_retired_branches_id;
} perf_state;
