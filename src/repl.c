#include <stdio.h>
#include <stdlib.h>
#include <editline.h>

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
    printf("Parsing failed.\n");
    token error_token = tres.tokens[pres.error_pos];
    format_error_ctx(stdout, input, error_token.start, error_token.end);
    putc('\n', stdout);
    goto end_b;
  }

  tc_res tc_res = typecheck(test_file, pres.tree);
  if (tc_res.errors.len > 0) {
    print_tc_errors(stdout, tc_res);
    putc('\n', stdout);
    goto end_c;
  }

  gen_and_print_module(tc_res, out);
  fflush(out);

end_c:
  VEC_FREE(&tc_res.errors);
  VEC_FREE(&tc_res.types);
  VEC_FREE(&tc_res.type_inds);
  free(tc_res.node_types);

end_b:
  free(pres.tree.inds);
  free(pres.tree.nodes);

end_a:
  free(tres.tokens);
}

int main(void) {
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
      write_history(hist_file_path);
      for (char *c = input; *c != '\0'; c += 1) {
        VEC_PUSH(&multiline_input, *c);
      }
      VEC_PUSH(&multiline_input, (char)'\n');
      free(input);
    }
  }

  VEC_FREE(&multiline_input);
  free(hist_file_path);
}
