#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "consts.h"
#include "dir_exists.h"
#include "util.h"

static inline int platform_mkdir(char *pathname, mode_t mode) {
#ifdef _WIN32
  // http://msdn.microsoft.com/en-us/library/2fkk4dzw.aspx
  return _mkdir(pathname);
#else
  return mkdir(pathname, mode);
#endif
}

static inline bool dir_segment_exists(char *pathbuf, size_t end) {
  pathbuf[end] = '\0';
  bool res = directory_exists(pathbuf);
  pathbuf[end] = path_sep[0];
  return res;
}

int mkdirp(char *path, mode_t mode) {

  size_t len = strlen(path);
  size_t sep_amt = path[len - 1] == path_sep[0] ? 0 : 1;

  for (size_t i = 0; i < len; i++) {
    if (path[i] == path_sep[0])
      sep_amt++;
  }

  size_t *seps = stalloc(sep_amt * sizeof(size_t));

  {
    size_t sep_ind = 0;
    for (size_t i = 0; i < len; i++) {
      if (path[i] == path_sep[0])
        seps[sep_ind++] = i;
    }
    if (path_sep[len - 1] != path_sep[0])
      seps[sep_ind] = len - 1;
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
    path[boundary] = '\0';
    int rc = platform_mkdir(path, mode);
    if (rc != 0 && errno != EEXIST)
      return -1;
    path[boundary] = path_sep[0];
  }

  stfree(seps, sep_amt * sizeof(size_t));
  return 0;
}
