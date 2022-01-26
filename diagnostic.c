#include <stdlib.h>
#include <string.h>

#include "consts.h"
#include "source.h"
#include "term.h"
#include "util.h"

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
} line;

// To previous '\n'
static BUF_IND_T skip_to_line_start(char *buf, BUF_IND_T pos) {
  while (pos > 0 && buf[pos] != '\n')
    pos--;
  return pos;
}

// To next '\n'
static BUF_IND_T skip_to_line_end(char *buf, BUF_IND_T pos) {
  char c = buf[pos];
  while (c != '\n' && c != '\0')
    c = buf[++pos];
  return pos;
}

static void get_error_ctx(source_file file, line *out, BUF_IND_T pos) {
  out[ERROR_LINES_CTX].start = skip_to_line_start(file.data, pos);
  out[ERROR_LINES_CTX].end = skip_to_line_end(file.data, pos);
  pos = out[ERROR_LINES_CTX].start;
  for (size_t i = 0; i < ERROR_LINES_CTX; i++) {
    if (pos == 0) {
      out[ERROR_LINES_CTX - i] = (line){.start = 0, .end = 0};
      continue;
    }
    pos = skip_to_line_start(file.data, pos - 1);
    out[ERROR_LINES_CTX - i].start = pos;
    out[ERROR_LINES_CTX - i].end = out[ERROR_LINES_CTX - i].start;
  }
}

char *stralloc(char *src) {
  char *res = malloc(strlen(src) + 1);
  strcpy(res, src);
  return res;
}

static char *format_error_ctx(char *buf, BUF_IND_T pos) {
  line *lines = alloca(sizeof(line) * (ERROR_LINES_CTX + 1));
  stringstream ss;
  ss_init(&ss);
  fprintf(ss.stream, RED "/---\n");
  for (size_t i = 0; i <= ERROR_LINES_CTX; i++) {
    line l = lines[i];
    if (l.start + 1 < l.end) {
      fprintf(ss.stream, RED "| " RESET "%.*s\n", l.end - l.start - 1,
              &buf[l.start]);
    }
  }
  fputs(RED "\\---" RESET, ss.stream);
  ss_finalize(&ss);
  return ss.string;
}

// SPEEDUP: this could get size before alloc
char *format_token_error(source_file file, BUF_IND_T pos) {
  char *ctx_str = format_error_ctx(file.data, pos);
  int ignore =
    asprintf(&res, "Error parsing file %s: unrecognized token\n%s", ctx_str);
  free(ctx_str);
  return res;
}
