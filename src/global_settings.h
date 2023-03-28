#pragma once

// These are things initialized in main, that remain constant for
// the lifetime of the program
//
// Unfortunately, in C, there's no way to mark these as immutable
// after they've been initialized in main.

typedef enum {
  VERBOSE_NONE = 0,
  VERBOSE_SOME = 1,
  VERBOSE_VERY = 2,
} verbosity;

typedef struct {
  verbosity verbosity;
} global_settings_struct;

extern global_settings_struct global_settings;
