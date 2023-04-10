// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#ifdef PREDEF_OS_WINDOWS
#include "platform/windows/mkdir.h"
#else
#include <sys/stat.h>
#endif
