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

void format_error_ctx(FILE *f, const char *restrict data, BUF_IND_T start,
                      BUF_IND_T end) {
  fprintf(f, "%d-%d\n", start, end);
  fputs(RED "/---\n| " RESET, f);
  size_t cursor = start;
  {
    size_t nls_wanted = ERROR_LINES_CTX + 1;
    while (nls_wanted > 0 && cursor > 0) {
      if (data[cursor] == '\n')
        nls_wanted--;
      cursor--;
    }
  }

  while (cursor < start) {
    char c = data[cursor];
    if (c == '\n') {
      fputs(RED "\n| " RESET, f);
    } else {
      fputc(c, f);
    }
    cursor++;
  }

  fputs(BLU, f);
  while (cursor <= end) {
    char c = data[cursor];
    if (c == '\n') {
      fputs(RED "\n| " BLU, f);
    } else {
      fputc(c, f);
    }
    cursor++;
  }

  fputs(RESET, f);
  {
    size_t nls_wanted = ERROR_LINES_CTX + 1;
    char c = data[cursor];
    while (nls_wanted > 0) {
      char c = data[cursor];
      switch (c) {
        case '\0':
          goto end;
        case '\n':
          fputs(RED "\n| " RESET, f);
          break;
        default:
          fputc(c, f);
          break;
      }
      c = data[++cursor];
    }
  }

end:
  fputs(RED "\n\\---" RESET, f);
}