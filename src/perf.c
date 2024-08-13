// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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
    .time_taken = timespec_add(a.time_taken, b.time_taken),
  };
  return res;
}

perf_values perf_zero = {.time_taken = 0};

perf_values perf_end(perf_state state) {
  perf_values res;
  res.time_taken = time_since_monotonic(state.start_time);
  return res;
}

#endif
