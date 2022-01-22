#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "consts.h"
#include "source.h"
#include "term.h"
#include "token.h"

token_res scan(source_file file, BUF_IND_T pos) {
  while (isspace(file.data[pos]))
    pos++;
  token_res res = {.succeeded = true, .token = {.start = pos}};
  char c = file.data[pos];
  switch (c) {
    case '(':
      res.token.type = T_OPEN_PAREN;
      res.token.end = pos;
      break;
    case ')':
      res.token.type = T_CLOSE_PAREN;
      res.token.end = pos;
      break;
    case ',':
      res.token.type = T_COMMA;
      res.token.end = pos;
      break;
    case 'A' ... 'Z':
    case 'a' ... 'z':
      while (isalnum(file.data[pos]))
        pos++;
      pos--;
      res.token.type = T_NAME;
      res.token.end = pos;
      break;
    case '0' ... '9':
      c = file.data[pos + 1];
      while (isdigit(c)) {
        pos++;
        c = file.data[pos + 1];
      }
      res.token.type = T_INT;
      res.token.end = pos;
      break;
    case '\0':
      res.token.type = T_EOF;
      res.token.end = pos;
      break;
    default:
      res.succeeded = false;
      res.error_pos = pos;
      break;
  }
  return res;
}
