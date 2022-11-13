#include <alloca.h>
#include <errno.h>
#include <predef/predef.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "attrs.h"
#include "consts.h"
#include "util.h"

void *memmem(const void *haystack, size_t haystack_len, const void *needle,
             size_t needle_len);

HEDLEY_RETURNS_NON_NULL
MALLOC_ATTR_2(free, 1)
void *malloc_safe(size_t size) {
  void *res = malloc(size);
  if (res == NULL) {
    fputs("Couldn't allocate memory", stderr);
    exit(1);
  }
  return res;
}

HEDLEY_NO_RETURN
NON_NULL_PARAMS
COLD_ATTR
void unimplemented(char *str, char *file, size_t line) {
  fprintf(stderr,
          "%s hasn't been implemented yet.\n"
          "file: %s\n"
          "line: %zu\n"
          "Giving up.\n",
          str,
          file,
          line);
  exit(1);
}

HEDLEY_NON_NULL(1)
MALLOC_ATTR_2(free, 1)
void *memclone(void *restrict src, size_t bytes) {
  void *dest = malloc_safe(bytes);
  memcpy(dest, src, bytes);
  return dest;
}

NON_NULL_PARAMS
size_t find_el(const void *haystack, size_t haystacklen, const void *needle,
               size_t needlelen) {
  void *ptr = memmem(haystack, haystacklen, needle, needlelen);
  return (size_t)((ptr - haystack) / needlelen);
}

// TODO(speedup) Boyer-Moore, or use memmem internally?
NON_NULL_PARAMS
size_t find_range(const void *haystack, size_t el_size, size_t el_amt,
                  const void *needle, size_t needle_els) {
  if (el_amt == 0 || needle_els > el_amt)
    return el_amt;
  for (size_t i = 0; i < el_amt - needle_els + 1; i++) {
    bool matches = true;
    for (size_t k = 0; matches && k < needle_els; k++) {
      for (size_t j = 0; j < el_size; j++) {
        if (((char *)haystack)[(i + k) * el_size + j] !=
            ((char *)needle)[k * el_size + j]) {
          matches = false;
          break;
        }
      }
    }
    if (matches) {
      return i;
    }
  }
  return el_amt;
}

static struct timespec timespec_subtract_internal(const struct timespec x,
                                                  const struct timespec y) {
  struct timespec result = {
    .tv_sec = x.tv_sec - y.tv_sec,
    .tv_nsec = x.tv_nsec - y.tv_nsec,
  };
  if (x.tv_nsec < y.tv_nsec) {
    result.tv_sec -= 1L;
    result.tv_nsec += 1e9L;
  }
  return result;
}

static bool timespec_gt(const struct timespec x, const struct timespec y) {
  return x.tv_sec > y.tv_sec || (x.tv_sec == y.tv_sec && x.tv_nsec > y.tv_nsec);
}

bool timespec_negative(const struct timespec ts) {
  struct timespec zero = {0};
  return timespec_gt(zero, ts);
}

struct timespec timespec_subtract(const struct timespec x,
                                  const struct timespec y) {
  struct timespec result;
  if (timespec_gt(x, y)) {
    return timespec_subtract_internal(x, y);
  } else {
    struct timespec res = timespec_subtract_internal(y, x);
    res.tv_sec *= -1;
    res.tv_nsec *= -1;
    return res;
  }
}

NON_NULL_PARAMS
void ss_init_immovable(stringstream *ss) {
  ss->stream = open_memstream(&ss->string, &ss->size);
}

// Won't update copies of the stringstream that was used to `init`.
NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *ss_finalize_free(stringstream *ss) {
  ss_finalize(ss);
  char *res = ss->string;
  free(ss);
  return res;
}

MALLOC_ATTR_2(ss_finalize_free, 1)
HEDLEY_RETURNS_NON_NULL
stringstream *ss_init(void) {
  stringstream *ss = malloc_safe(sizeof(stringstream));
  if (ss == NULL)
    goto err;
  ss_init_immovable(ss);
  if (ss->stream == NULL)
    goto err;
  return ss;
err:
  perror("Can't make string stream");
  exit(1);
}

// This is malloc-like, but it stores the thing inline, so we can't use the
// attribute :(
HEDLEY_NON_NULL(1)
void ss_finalize(stringstream *ss) {
  putc(0, ss->stream);
  fclose(ss->stream);
}

NON_NULL_PARAMS
int vasprintf(char **buf, const char *restrict fmt, va_list rest) {
  stringstream *ss = ss_init();
  int res = vfprintf(ss->stream, fmt, rest);
  *buf = ss_finalize_free(ss);
  return res;
}

NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(2, 3)
int asprintf(char **buf, const char *restrict fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int res = vasprintf(buf, fmt, ap);
  va_end(ap);
  return res;
}

HEDLEY_NON_NULL(1, 2)
NON_NULL_PARAMS
void memset_arbitrary(void *restrict dest, void *restrict el, size_t amt,
                      size_t elsize) {
  while (amt--) {
    for (size_t i = 0; i < elsize; i++) {
      *((char *)dest) = ((char *)el)[i];
      dest = ((char *)dest) + 1;
    }
  }
}

