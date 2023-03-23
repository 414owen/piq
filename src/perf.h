#pragma once

#include <stdint.h>
#include <time.h>
#include <predef/predef/os.h>

#include "util.h"

#ifdef PREDEF_OS_LINUX
# include "platform/linux/perf.h"
#else

typedef struct {
  struct timespec time_taken;
} perf_state;

#endif

typedef struct {
  struct timespec time_taken;

#ifdef PREDEF_OS_LINUX
  uint64_t hw_cpu_cycles;
  uint64_t hw_branch_mispredictions;
  uint64_t hw_cache_misses;
  uint64_t hw_retired_instructions;
#endif
} perf_values;

extern perf_values perf_zero;

perf_state perf_start(void);
perf_values perf_end(perf_state state);
perf_values perf_add(perf_values a, perf_values b);
