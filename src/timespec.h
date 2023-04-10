// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <time.h>

bool timespec_negative(struct timespec a);

bool timespec_gt(struct timespec x, struct timespec y);

struct timespec timespec_subtract(const struct timespec x,
                                  const struct timespec y);

struct timespec timespec_add(const struct timespec x, const struct timespec y);
