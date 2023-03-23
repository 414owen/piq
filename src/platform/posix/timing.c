#include <time.h>
#include <stdio.h>

struct timespec get_monotonic_time(void) {
  struct timespec res;
  clock_gettime(CLOCK_MONOTONIC, &res);
  return res;
}
