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

int main(const int argc, const char **argv) {
  argument args[] = {
    {
      .tag = ARG_SUBCOMMAND,
      .long_name = "repl",
      .description = "filter tests by name. Matches on <group>.<group>.<test>",
      .subcommand_value = COMMAND_REPL,
    },
  };
  
  argument_bag bag = {
    .amt = STATIC_LEN(args),
    .args = args,
  };

  parse_args(&bag, argc, argv);

}
