#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "consts.h"
#include "source.h"
#include "term.h"
#include "token.h"

static BUF_IND_T skip_ws(char *buf, BUF_IND_T pos) {
  while (isspace(buf[pos])) pos++;
  return pos;
}

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
} line;

// To previous '\n'
static BUF_IND_T skip_to_line_start(char *buf, BUF_IND_T pos) {
  while (pos > 0 && buf[pos] != '\n') pos--;
  return pos;
}

// To next '\n'
static BUF_IND_T skip_to_line_end(char *buf, BUF_IND_T pos) {
  char c = buf[pos];
  while (c != '\n' && c != '\0') c = buf[++pos];
  return pos;
}

static void get_error_ctx(source_file file, line *out, BUF_IND_T pos) {
  out[ERROR_LINES_CTX].start = skip_to_line_start(file.data, pos);
  out[ERROR_LINES_CTX].end = skip_to_line_end(file.data, pos);
  pos = out[ERROR_LINES_CTX].start;
  for (size_t i = 0; i < ERROR_LINES_CTX; i++) {
    if (pos == 0) {
      out[ERROR_LINES_CTX - i] = (line) {.start = 0, .end = 0};
      continue;
    }
    pos = skip_to_line_start(file.data, pos - 1);
    out[ERROR_LINES_CTX - i].start = pos;
    out[ERROR_LINES_CTX - i].end = out[ERROR_LINES_CTX - i].start;
  }
}

// SPEEDUP: this could get size before alloc
static char *get_token_error(source_file file, BUF_IND_T pos) {
  line *lines = alloca(sizeof(line) * (ERROR_LINES_CTX + 1));
  get_error_ctx(file, lines, pos);
  char *res, *new_res;
  int ignore;
  ignore = asprintf(&res, "Error parsing file %s: unrecognized token\n" RED "/---\n", file.path);
  for (size_t i = 0; i <= ERROR_LINES_CTX; i++) {
    line l = lines[i];
    if (l.start + 1 < l.end) {
      ignore = asprintf(&new_res, "%s" RED " | " RESET "%.*s\n", res, l.end - l.start - 1, &file.data[l.start]);
      free(res);
      res = new_res;
    }
  }
  ignore = asprintf(&new_res, "%s\n " RED "| " RESET "\\---\n", res);
  free(res);
  res = new_res;
  return res;
}

token_res scan(source_file file, BUF_IND_T pos) {
  pos = skip_ws(file.data, pos);
  token_res res = { .succeeded = true, .token = { .start = pos }};
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
      while (isalnum(file.data[pos])) pos++;
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
      res.error = get_token_error(file, pos);
      break;
  }
  return res;
}
