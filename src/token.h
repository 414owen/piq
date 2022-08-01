#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "consts.h"
#include "source.h"
#include "vec.h"
#include "parser.h"

typedef unsigned char token_type;

typedef struct {
  token_type type;
  BUF_IND_T start;
  BUF_IND_T end;
} token;

typedef struct {
  bool succeeded;
  token tok;
} token_res;

VEC_DECL(token);

typedef struct {
  bool succeeded;
  token *tokens;
  size_t token_amt;
  BUF_IND_T error_pos;
} tokens_res;

token_res scan(source_file file, BUF_IND_T pos);
tokens_res scan_all(source_file file);
void free_tokens_res(tokens_res res);
