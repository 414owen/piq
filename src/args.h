#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
  ARG_FLAG,
  ARG_STRING,
  ARG_INT,
  ARG_SUBCOMMAND,
} arg_type;

struct argument;
typedef struct argument argument;

typedef struct {
  argument *args;
  unsigned amt;
  int subcommand_chosen;
} argument_bag;

struct argument {
  arg_type tag;
  // flag, string, int
  struct {
    // when zero, argument has no short name
    const char short_name;
    union {
      const char *long_name;
      const char *subcommand_name;
    };
    const char *description;
    union {
      bool *flag_data;
      const char **string_data;
      int *int_data;
      struct {
        argument_bag subs;
        int subcommand_value;
      };
    };
    // Filled in by argument parser
    unsigned long_len;
  };
};

typedef struct {
  argument_bag *root;
  const char *preamble;
} program_args;

void parse_args(program_args args, int argc, const char **argv);
