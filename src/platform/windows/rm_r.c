#include "../../util.h"

int rm_r(char *dir) {
  static const char *prefix = "rmdir ";
  static const char *suffix = " /s /q";
  const char *strs[3] = {prefix, dir, suffix};
  char *cmd = join(3, strs, "");
  int res = system(cmd);
  free(cmd);
  return res;
}
