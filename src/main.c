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
      .description = "launch the Read Evaluate Print Loop",
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
  switch (root.subcommand_chosen) {
    case COMMAND_NONE:
      print_help(pa, argc, argv);
      break;
    case COMMAND_REPL:
      repl();
      break;
  }
}
