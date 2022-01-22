#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "consts.h"
#include "source.h"

typedef enum {
  T_OPEN_PAREN,
  T_CLOSE_PAREN,
  T_NAME,
  T_INT,
  T_COMMA,
  T_EOF,
} token_type;

typedef struct {
  token_type type;
  BUF_IND_T start;
  BUF_IND_T end;
} token;

typedef struct {
  bool succeeded;
  union {
    token token;
    BUF_IND_T error_pos;
  };
} token_res;

token_res scan(source_file file, BUF_IND_T pos);
