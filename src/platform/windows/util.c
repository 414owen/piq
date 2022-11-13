#include <stdint.h>
#include <time.h>
#include <Winnt.h>

// UNTESTED

static LARGE_INTEGER freq = 0;

static long ticks_to_nanos(LONGLONG subsecond_time) {
    return (long)((1e9 * subsecond_time) / freq);
}

void initialise_util_windows(void) {
  QueryPerformanceFrequency(&freq);
}

struct timespec get_monotonic_time(void) {
  LARGE_INTEGER time;
  QueryPerformanceCounter(&time);
  struct timespec res = {
    .tv_sec = time.QuadPart / freq.QuadPart;
    .tv_nsec = ticks_to_nanos(time.QuadPart % freq.QuadPart);
  };
  return res;
}
