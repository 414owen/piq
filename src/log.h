// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <hedley.h>
#include <stdarg.h>
#include <stdio.h>

#include "attrs.h"
#include "global_settings.h"
#include "util.h"

/**
 * Log something to stdout when verbosity is >= VERBOSE_SOME
 */
NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(1, 2)
void log_verbose(const char *restrict fmt, ...);

/**
 * Log something to stdout when verbosity is >= VERBOSE_VERY
 */
NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(1, 2)
void log_extra_verbose(const char *restrict fmt, ...);
