#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mkdir.h"
#include "consts.h"
#include "dir_exists.h"
#include "util.h"

NON_NULL_PARAMS
static inline bool dir_segment_exists(char *pathbuf, size_t end) {
  // can be path separator or null terminator
  char prev = pathbuf[end];
  pathbuf[end] = '\0';
  bool res = directory_exists(pathbuf);
  pathbuf[end] = prev;
  return res;
}

// TODO really don't need to use size_t here,
// if our filenames are that long we probably have a bug
NON_NULL_PARAMS
bool mkdirp(char *path, mode_t mode) {

  size_t len = strlen(path);
  bool ends_in_sep = path[len - 1] == path_sep;
  size_t sep_amt = ends_in_sep ? 0 : 1;

  for (size_t i = 0; i < len; i++) {
    if (path[i] == path_sep)
      sep_amt++;
  }

  size_t *seps = stalloc(sep_amt * sizeof(size_t));

  {
    size_t sep_ind = 0;
    for (size_t i = 0; i < len; i++) {
      if (path[i] == path_sep)
        seps[sep_ind++] = i;
    }
    if (!ends_in_sep) {
      seps[sep_ind] = len;
    }
  }

  size_t last_created_parent = 0;

  // binary search for an ancestor that exists
  {
    size_t jmp = sep_amt >> 1;
    while (jmp > 0) {
      while (last_created_parent + jmp < sep_amt &&
             dir_segment_exists(path, seps[last_created_parent + jmp])) {
        last_created_parent += jmp;
      }
      jmp = jmp >> 1;
    }
  }

  last_created_parent++;
  while (last_created_parent < sep_amt) {
    size_t boundary = seps[last_created_parent++];
    char prev = path[boundary];
    path[boundary] = '\0';
    int rc = mkdir(path, mode);
    if (rc != 0 && errno != EEXIST)
      return false;
    path[boundary] = prev;
  }

  stfree(seps, sep_amt * sizeof(size_t));
  return true;
}
