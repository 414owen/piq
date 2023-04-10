// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>

#include "token.h"
#include "util.h"

extern char *yyTokenName[];

void print_token(FILE *f, token_type t) { fputs(yyTokenName[t], f); }

void print_tokens(FILE *f, const token_type *restrict tokens,
                  unsigned token_amt) {
  fputc('[', f);
  for (unsigned i = 0; i < token_amt; i++) {
    if (i > 0) {
      fputs(", ", f);
    }
    print_token(f, tokens[i]);
  }
  fputc(']', f);
}

MALLOC_ATTR_2(free, 1)
char *print_tokens_str(const token_type *restrict tokens, unsigned token_amt) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_tokens(ss.stream, tokens, token_amt);
  ss_finalize(&ss);
  return ss.string;
}
