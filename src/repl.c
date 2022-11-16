#include <stdio.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>

#include "diagnostic.h"
#include "llvm.h"
#include "parser.h"
#include "token.h"
#include "typecheck.h"
#include "util.h"

static void reply(char *input, FILE *out) {
  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    puts("Tokenization failed:\n");
    format_error_ctx(stdout, input, tres.error_pos, tres.error_pos);
    putc('\n', stdout);
    goto end_a;
  }

  parse_tree_res pres = parse(tres.tokens, tres.token_amt);
  if (!pres.succeeded) {
    token error_token = tres.tokens[pres.error_pos];
    print_parse_tree_error(stdout, input, tres.tokens, pres);
    putc('\n', stdout);
    goto end_b;
  }

  tc_res tc_res = typecheck(input, pres.tree);
  if (tc_res.error_amt > 0) {
    print_tc_errors(stdout, input, pres.tree, tc_res);
    putc('\n', stdout);
    goto end_c;
  }

  gen_and_print_module(test_file, pres.tree, tc_res.types, out);
  fflush(out);

end_c:
  free_tc_res(tc_res);

end_b:
  free_parse_tree_res(pres);

end_a:
  free_tokens_res(tres);
}

int repl(void) {
  puts("Entering multiline REPL. Press <enter> twice to evaluate.");
  char *hist_file_path = join_two_paths(get_cache_dir(), "repl_history");
  fclose(fopen(hist_file_path, "a"));
  read_history(hist_file_path);
  vec_char multiline_input = VEC_NEW;

  while (true) {
    char *input = readline("> ");
    if (!input) {
      putc('\n', stdout);
      break;
    }
    // end of multiline input
    if (strlen(input) == 0) {
      // remove newline
      if (multiline_input.len > 0) {
        VEC_POP(&multiline_input);
      }
      VEC_PUSH(&multiline_input, (char)'\0');
      char *data = VEC_DATA_PTR(&multiline_input);
      // printf("Got input: '%s'\n", data);
      reply(data, stdout);
      VEC_CLEAR(&multiline_input);
    } else {
      add_history(input);
      append_history(1, hist_file_path);
      for (char *c = input; *c != '\0'; c += 1) {
        VEC_PUSH(&multiline_input, *c);
      }
      VEC_PUSH(&multiline_input, (char)'\n');
      free(input);
    }
  }

  VEC_FREE(&multiline_input);
  free(hist_file_path);
  return 0;
}
