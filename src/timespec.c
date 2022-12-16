#include "defs.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define NSEC_PER_SEC 1e9

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
static struct timespec timespec_normalise(struct timespec ts) {
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

/* struct timespec timespec_add(struct timespec ts1, struct timespec ts2)
 *  Returns the result of adding two timespec structures.
 */
struct timespec timespec_add(struct timespec ts1, struct timespec ts2) {
  /* Normalise inputs to prevent tv_nsec rollover if whole-second values
   * are packed in it.
   */
  ts1 = timespec_normalise(ts1);
  ts2 = timespec_normalise(ts2);

  ts1.tv_sec += ts2.tv_sec;
  ts1.tv_nsec += ts2.tv_nsec;

  return timespec_normalise(ts1);
}

/* struct timespec timespec_sub(struct timespec ts1, struct timespec ts2)
 * Returns the result of subtracting ts2 from ts1.
 */
struct timespec timespec_subtract(struct timespec ts1, struct timespec ts2) {
  /* Normalise inputs to prevent tv_nsec rollover if whole-second values
   * are packed in it.
   */
  ts1 = timespec_normalise(ts1);
  ts2 = timespec_normalise(ts2);

  ts1.tv_sec -= ts2.tv_sec;
  ts1.tv_nsec -= ts2.tv_nsec;

  return timespec_normalise(ts1);
}

bool timespec_negative(struct timespec a) {
  return a.tv_sec < 0 || (a.tv_sec == 0 && a.tv_nsec < 0);
}

bool timespec_gt(struct timespec x, struct timespec y) {
  return x.tv_sec > y.tv_sec || (x.tv_sec == y.tv_sec && x.tv_nsec > y.tv_nsec);
}
