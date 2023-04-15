// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "defs.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "defs.h"
#include "timespec.h"

#define NSEC_PER_SEC 1e9

#ifdef STD_C11

/* timespec_normalise, timespec_add, timespec_subtract functions taken from
 * https://github.com/solemnwarning/timespec/blob/master/timespec.c
 * which is published under the <http://unlicense.org/> license.

 * Credit goes to:
 * Daniel Collins
 * Alex Forencich
 * Ingo Albrecht
 */

/* Normalises a timespec structure.
 *
 * Returns a normalised version of a timespec structure, according to the
 * following rules:
 *
 * 1) If tv_nsec is >=1,000,000,00 or <=-1,000,000,000, flatten the surplus
 *    nanoseconds into the tv_sec field.
 *
 * 2) If tv_nsec is negative, decrement tv_sec and roll tv_nsec up to represent
 *    the same value attainable by ADDING nanoseconds to tv_sec.
 */

static timespec timespec_normalise(timespec ts) {
  while (ts.tv_nsec >= NSEC_PER_SEC) {
    ++(ts.tv_sec);
    ts.tv_nsec -= NSEC_PER_SEC;
  }

  while (ts.tv_nsec <= -NSEC_PER_SEC) {
    --(ts.tv_sec);
    ts.tv_nsec += NSEC_PER_SEC;
  }

  if (ts.tv_nsec < 0) {
    /* Negative nanoseconds isn't valid according to POSIX.
     * Decrement tv_sec and roll tv_nsec over.
     */

    --(ts.tv_sec);
    ts.tv_nsec = (NSEC_PER_SEC + ts.tv_nsec);
  }

  return ts;
}

/* timespec timespec_add(timespec ts1, timespec ts2)
 *  Returns the result of adding two timespec structures.
 */
timespec timespec_add(timespec ts1, timespec ts2) {
  /* Normalise inputs to prevent tv_nsec rollover if whole-second values
   * are packed in it.
   */
  ts1 = timespec_normalise(ts1);
  ts2 = timespec_normalise(ts2);

  ts1.tv_sec += ts2.tv_sec;
  ts1.tv_nsec += ts2.tv_nsec;

  return timespec_normalise(ts1);
}

/* timespec timespec_sub(timespec ts1, timespec ts2)
 * Returns the result of subtracting ts2 from ts1.
 */
timespec timespec_subtract(timespec ts1, timespec ts2) {
  /* Normalise inputs to prevent tv_nsec rollover if whole-second values
   * are packed in it.
   */
  ts1 = timespec_normalise(ts1);
  ts2 = timespec_normalise(ts2);

  ts1.tv_sec -= ts2.tv_sec;
  ts1.tv_nsec -= ts2.tv_nsec;

  return timespec_normalise(ts1);
}

bool timespec_negative(timespec a) {
  return a.tv_sec < 0 || (a.tv_sec == 0 && a.tv_nsec < 0);
}

bool timespec_gt(timespec x, timespec y) {
  return x.tv_sec > y.tv_sec || (x.tv_sec == y.tv_sec && x.tv_nsec > y.tv_nsec);
}

uint64_t timespec_to_nanos(timespec ts) {
  return (uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * 1e9;
}

#else

uint64_t timespec_to_nanoseconds(timespec ts) {
  return ts * 1e9 / CLOCKS_PER_SEC;
}

#endif
