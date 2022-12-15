#include <hedley.h>
#include <stdio.h>
#include <stdint.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

#include "args.h"
#include "diagnostic.h"
#include "initialise.h"
#include "llvm.h"
#include "repl.h"
#include "util.h"

typedef enum { COMMAND_NONE, COMMAND_REPL, COMMAND_COMPILE } subcommand_tag;

static const char *preamble = "Lang v0.1.0 pre-alpha\n";

typedef struct {
  bool stdin_input;
  bool no_codegen;
  const char *input_file_path;
  const char *output_file_path;
  const char *llvm_dump_path;
} compile_arguments;

static void compile_llvm(compile_arguments args) {
  LLVMInitializeNativeTarget();
  const char *restrict source_code;
  if (args.stdin_input) {
    source_code = read_entire_file_no_seek(stdin);
  } else {
    source_code = read_entire_file(args.input_file_path);
  }

  source_file file = {
    .path = args.input_file_path,
    .data = source_code,
  };

  tokens_res tres = scan_all(file);

  if (!tres.succeeded) {
    // TODO extract out to function, share with repl, maybe share with tests?
    puts("Tokenization failed:\n");
    format_error_ctx(stdout, source_code, tres.error_pos, 1);
    putc('\n', stdout);
    free_tokens_res(tres);
    return;
  }

  parse_tree_res pres = parse(tres.tokens, tres.token_amt);
  if (!pres.succeeded) {
    print_parse_tree_error(stdout, source_code, tres.tokens, pres);
    putc('\n', stdout);
    free_parse_tree_res(pres);
    return;
  }

  free_tokens_res(tres);

  tc_res tc_res = typecheck(source_code, pres.tree);
  if (tc_res.error_amt > 0) {
    print_tc_errors(stdout, source_code, pres.tree, tc_res);
    putc('\n', stdout);
    goto end_c;
  }

  LLVMContextRef ctx = LLVMContextCreate();
  // llvm_res res = gen_module("my_module", source, tree, types, ctx);
  llvm_res llvm_ir_res =
    llvm_gen_module(args.input_file_path, file, pres.tree, tc_res.types, ctx);
  free_parse_tree_res(pres);

  if (args.llvm_dump_path != NULL) {
    char *err = NULL;
    bool print_res =
      LLVMPrintModuleToFile(llvm_ir_res.module, args.llvm_dump_path, &err);
    if (print_res || err != NULL) {
      printf("Couldn't dump LLVM IR: %s\n", err != NULL ? err : "");
    }
  }
  LLVMContextDispose(ctx);

end_c:
  free_tc_res(tc_res);
}

static char *validate_compile_args(compile_arguments args) {
  if (args.stdin_input && args.input_file_path != NULL) {
    return "Can't use both a file and stdin";
  }
  if (!(args.stdin_input || args.input_file_path != NULL)) {
    return "Need an input file";
  }
  if (!args.no_codegen && args.output_file_path == NULL) {
    return "No output file given";
  }
  return NULL;
}

// Create the pass manager.
// This one corresponds to a typical -O2 optimization pipeline.
int main(const int argc, const char **argv) {

  // Parsed arguments

  compile_arguments compile_args = {
    .stdin_input = false,
    .no_codegen = false,
    .input_file_path = NULL,
    .output_file_path = NULL,
    .llvm_dump_path = NULL,
  };

  // Descriptions:

  argument compile_arg_descriptions[] = {
    {
      .tag = ARG_FLAG,
      .long_name = "stdin",
      .description = "take program input from stdin",
      .flag_data = &compile_args.stdin_input,
    },
    {
      .tag = ARG_FLAG,
      .long_name = "no-codegen",
      .description = "disable generating machine code",
      .flag_data = &compile_args.no_codegen,
    },
    {
      .tag = ARG_STRING,
      .long_name = "input",
      .short_name = 'i',
      .description = "path to input file",
      .string_data = &compile_args.input_file_path,
    },
    {
      .tag = ARG_STRING,
      .long_name = "dump-llvm",
      .description = "path to dump LLVM IR to",
      .string_data = &compile_args.llvm_dump_path,
    },
  };

  argument_bag compile_arg_bag = {
    .amt = STATIC_LEN(compile_arg_descriptions),
    .args = compile_arg_descriptions,
  };

  argument args[] = {
    {
      .tag = ARG_SUBCOMMAND,
      .long_name = "repl",
      .description = "launch the Read Evaluate Print Loop",
      .subcommand_value = COMMAND_REPL,
    },
    {
      .tag = ARG_SUBCOMMAND,
      .long_name = "compile",
      .description = "compile a file",
      .subs = compile_arg_bag,
      .subcommand_value = COMMAND_COMPILE,
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
    case COMMAND_NONE: {
      puts("Expected a subcommand.");
      print_help(pa, argc, argv);
      break;
    }
    case COMMAND_COMPILE: {
      char *validation_error = validate_compile_args(compile_args);
      if (validation_error == NULL) {
        initialise();
        compile_llvm(compile_args);
        break;
      }
      printf("%s.\nsee: %s compile --help\n", validation_error, argv[0]);
      return 1;
    }
    case COMMAND_REPL:
      initialise();
      repl();
      break;
  }
}
