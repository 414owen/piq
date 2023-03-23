#include <predef/predef/os.h>

#include "perf.h"

// linux is handled by platform/linux/perf.{h,c}
// on all other systems, we just record time.
#ifndef PREDEF_OS_LINUX

perf_state perf_start(void) {
  perf_state res = {
    .start_time = get_monotonic_time(),
  };
  return res;
}

perf_values perf_add(perf_values a, perf_values b) {
  perf_values res = {
    .time_taken = timespec_add(a, b),
  };
  return res;
}

perf_values perf_zero = {.time_taken = {0}};

#endif
