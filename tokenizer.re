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
  return (token_res) {.succeeded = true, .tok = {.type = type, .start = start, .end = end}};
}

token_res scan(source_file file, BUF_IND_T start) {
  while (isspace(file.data[start]))
    start++;
  BUF_IND_T pos = start;
  BUF_IND_T marker = start;

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
  lower_ident = lowerAlpha (alpha | [0-9])*;
  upper_ident = upperAlpha (alpha | [0-9])*;
  int = [0-9]+;

  // "type"  { return mk_token(TK_TYPE, start, pos - 1);        }
  // "match" { return mk_token(TK_MATCH, start, pos - 1);       }
  // "do"    { return mk_token(TK_DO, start, pos - 1)}

  str = ["]([^"\\\n] | "\\" [^\n])*["];
  str     { return mk_token(TK_STRING, start, pos - 1);         }
  "if"    { return mk_token(TK_IF, start, pos - 1);             }
  "fn"    { return mk_token(TK_FN, start, pos - 1);             }
  "fun"   { return mk_token(TK_FUN, start, pos - 1);            }
  "sig"   { return mk_token(TK_SIG, start, pos - 1);            }
  "as"    { return mk_token(TK_AS, start, pos - 1);             }
  lower_ident { return mk_token(TK_LOWER_NAME, start, pos - 1); }
  upper_ident { return mk_token(TK_UPPER_NAME, start, pos - 1); }
  "["     { return mk_token(TK_OPEN_BRACKET, start, pos - 1);   }
  "]"     { return mk_token(TK_CLOSE_BRACKET, start, pos - 1);  }

  // parsed here, because allowing spaces between parens
  // can be used to obfuscate
  "()"    { return mk_token(TK_UNIT, start, pos - 1);  }
  [(]     { return mk_token(TK_OPEN_PAREN, start, pos - 1);     }
  [)]     { return mk_token(TK_CLOSE_PAREN, start, pos - 1);    }
  [,]     { return mk_token(TK_COMMA, start, pos - 1);          }
  int     { return mk_token(TK_INT, start, pos - 1);            }
  [\x00]  { return mk_token(TK_EOF, start, pos - 1);            }
  *       { return (token_res) {.succeeded = false, .tok.start = pos - 1}; }
*/
}

tokens_res scan_all(source_file file) {
  BUF_IND_T ind = 0;
  vec_token tokens = VEC_NEW;
  tokens_res res = { .succeeded = true };
  token_res tres;
  for (;;) {
    tres = scan(file, ind);
    if (!tres.succeeded) {
      VEC_FREE(&tokens);
      res.succeeded = false;
      res.error_pos = tres.tok.start;
      return res;
    }
    VEC_PUSH(&tokens, tres.tok);
    if (tres.tok.type == TK_EOF) {
      res.token_amt = tokens.len;
      res.tokens = VEC_FINALIZE(&tokens);
      return res;
    }
    ind = tres.tok.end + 1;
  }
}
