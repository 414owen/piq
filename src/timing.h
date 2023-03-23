#pragma once

#include <time.h>

// platform-dependent
struct timespec get_monotonic_time(void);

struct timespec time_since_monotonic(const struct timespec start);
