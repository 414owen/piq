#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>

#include "llvm.h"
#include "parser.h"
#include "token.h"
#include "typecheck.h"
#include "util.h"

static void reply(char *input, FILE *out) {
  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    printf("Parser test \"%s\" failed tokenization at position %d.\n", input,
           tres.error_pos);
    goto end_a;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (!pres.succeeded) {
    printf("Parsing \"%s\" failed.\n", input);
    goto end_b;
  }

  tc_res tc_res = typecheck(test_file, pres.tree);
  if (tc_res.errors.len > 0) {
    printf("Typechecking \"%s\" failed.\n", input);
    goto end_c;
  }

  gen_and_print_module(tc_res, out);
  fflush(out);

end_c:
  VEC_FREE(&tc_res.errors);
  VEC_FREE(&tc_res.types);
  VEC_FREE(&tc_res.type_inds);
  VEC_FREE(&tc_res.node_types);

end_b:
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
}

int main(void) {
  const char *hist_file_path = join_two_paths(get_cache_dir(), "repl_history");
  fclose(fopen(hist_file_path, "a"));
  read_history(hist_file_path);
  while (true) {
    char *input = readline("> ");
    add_history(input);
    if (!input) {
      putc('\n', stdout);
      break;
    }
    printf("Got input: %s\n", input);
    append_history(1, hist_file_path);
    reply(input, stdout);
    free(input);
  }
}
