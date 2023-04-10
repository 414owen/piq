// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdlib.h>
#include <time.h>

#include "llvm.h"
#include "types.h"
#include "util.h"

// I'm gonna be compiling things. I neeed everything to be set up.
void initialise(void) {
  srand(time(NULL));
  initialise_util();
  llvm_init();
}
