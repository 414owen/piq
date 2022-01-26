#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "consts.h"
#include "source.h"
#include "term.h"
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

static token_res mk_token(token_type type, BUF_IND_T start, BUF_IND_T end) {
  return (token_res) {.succeeded = true, .token = {.type = type, .start = start, .end = end}};
}

token_res scan(source_file file, BUF_IND_T start) {
  while (isspace(file.data[start]))
    start++;
  BUF_IND_T pos = start;

/*!re2c
  re2c:define:YYCTYPE = char;
  re2c:yyfill:enable  = 0;
  re2c:flags:input = custom;
  re2c:indent:string = "  ";
  re2c:eof = -1;
  re2c:yyfill:enable = 0;

  alpha = [a-zA-Z];
  ident = alpha (alpha | [0-9])*;
  int = [0-9]+;

  // "type"  { return mk_token(T_TYPE, start, pos - 1);          }
  // "match" { return mk_token(T_MATCH, start, pos - 1);          }
  "if"    { return mk_token(T_IF, start, pos - 1);          }
  ident   { return mk_token(T_NAME, start, pos - 1);        }
  [(]     { return mk_token(T_OPEN_PAREN, start, pos - 1);  }
  [)]     { return mk_token(T_CLOSE_PAREN, start, pos - 1); }
  [,]     { return mk_token(T_COMMA, start, pos - 1);       }
  int     { return mk_token(T_INT, start, pos - 1);         }
  [\x00]  { return mk_token(T_EOF, start, pos - 1);         }
  *       { return (token_res) {.succeeded = false, .token.start = pos - 1}; }
*/
}

tokens_res scan_all(source_file file) {
  BUF_IND_T ind = 0;
  tokens_res res = { .succeeded = true, .tokens = VEC_NEW };
  token_res tres;
  for (;;) {
    tres = scan(file, ind);
    if (!tres.succeeded)
      return (tokens_res) {
        .succeeded = false,
        .error_pos = tres.token.start,
      };
    VEC_PUSH(&res.tokens, tres.token);
    if (tres.token.type == T_EOF) {
      VEC_FINALIZE(&res.tokens);
      return res;
    }
    ind = tres.token.end + 1;
  }
}
