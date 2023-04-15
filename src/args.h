// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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

  // when zero, argument has no short name
  const char short_name;

  union {
    const char *long_name;
    const char *subcommand_name;
  } names;

  const char *description;

  union {
    bool *flag_val;
    const char **string_val;
    int *int_val;
    struct {
      argument_bag subs;
      int value;
    } subcommand;
  } data;

  // Filled in by argument parser
  unsigned long_len;
};

typedef struct {
  argument_bag *root;
  const char *preamble;
} program_args;

void parse_args(program_args args, int argc, const char **argv);
void print_help(program_args program_args, int argc, const char **argv);
