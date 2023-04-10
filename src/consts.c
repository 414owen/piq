// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <predef/predef.h>

const char *const program_name = "lang";

#if PREDEF_OS_WINDOWS
const char path_sep = '\\';
#else
const char path_sep = '/';
#endif

const char *issue_tracker_url = "https://github.com/414owen/piq/issues";
