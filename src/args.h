#pragma once

#include <stdbool.h>

typedef enum {
  ARG_FLAG,
  ARG_STRING,
} arg_type;

typedef struct {
  arg_type tag;
  char short_name;
  char *long_name;
  union {
    bool *flag_data;
    const char **string_data;
  };
} argument;
