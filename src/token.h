#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "defs.h"
#include "consts.h"
#include "source.h"
#include "vec.h"

typedef unsigned char token_type;

VEC_DECL(token_type);

typedef struct {
  token_type type;
  buf_ind_t start;
  buf_ind_t end;
} token;

typedef struct {
  bool succeeded;
  token tok;
} token_res;

VEC_DECL(token);

typedef struct {
  token *tokens;
  size_t token_amt;
  buf_ind_t error_pos;
  bool succeeded;
#ifdef TIME_TOKENIZER
  struct timespec time_taken;
#endif
} tokens_res;

token_res scan(source_file file, buf_ind_t pos);
tokens_res scan_all(source_file file);
void free_tokens_res(tokens_res res);
void print_token(FILE *f, token_type t);
void print_tokens(FILE *f, const token_type *tokens, unsigned token_amt);
char *print_tokens_str(const token_type *tokens, unsigned token_amt);
