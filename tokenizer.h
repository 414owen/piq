#include <stdint.h>

#define BUF_IND_T uint32_t

typedef enum {
  T_OPEN_PAREN,
  T_CLOSE_PAREN,
  T_NAME,
  T_INT,
  T_COMMA,
} token_type;

typedef struct {
  token_type type;
  BUF_IND_T start;
  BUF_IND_T end;
} token;

typedef enum {
  A_PAREN_FORM,
  A_BRACE_FORM,
  A_BRACKET_FORM,
  A_NAME,
  A_NUM,
} parse_ast_type;

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
  struct parse_ast *sub;
  uint16_t sub_num;
  parse_ast_type type;
} parse_ast;

token scan(char *buf, BUF_IND_T pos);
