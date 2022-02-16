#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define STATIC_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
  char *string;
  FILE *stream;
  size_t size;
} stringstream;

size_t find_el(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
size_t find_range(const void *haystack, size_t el_size, size_t el_amt, const void *needle, size_t needle_els);

stringstream *ss_init(void);
char *ss_finalize(stringstream *ss);

int timespec_subtract(struct timespec *result, struct timespec *x, struct timespec *y);
void *memclone(void *src, size_t bytes);
void memset_arbitrary(void *dest, void* el, size_t amt, size_t elsize);
char *join(size_t str_amt, char **strs, char *sep);
int vasprintf(char **, const char * restrict, va_list);
int asprintf(char **, const char *restrict, ...);
void reverse_arbitrary(void *dest, size_t amt, size_t elsize);