// TODO
// If this is specialized on elsize, the memcpy calls will actually be
// inlined with the __builtin_memcpy machinery
// would the `flatten` attribute help?
NON_NULL_PARAMS
void reverse_arbitrary(void *dest, size_t amt, size_t elsize) {
  if (amt == 0)
    return;
  size_t start = 0;
  size_t end = amt - 1;
  void *tmp = stalloc(elsize);
  while (start < end) {
    memcpy(tmp, ((char *)dest) + elsize * start, elsize);
    memcpy(
      ((char *)dest) + elsize * start, ((char *)dest) + elsize * end, elsize);
    memcpy(((char *)dest) + elsize * end, tmp, elsize);
    start++;
    end--;
  }
  stfree(tmp, elsize);
}

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join(const size_t str_amt, const char *const *restrict const strs,
           const char *restrict sep) {
  size_t len = 0;
  size_t seplen = strlen(sep);
  for (size_t i = 0; i < str_amt; i++)
    len += strlen(strs[i]);
  if (str_amt >= 2)
    len += seplen * (str_amt - 1);
  len += 1;

  char *buf = malloc_safe(len);
  buf[len - 1] = '\0';

  if (str_amt > 0) {
    size_t ind = 0;
    {
      const char *s = strs[0];
      size_t slen = strlen(s);
      memcpy(&buf[ind], s, slen);
      ind += slen;
    }
    for (size_t i = 1; i < str_amt; i++) {
      const char *s = strs[i];
      size_t slen = strlen(s);
      memcpy(&buf[ind], sep, seplen);
      ind += seplen;
      memcpy(&buf[ind], s, slen);
      ind += slen;
    }
  }
  return buf;
}

HEDLEY_NO_RETURN
HEDLEY_PRINTF_FORMAT(3, 4)
COLD_ATTR
void give_up_internal(const char *file, size_t line, const char *err, ...) {
  va_list argp;
  va_start(argp, err);
  fprintf(stderr, "%s:%zu", file, line);
  vfprintf(stderr, err, argp);
  fputs("\nThis is a compiler bug! Giving up.\n", stderr);
  exit(1);
}

static uid_t uid_cache;
static bool uid_populated = false;

static uid_t get_uid(void) {
  if (!uid_populated)
    uid_cache = getuid();
  return uid_cache;
}

static struct passwd *pwd_cache = NULL;

// doesn't need freeing
// can return null
static struct passwd *get_passwd(void) {
  if (pwd_cache == NULL)
    pwd_cache = getpwuid(get_uid());
  return pwd_cache;
}

// doesn't need freeing
// can return null (presumably)
// not cached
static char *get_home_dir(void) {
  char *dir = getenv("HOME");
  if (dir == NULL)
    dir = get_passwd()->pw_dir;
  return dir;
}

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join_paths(const char *const *paths, size_t path_num) {
  return join(path_num, paths, path_sep);
}

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join_two_paths(const char *front, const char *back) {
  const char *const paths[] = {front, back};
  char *res = join_paths(paths, STATIC_LEN(paths));
  return res;
}

// NON_NULL_PARAMS
// static bool forgiving_mkdir(const char *dirname) {
//  return mkdir(dirname, 0774) == 0 || errno == EEXIST;
// }

static char *cache_dir_cache = NULL;
HEDLEY_RETURNS_NON_NULL
char *get_cache_dir(void) {
  if (cache_dir_cache != NULL)
    return cache_dir_cache;
  char *dir = getenv("XDG_CACHE_DIR");
  if (dir == NULL) {
    dir = get_home_dir();
    const char *const paths[] = {dir, ".cache", program_name};
    dir = join_paths(paths, STATIC_LEN(paths));
  } else {
    dir = join_two_paths(dir, program_name);
  }
  if (!mkdirp(dir, S_IRWXU | S_IRWXG | S_IROTH)) {
    fputs("Couldn't create cache directory. Giving up.\n", stderr);
    exit(1);
  }
  cache_dir_cache = dir;
  return dir;
}

NON_NULL_PARAMS
void debug_assert_internal(bool b, const char *file, size_t line) {
  if (!b) {
    fprintf(stderr, "Assertion failed at %s:%zu\n", file, line);
    exit(1);
  }
}

NON_NULL_PARAMS
bool prefix(const char *restrict pre, const char *restrict str) {
  return strncmp(pre, str, strlen(pre)) == 0;
}

NON_NULL_PARAMS
size_t count_char(char *data, int needle, size_t len) {
  size_t res = 0;
  char *cursor = data;
  data += len;
  while (true) {
    cursor = strchr(cursor, needle);
    if (cursor && cursor < data)
      res += 1;
    else
      break;
  }
  return res;
}

NON_NULL_PARAMS
size_t split_buf_size(char *data, int needle, size_t len) {
  return count_char(data, needle, len) + 1;
}

NON_NULL_PARAMS
void split(char *data, int needle, char **buf, size_t buf_size) {
  for (size_t i = 0; i < buf_size; i++) {
    data = strchr(data, needle);
    buf[i] = data;
  }
}

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
void *__malloc_fill(size_t num, size_t elemsize, void *elem) {
  void *res = malloc_safe(num * elemsize);
  memset_arbitrary(res, elem, num, elemsize);
  return res;
}

#ifdef PREDEF_OS_WINDOWS
#include "platform/windows/util.c"
#include "platform/windows/rm_r.c"
#elif defined(PREDEF_STANDARD_POSIX_2001)
#include "platform/posix/util.c"
#include "platform/posix/rm_r.c"
#endif

void initialise_util(void) {
#ifdef PREDEF_OS_WINDOWS
  initialise_util_windows();
#endif
}
