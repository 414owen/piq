// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <hedley.h>
#include <stdarg.h>
#include <stdio.h>

#include "attrs.h"
#include "global_settings.h"
#include "util.h"

// We don't want these to be inlined, because the branch will always
// be predicted correctly after the first call, so we wouldn't get
// any extra information from inlining.

NON_NULL_PARAMS
HEDLEY_NEVER_INLINE
HEDLEY_PRINTF_FORMAT(1, 2)
void log_verbose(const char *restrict fmt, ...) {
  if (global_settings.verbosity > VERBOSE_SOME) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}

NON_NULL_PARAMS
HEDLEY_NEVER_INLINE
HEDLEY_PRINTF_FORMAT(1, 2)
void log_extra_verbose(const char *restrict fmt, ...) {
  if (global_settings.verbosity > VERBOSE_VERY) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}
