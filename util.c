#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

void *memclone(void *src, size_t bytes) {
  void *dest = malloc(bytes);
  memcpy(dest, src, bytes);
  return dest;
}

int timespec_subtract(struct timespec *result, struct timespec *x,
                      struct timespec *y) {
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_nsec < y->tv_nsec) {
    int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
    y->tv_nsec -= 1000000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_nsec - y->tv_nsec > 1000000000) {
    int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
    y->tv_nsec += 1000000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_nsec = x->tv_nsec - y->tv_nsec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void ss_init(stringstream *ss) {
  ss->stream = open_memstream(&ss->string, &ss->size);
  if (ss->stream != NULL)
    return;
  perror("Can't make string stream");
  exit(1);
}

// Won't update copies of the stringstream that was used to `init`.
void ss_finalize(stringstream *ss) {
  putc(0, ss->stream);
  fclose(ss->stream);
}

int vasprintf(char **buf, const char *restrict fmt, va_list rest) {
  stringstream ss;
  ss_init(&ss);
  int res = vfprintf(ss.stream, fmt, rest);
  ss_finalize(&ss);
  *buf = ss.string;
  return res;
}

int asprintf(char **buf, const char *restrict fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int res = vasprintf(buf, fmt, ap);
  va_end(ap);
  return res;
}

void memset_arbitrary(void *dest, void *el, size_t amt, size_t elsize) {
  while (amt--) {
    for (size_t i = 0; i < elsize; i++) {
      *((char *)dest) = ((char *)el)[i];
      dest = ((char *)dest) + 1;
    }
  }
}

void reverse_arbitrary(void *dest, size_t amt, size_t elsize) {
  if (amt == 0)
    return;
  size_t start = 0;
  size_t end = amt - 1;
  void *tmp = alloca(elsize);
  while (start < end) {
    memcpy(tmp, ((char *)dest) + elsize * start, elsize);
    memcpy(((char *)dest) + elsize * start, ((char *)dest) + elsize * end,
           elsize);
    memcpy(((char *)dest) + elsize * end, tmp, elsize);
    start++;
    end--;
  }
}

char *join(size_t str_amt, char **strs, char *sep) {
  size_t space = 0;
  size_t *lens = alloca(str_amt * sizeof(size_t));
  for (size_t i = 0; i < str_amt; i++) {
    lens[i] = strlen(strs[i]);
    space += lens[i];
  }
  size_t seplen = strlen(sep);
  space += (str_amt / 2) * seplen;
  if ((space & 1) == 1)
    space += seplen;
  char *res = malloc(space + 1);
  res[space] = '\0';
  size_t ind = 0;
  for (size_t i = 0; i < str_amt; i++) {
    if (i > 0) {
      memcpy(res + ind, sep, seplen);
      ind += seplen;
    }
    memcpy(res + ind, strs[i], lens[i]);
    ind += lens[i];
  }
  return res;
}
