#include <hedley.h>
#include <stdio.h>

#include "args.h"
#include "repl.h"

typedef enum {
  COMMAND_NONE,
  COMMAND_REPL,
  COMMAND_COMPILE
} subcommand_tag;

int main(int argc, char **argv) {
  argument args[] = {
    {
      .tag = ARG_SUBCOMMAND,
      .long_name = "repl",
      .description = "filter tests by name. Matches on <group>.<group>.<test>",
      .subcommand_value = 
    }
  };
  
  argument_bag bag = {
    .amt = STATIC_LEN(args),
    .args = args,
  };

  parse_args(&bag, argc, argv);

}
