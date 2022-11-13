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
    char short_name;
    union {
      char *long_name;
      char *subcommand_name;
    };
    char *description;
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

void parse_args(argument_bag *bag, int argc, const char **argv);
