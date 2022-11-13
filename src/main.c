#include <hedley.h>
#include <stdio.h>

#include "args.h"
#include "repl.h"
#include "util.h"

typedef enum {
  COMMAND_NONE,
  COMMAND_REPL,
  COMMAND_COMPILE
} subcommand_tag;

const char *preamble = "Lang v0.1.0 pre-alpha\n";

int main(const int argc, const char **argv) {
  argument args[] = {
    {
      .tag = ARG_SUBCOMMAND,
      .long_name = "repl",
      .description = "filter tests by name. Matches on <group>.<group>.<test>",
      .subcommand_value = COMMAND_REPL,
    },
  };
  
  argument_bag root = {
    .amt = STATIC_LEN(args),
    .args = args,
    .subcommand_chosen = COMMAND_NONE,
  };
  
  program_args pa = {
    .root = &root,
    .preamble = preamble,
  };

  parse_args(pa, argc, argv);
  printf("%d\n", root.subcommand_chosen);
}
