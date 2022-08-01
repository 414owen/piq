#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

int directory_exists(const char *path) {
  struct stat stats;
  return stat(path, &stats) == 0 && S_ISDIR(stats.st_mode);
}
