#include "../../util.h"

int rm_r(char *dir) {
  static const char *prefix = "rm -rf ";
  const char *strs[2] = {prefix, dir};
  char *cmd = join(2, strs, "");
  int res = system(cmd);
  free(cmd);
  return res;
}
