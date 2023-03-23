#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "consts.h"
#include "defs.h"
#include "parser.h"
#include "source.h"
#include "term.h"
#include "timing.h"
#include "token.h"
#include "vec.h"

#define  YYPEEK()                 file.data[pos]
#define  YYSKIP()                 ++pos
#define  YYBACKUP()               marker = pos
#define  YYRESTORE()              pos = marker
#define  YYBACKUPCTX()            ctxmarker = pos
#define  YYRESTORECTX()           pos = ctxmarker
#define  YYRESTORETAG(tag)        pos = tag
#define  YYLESSTHAN(len)          limit - pos < len
#define  YYSTAGP(tag)             tag = pos
#define  YYSTAGN(tag)             tag = NULL
#define  YYSHIFT(shift)           cursor += shift
#define  YYSHIFTSTAG(tag, shift)  tag += shift

static token_res mk_token(token_type type, buf_ind_t start, buf_ind_t after_end) {
  token_res res = {
    .succeeded = true,
    .tok = {
      .type = type,
      .start = start,
      .len = after_end - start,
    },
  };
  return res;
}

static token_res scan(source_file file, buf_ind_t start) {
  while (isspace(file.data[start]))
    start++;
  buf_ind_t pos = start;
  buf_ind_t marker = start;

/*!re2c
  re2c:define:YYCTYPE = char;
  re2c:yyfill:enable  = 0;
  re2c:flags:input = custom;
  re2c:indent:string = "  ";
  re2c:eof = -1;
  re2c:yyfill:enable = 0;

  alpha = [a-zA-Z];
  lowerAlpha = [a-z];
  upperAlpha = [A-Z];
  digit = [0-9];
  lower_ident = lowerAlpha (alpha | digit | [?-])*;
  upper_ident = upperAlpha (alpha | digit | [-])*;
  int = [-]?[0-9]+;

  // "type"  { return mk_token(TK_TYPE, start, pos);        }
  // "match" { return mk_token(TK_MATCH, start, pos);       }
  // "do"    { return mk_token(TK_DO, start, pos)}

  str = ["]([^"\\\n] | "\\" [^\n])*["];
  str     { return mk_token(TK_STRING, start, pos);         }
  "if"    { return mk_token(TK_IF, start, pos);             }
  "Fn"    { return mk_token(TK_FN_TYPE, start, pos);             }
  "fn"    { return mk_token(TK_FN, start, pos);             }
  "fun"   { return mk_token(TK_FUN, start, pos);            }
  "sig"   { return mk_token(TK_SIG, start, pos);            }
  "let"   { return mk_token(TK_LET, start, pos);            }
  "as"    { return mk_token(TK_AS, start, pos);             }
  "data"  { return mk_token(TK_DATA, start, pos);           }
  lower_ident { return mk_token(TK_LOWER_NAME, start, pos); }
  upper_ident { return mk_token(TK_UPPER_NAME, start, pos); }
  "["     { return mk_token(TK_OPEN_BRACKET, start, pos);   }
  "]"     { return mk_token(TK_CLOSE_BRACKET, start, pos);  }

  // parsed here, because allowing spaces between parens
  // can be used to obfuscate
  "()"    { return mk_token(TK_UNIT, start, pos);  }
  [(]     { return mk_token(TK_OPEN_PAREN, start, pos);     }
  [)]     { return mk_token(TK_CLOSE_PAREN, start, pos);    }
  [,]     { return mk_token(TK_COMMA, start, pos);          }
  int     { return mk_token(TK_INT, start, pos);            }
  [\x00]  { return mk_token(TK_EOF, start, pos);            }
  *       { return (token_res) {.succeeded = false, .tok.start = start}; }
*/
}

tokens_res scan_all(source_file file) {
#ifdef TIME_TOKENIZER
  struct timespec start = get_monotonic_time();
#endif
  buf_ind_t ind = 0;
  vec_token tokens = VEC_NEW;
  tokens_res res = { .succeeded = true };
  token_res tres;
  for (;;) {
    tres = scan(file, ind);
    if (!tres.succeeded) {
      VEC_FREE(&tokens);
      res.succeeded = false;
      res.error_pos = tres.tok.start;
      break;
    }
    VEC_PUSH(&tokens, tres.tok);
    if (tres.tok.type == TK_EOF) {
      res.token_amt = tokens.len;
      res.tokens = VEC_FINALIZE(&tokens);
      break;
    }
    ind = tres.tok.start + tres.tok.len;
  }
#ifdef TIME_TOKENIZER
  res.time_taken = time_since_monotonic(start);
#endif
  return res;
}

void free_tokens_res(tokens_res res) {
  free(res.tokens);
}
