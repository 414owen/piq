#pragma once

#include <hedley.h>
#include <alloca.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define STATIC_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef DEBUG
#define debug_assert(b) \
  if (!b) \
    fputs("Assertion failed at " ## __FILE__ ## ":" ## STRINGIZE(__LINE__); \
    abort(); \
  }
#else
#define debug_assert(b) do {} while (0);
#endif

#define MAX_STACK_ALLOC 1024
#define stalloc(bytes) \
  (((bytes) > MAX_STACK_ALLOC) ? malloc(bytes) : alloca(bytes))

#define stfree(ptr, bytes) \
  if ((bytes) > MAX_STACK_ALLOC) { free(ptr); }

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
char *join(const size_t str_amt, const char *const *const strs, const char *sep);
int vasprintf(char **, const char * restrict, va_list);
int asprintf(char **, const char *restrict, ...);
void reverse_arbitrary(void *dest, size_t amt, size_t elsize);
HEDLEY_NO_RETURN void give_up(const char *err);

char *join_paths(const char *const *paths, size_t path_num);
char *join_two_paths(const char *front, const char *back);
char *get_cache_dir(void);
