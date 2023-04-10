// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

int directory_exists(const char *path) {
  struct stat stats;
  return stat(path, &stats) == 0 && S_ISDIR(stats.st_mode);
}
