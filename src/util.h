#pragma once

#include <hedley.h>
#include <predef/predef.h>
#include <alloca.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#include "attrs.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define STATIC_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef DEBUG
#define debug_assert(b) debug_assert_internal(b, __FILE__, __LINE__)
#else
#define debug_assert(b)                                                        \
  do {                                                                         \
  } while (0);
#endif

#define UNIMPLEMENTED(str) unimplemented(str, __FILE__, __LINE__)

#define MAX_STACK_ALLOC 1024

// Haven't figured out how to get alloca working in tcc
#if ((defined(__GNUC__) || defined(__clang__)))

#define stalloc(bytes)                                                         \
  (((bytes) > MAX_STACK_ALLOC) ? malloc_safe(bytes) : alloca(bytes))

#define stcalloc(n, size)                                                      \
  (((n) * (size) > MAX_STACK_ALLOC)                                            \
     ? calloc((n), (size))                                                     \
     : memset(alloca(n * size), 0, (n) * (size)))

#define stfree(ptr, bytes)                                                     \
  if ((bytes) > MAX_STACK_ALLOC) {                                             \
    free(ptr);                                                                 \
  }

#else

#define stalloc(bytes) malloc(bytes)

#define stcalloc(n, size) calloc((n), (size))

#define stfree(ptr, bytes) free(ptr);

#endif

#define give_up(...) give_up_internal(__FILE__, __LINE__, __VA_ARGS__)

typedef struct {
  char *string;
  FILE *stream;
  size_t size;
} stringstream;

typedef enum {
  LT = -1,
  EQ = 0,
  GT = 1,
} order;

HEDLEY_RETURNS_NON_NULL
MALLOC_ATTR_2(free, 1)
void *malloc_safe(size_t bytes);

NON_NULL_PARAMS
size_t find_el(const void *haystack, size_t haystacklen, const void *needle,
               size_t needlelen);

NON_NULL_PARAMS
size_t find_range(const void *haystack, size_t el_size, size_t el_amt,
                  const void *needle, size_t needle_els);

NON_NULL_PARAMS
void ss_init_immovable(stringstream *ss);

HEDLEY_NON_NULL(1)
void ss_finalize(stringstream *ss);

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *ss_finalize_free(stringstream *ss);

MALLOC_ATTR_2(ss_finalize_free, 1)
HEDLEY_RETURNS_NON_NULL
stringstream *ss_init(void);

HEDLEY_NO_RETURN
NON_NULL_PARAMS
COLD_ATTR
void unimplemented(char *str, char *file, size_t line);

bool timespec_negative(struct timespec a);

struct timespec timespec_subtract(const struct timespec x,
                                  const struct timespec y);

struct timespec timespec_add(const struct timespec x, const struct timespec y);

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
void *memclone(void *src, size_t bytes);

HEDLEY_NON_NULL(1, 2)
NON_NULL_PARAMS
void memset_arbitrary(void *dest, void *el, size_t amt, size_t elsize);

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join(const size_t str_amt, const char *const *const strs,
           const char *sep);

NON_NULL_PARAMS
int vasprintf(char **, const char *restrict, va_list);

NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(2, 3)
int asprintf(char **, const char *restrict, ...);

NON_NULL_PARAMS
void reverse_arbitrary(void *dest, size_t amt, size_t elsize);

HEDLEY_NO_RETURN
HEDLEY_PRINTF_FORMAT(3, 4)
COLD_ATTR
void give_up_internal(const char *file, size_t line, const char *err, ...);

NON_NULL_PARAMS
void debug_assert_internal(bool b, const char *file, size_t line);

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join_paths(const char *const *paths, size_t path_num);

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join_two_paths(const char *front, const char *back);

HEDLEY_RETURNS_NON_NULL
char *get_cache_dir(void);

NON_NULL_PARAMS
bool mkdirp(char *path, mode_t mode);

NON_NULL_PARAMS
int directory_exists(const char *path);

NON_NULL_PARAMS
int rm_r(char *dir);

NON_NULL_PARAMS
bool prefix(const char *restrict pre, const char *restrict str);

NON_NULL_PARAMS
size_t count_char_occurences(const char *restrict data, char needle);

NON_NULL_PARAMS
void split(char *data, int needle, char **buf, size_t buf_size);

#define malloc_fill(num, el) __malloc_fill((num), sizeof(*el), (el))
NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
void *__malloc_fill(size_t num, size_t elemsize, void *elem);

void initialize_util(void);

// platform-dependent
struct timespec get_monotonic_time(void);

struct timespec time_since_monotonic(const struct timespec start);

#ifdef PREDEF_OS_WINDOWS
#include "platform/windows/mkdir.h"
#endif
