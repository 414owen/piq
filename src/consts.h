// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdint.h>
#include <inttypes.h>

#define PRBI PRId32
#define ERROR_LINES_CTX 2

// TODO move to typedefs.h?
typedef uint32_t buf_ind_t;
typedef uint16_t token_len_t;
typedef uint32_t node_ind_t;
typedef uint32_t environment_ind_t;

extern const char *const program_name;
extern const char path_sep;
extern const char *issue_tracker_url;
