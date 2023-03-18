#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "defs.h"
#include "consts.h"
#include "source.h"
#include "vec.h"

#define TOKENS \
  X(EOF) \
  X(AS) \
  X(CLOSE_BRACKET) \
  X(CLOSE_PAREN) \
  X(COMMA) \
  X(FN) \
  X(FN_TYPE) \
  X(FUN) \
  X(IF) \
  X(INT) \
  X(LOWER_NAME) \
  X(OPEN_BRACKET) \
  X(OPEN_PAREN) \
  X(SIG) \
  X(UPPER_NAME) \
  X(LET) \
  X(DATA) \
  X(STRING) \
  X(UNIT) \

#define X(a) TKN ## _ ## a,
typedef enum {
  TOKENS
} token_type;
#undef X

VEC_DECL(token_type);

extern const char *tokenNames[];

typedef struct {
  token_type type;
  token_len_t len;
  buf_ind_t start;
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

tokens_res scan_all(source_file file);
void free_tokens_res(tokens_res res);
void print_token(FILE *f, token_type t);
void print_tokens(FILE *f, const token_type *tokens, unsigned token_amt);
char *print_tokens_str(const token_type *tokens, unsigned token_amt);
