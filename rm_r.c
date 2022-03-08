#include "util.h"

// We only use it to remove test files
int rm_r(char *dir) {
#ifdef _WIN32
  static const char *prefix = "rmdir ";
  static const char *suffix = " /s /q";
#define str_amt 3
  const char *strs[str_amt] = {prefix, dir, suffix};
  char *cmd = join(str_amt, strs, "");
#undef str_amt
#else
  static const char *prefix = "rm -rf ";
#define str_amt 2
  const char *strs[str_amt] = {prefix, dir};
  char *cmd = join(str_amt, strs, "");
#undef str_amt
#endif
  int res = system(cmd);
  free(cmd);
  return res;
}
