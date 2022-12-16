#include <time.h>
#include <stdio.h>

struct timespec get_monotonic_time(void) {
  struct timespec res;
  clock_gettime(CLOCK_MONOTONIC, &res);
  printf("%ld %ld\n", res.tv_sec, res.tv_nsec);
  return res;
}
