#include <stdint.h>
#include <ctype.h>

#include "token.h"

static BUF_IND_T skip_ws(char *buf, BUF_IND_T pos) {
  while (isspace(buf[pos])) {}
  return pos;
}

token scan(char *buf, BUF_IND_T pos) {
  pos = skip_ws(buf, pos);
  token res = {.start = pos};
  char c = buf[pos];
  switch (c) {
    case '(':
      res.type = T_OPEN_PAREN;
      res.end = pos;
      break;
    case ')':
      res.type = T_CLOSE_PAREN;
      res.end = pos;
       break;
    case 'A' ... 'Z':
    case 'a' ... 'z':
      c = buf[pos + 1];
      while (isalnum(c)) {
        pos++;
        c = buf[pos + 1];
      }
      res.type = T_NAME;
      res.end = pos;
      break;
    case '0' ... '9':
      c = buf[pos + 1];
      while (isdigit(c)) {
        pos++;
        c = buf[pos + 1];
      }
      res.type = T_INT;
      res.end = pos;
      break;
  }
  return res;
}
