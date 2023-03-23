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

