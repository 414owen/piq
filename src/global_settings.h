// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

// These are things initialized in main, that remain constant for
// the lifetime of the program
//
// Unfortunately, in C, there's no way to mark these as immutable
// after they've been initialized in main.

typedef enum {
  VERBOSE_NONE = 0,
  // print warnings
  VERBOSE_SOME = 1,
  // print info
  VERBOSE_VERY = 2,
} verbosity;

typedef struct {
  verbosity verbosity;
} global_settings_struct;

extern global_settings_struct global_settings;
