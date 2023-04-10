// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <time.h>
#include <predef/predef.h>

#include "timing.h"
#include "timespec.h"

#ifdef PREDEF_OS_WINDOWS
#include "platform/windows/timing.c"
#elif defined(PREDEF_STANDARD_POSIX_2001)
#include "platform/posix/timing.c"
#endif

struct timespec time_since_monotonic(const struct timespec start) {
  struct timespec end = get_monotonic_time();
  return timespec_subtract(end, start);
}
