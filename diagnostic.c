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

static void get_error_ctx_up(char *data, line *out, BUF_IND_T pos) {
  pos = skip_to_line_start(data, pos);
  for (size_t i = 0; i < ERROR_LINES_CTX; i++) {
    if (pos == 0) {
      out[ERROR_LINES_CTX - 1 - i] = (line){.start = 0, .end = 0};
      continue;
    }
    out[ERROR_LINES_CTX - 1 - i].start = skip_to_line_start(data, pos - 1);
    out[ERROR_LINES_CTX - 1 - i].end = skip_to_line_end(data, pos - 1);
    pos = out[ERROR_LINES_CTX - 1 - i].start;
  }
}

static void get_error_ctx_down(char *data, line *out, BUF_IND_T pos) {
  pos = skip_to_line_end(data, pos);
  for (size_t i = 0; i < ERROR_LINES_CTX; i++) {
    if (data[pos] == '\0') {
      out[i] = (line){.start = pos, .end = pos};
      continue;
    }
    out[i].start = skip_to_line_start(data, pos);
    out[i].end = skip_to_line_end(data, pos);
    pos = out[i].end + 1;
  }
}

void format_error_ctx(FILE *f, char *data, BUF_IND_T start, BUF_IND_T end) {
  line *lines = alloca(sizeof(line) * ERROR_LINES_CTX);
  memset(lines, 0, sizeof(line) * ERROR_LINES_CTX);
  BUF_IND_T line_start = skip_to_line_start(data, start);
  BUF_IND_T line_end = skip_to_line_end(data, end);
  get_error_ctx_up(data, lines, line_start);
  fputs(RED "/---\n", f);
  for (size_t i = 0; i < ERROR_LINES_CTX; i++) {
    line l = lines[ERROR_LINES_CTX - 1 - i];
    if (l.end > 0 && data[l.start] != '\0') {
      fprintf(f, RED "| " RESET "%.*s\n", l.end - l.start - 1, &data[l.start]);
    }
  }
  fprintf(f, RED "| " RESET "%.*s" BLU, start - line_start, &data[line_start]);
  for (size_t i = start; i <= end; i++) {
    char c = data[i];
    switch (c) {
      case '\n':
        fputs(RED "\n| " RESET, f);
        break;
      case '\r':
        break;
      default:
        putc(c, f);
        break;
    }
  }
  fprintf(f, RESET "%.*s\n", line_end - end, &data[end + 1]);
  memset(lines, 0, sizeof(line) * ERROR_LINES_CTX);
  get_error_ctx_down(data, lines, line_end);
  for (size_t i = 0; i < ERROR_LINES_CTX; i++) {
    line l = lines[i];
    if (l.end > 0 && data[l.start] != '\0') {
      fprintf(f, RED "| " RESET "%.*s\n", l.end - l.start - 1, &data[l.start]);
    }
  }
  fputs(RED "\\---\n" RESET, f);
}
