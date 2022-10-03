#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
  ARG_FLAG,
  ARG_STRING,
  ARG_INT,
} arg_type;

typedef struct {
  arg_type tag;
  char short_name;
  char *long_name;
  union {
    bool *flag_data;
    const char **string_data;
    int *int_data;
  };

  // Filled in by argument parser
  int long_len;
} argument;

void parse_args(argument *args, int argument_amt, int argc, const char **argv);
