// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <time.h>



// platform-dependent
struct timespec get_monotonic_time(void);

struct timespec time_since_monotonic(const struct timespec start);
