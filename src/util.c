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
#include "bitset.h"
#include "consts.h"
#include "util.h"
#include "vec.h"

// Returns true iff every element in as_v has an element in bs_v
// where duplicates of each matter.
NON_NULL_PARAMS
static bool test_all_as_in_b(size_t el_size, size_t el_amt,
                             const void *restrict as_v,
                             const void *restrict bs_v) {
  bool res = true;
  char *as = (char *)as_v;
  char *bs = (char *)bs_v;
  size_t bitset_chars = BITNSLOTS(el_amt);
  bitset_data used = stcalloc(1, bitset_chars);
  for (node_ind_t i = 0; i < el_amt; i++) {
    bool has_match = false;
    void *a = as + el_size * i;
    for (node_ind_t j = 0; j < el_amt; j++) {
      void *b = bs + el_size * j;
      if (memcmp(a, b, el_size) == 0 && !BITTEST(used, j)) {
        has_match = true;
        BITSET(used, j);
        break;
      }
    }
    if (!has_match) {
      res = false;
      break;
    }
  }
  stfree(used, bitset_chars);
  return res;
}

// You have two arrays you're using as multisets, for whatever reason
// O(n^2)
NON_NULL_PARAMS
bool multiset_eq(size_t el_size, size_t as_len, size_t bs_len,
                 const void *restrict as_v, const void *restrict bs_v) {
  return as_len == bs_len && test_all_as_in_b(el_size, as_len, as_v, bs_v);
}

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
size_t find_range(const void *haystack, size_t el_size, size_t el_amt,
                  const void *needle, size_t needle_els) {
  if (el_amt == 0 || needle_els > el_amt)
    return el_amt;
  for (size_t i = 0; i < el_amt - needle_els + 1; i++) {
    bool matches = true;
    for (size_t k = 0; matches && k < needle_els; k++) {
      const size_t el_ind = i + k;
      for (size_t j = 0; j < el_size; j++) {
        if (((char *)haystack)[el_ind * el_size + j] !=
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

struct timespec time_since_monotonic(const struct timespec start) {
  struct timespec end = get_monotonic_time();
  return timespec_subtract(end, start);
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
HEDLEY_DEPRECATED(1)
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
  // doesn't do anything, apart from appeasing tcc
  return ss;
}

// This is malloc-like, but it stores the thing inline, so we can't use the
// attribute :(
// TODO call by value?
HEDLEY_NON_NULL(1)
void ss_finalize(stringstream *ss) {
  putc(0, ss->stream);
  fclose(ss->stream);
}

// You're probably reading from stdin...
// We won't worry too much about the extra copy
NON_NULL_PARAMS
char *read_entire_file_no_seek(FILE *restrict f) {
  stringstream ss;
  ss_init_immovable(&ss);
  char buf[4096];
  do {
    size_t amt = fread(buf, 1, 4096, f);
    fwrite(buf, 1, amt, ss.stream);
  } while (!feof(f));
  fputc('\0', ss.stream);
  ss_finalize(&ss);
  return ss.string;
}

NON_NULL_PARAMS
char *read_entire_file(const char *restrict file_path) {
  errno = 0;
  FILE *f = fopen(file_path, "rb");
  if (errno != 0) {
    perror("Error occurred while opening file");
    exit(1);
  }
  int seek_res = fseek(f, 0, SEEK_END);
  if (seek_res != 0) {
    if (errno == ESPIPE) {
      // fall back on slower, no-seek version
      return read_entire_file_no_seek(f);
    } else {
      perror("Error occurred while seeking file");
      exit(1);
    }
  } else {
    long fsize = ftell(f);
    rewind(f);
    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    return string;
  }
}

NON_NULL_PARAMS
int vasprintf(char **buf, const char *restrict fmt, va_list rest) {
  stringstream ss;
  ss_init_immovable(&ss);
  int res = vfprintf(ss.stream, fmt, rest);
  ss_finalize(&ss);
  *buf = ss.string;
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

NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(1, 2)
char *format_to_string(const char *restrict fmt, ...) {
  char *res;
  va_list ap;
  va_start(ap, fmt);
  vasprintf(&res, fmt, ap);
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

  // Why even call me?
  if (str_amt == 0)
    return calloc(1, 1);

  stringstream ss;
  ss_init_immovable(&ss);

  fputs(strs[0], ss.stream);
  for (size_t i = 1; i < str_amt; i++) {
    fputs(sep, ss.stream);
    fputs(strs[i], ss.stream);
  }
  ss_finalize(&ss);
  return ss.string;
}

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join_two(const char *const restrict str1,
               const char *const restrict str2) {
  const char *strs[] = {str1, str2};
  return join(2, strs, "");
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
char *join_paths(const char *const *restrict paths, size_t path_num) {
  char path_sep_str[2] = {path_sep, '\0'};
  return join(path_num, paths, path_sep_str);
}

NON_NULL_ALL
MALLOC_ATTR_2(free, 1)
char *join_two_paths(const char *front, const char *back) {
  const char *const paths[2] = {front, back};
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
size_t count_char_occurences(const char *data, char needle) {
  size_t res = 0;
  char c = *data++;
  while (c != '\0') {
    if (c == needle)
      res += 1;
    c = *data++;
  }
  return res;
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
