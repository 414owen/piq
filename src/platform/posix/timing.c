#include <time.h>

#include "../../timespec.h"

timespec get_monotonic_time(void) {
#ifdef STD_C11
  timespec res;
  clock_gettime(CLOCK_MONOTONIC, &res);
  return res;
#else
  return clock();
#endif
}
