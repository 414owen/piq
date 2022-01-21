#include <stdint.h>

#include "consts.h"

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

token scan(char *buf, BUF_IND_T pos);
