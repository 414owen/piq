#define _GNU_SOURCE
#include <alloca.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "consts.h"
#include "util.h"

HEDLEY_NO_RETURN
void unimplemented(char *str, char *file, size_t line) {
  fprintf(stderr,
          "%s hasn't been implemented yet.\n"
          "file: %s\n"
          "line: %zu\n"
          "Giving up.\n",
          str, file, line);
  exit(1);
}

void *memclone(void *src, size_t bytes) {
  void *dest = malloc(bytes);
  if (dest != NULL) {
    memcpy(dest, src, bytes);
  }
  return dest;
}

size_t find_el(const void *haystack, size_t haystacklen, const void *needle,
               size_t needlelen) {
  void *ptr = memmem(haystack, haystacklen, needle, needlelen);
  return (size_t)((ptr - haystack) / needlelen);
}

// TODO(speedup) Boyer-Moore, or use memmem internally?
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

stringstream *ss_init(void) {
  stringstream *ss = malloc(sizeof(stringstream));
  if (ss == NULL)
    goto err;
  ss->stream = open_memstream(&ss->string, &ss->size);
  if (ss->stream == NULL)
    goto err;
  return ss;
err:
  perror("Can't make string stream");
  exit(1);
}

// Won't update copies of the stringstream that was used to `init`.
char *ss_finalize(stringstream *ss) {
  putc(0, ss->stream);
  fclose(ss->stream);
  char *res = ss->string;
  free(ss);
  return res;
}

int vasprintf(char **buf, const char *restrict fmt, va_list rest) {
  stringstream *ss = ss_init();
  int res = vfprintf(ss->stream, fmt, rest);
  *buf = ss_finalize(ss);
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

char *join(const size_t str_amt, const char *const *const strs,
           const char *sep) {
  /*
    stringstream *ss = ss_init();
    for (size_t i = 0; i < str_amt; i++) {
      if (i > 0)
        fputs(sep, ss->stream);
      fputs(strs[i], ss->stream);
    }
    return ss_finalize(ss);
  */
  size_t len = 0;
  size_t seplen = strlen(sep);
  for (size_t i = 0; i < str_amt; i++)
    len += strlen(strs[i]);
  if (str_amt >= 2)
    len += seplen * (str_amt - 1);
  len += 1;

  char *buf = malloc(len);
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

void give_up(const char *err) {
  fprintf(stderr, "%s.\nThis is a compiler bug! Giving up.\n", err);
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

static struct passwd *get_passwd(void) {
  if (pwd_cache == NULL)
    pwd_cache = getpwuid(get_uid());
  return pwd_cache;
}

static char *get_home_dir(void) {
  char *dir = getenv("HOME");
  if (dir == NULL)
    dir = get_passwd()->pw_dir;
  return dir;
}

char *join_paths(const char *const *paths, size_t path_num) {
  return join(path_num, paths, path_sep);
}

char *join_two_paths(const char *front, const char *back) {
#define path_len 2
  const char *const paths[path_len] = {front, back};
  char *res = join_paths(paths, path_len);
#undef path_len
  return res;
}

static bool forgiving_mkdir(const char *dirname) {
  return mkdir(dirname, 0774) == 0 || errno == EEXIST;
}

// TODO change to use openat() and relative paths
bool recurse_mkdir(char *dirname) {
  bool ret = true;
  const char *p = dirname;

  while ((p = strchr(p, path_sep[0])) != NULL) {
    /* Skip empty elements. Multiple separators are okay. */
    if (p == dirname || *(p - 1) == path_sep[0]) {
      p++;
      continue;
    }
    dirname[p - dirname] = '\0';
    if (!forgiving_mkdir(dirname)) {
      ret = false;
      break;
    }
    dirname[p - dirname] = path_sep[0];
    p++;
  }
  if (!forgiving_mkdir(dirname)) {
    ret = false;
  }
  return ret;
}

static char *cache_dir_cache = NULL;
char *get_cache_dir(void) {
  if (cache_dir_cache != NULL)
    return cache_dir_cache;
  char *dir = getenv("XDG_CACHE_DIR");
  if (dir == NULL) {
    dir = get_home_dir();
#define path_len 3
    const char *const paths[path_len] = {dir, ".cache", program_name};
    dir = join_paths(paths, path_len);
#undef path_len
  } else {
    dir = join_two_paths(dir, program_name);
  }
  if (!recurse_mkdir(dir)) {
    fputs("Couldn't create cache directory. Giving up.\n", stderr);
    exit(1);
  }
  cache_dir_cache = dir;
  return dir;
}

void debug_assert_internal(bool b, const char *file, size_t line) {
  if (!b) {
    fprintf(stderr, "Assertion failed at %s:%zu\n", file, line);
    exit(1);
  }
}

bool prefix(const char *restrict pre, const char *restrict str) {
  return strncmp(pre, str, strlen(pre)) == 0;
}

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

size_t split_buf_size(char *data, int needle, size_t len) {
  return count_char(data, needle, len) + 1;
}

void split(char *data, int needle, char **buf, size_t buf_size) {
  for (size_t i = 0; i < buf_size; i++) {
    data = strchr(data, needle);
    buf[i] = data;
  }
}

void *__malloc_fill(size_t num, size_t elemsize, void *elem) {
  void *res = malloc(num * elemsize);
  memset_arbitrary(res, elem, num, elemsize);
  return res;
}
