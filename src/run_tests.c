#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "args.h"
#include "benchmark.h"
#include "initialise.h"
#include "llvm.h"
#include "test.h"
#include "tests.h"

const char *byte_suffixes[] = {"B", "KB", "MB", "GB", "TB"};
const long mega = 1e6;

// TODO move to utils, if we need it again
static void print_byte_amount(FILE *f, uint64_t bytes) {
  unsigned long i = 0;
  double dblBytes = bytes;

  if (bytes > 1024) {
    for (i = 0; (bytes / 1024) > 0 && i < STATIC_LEN(byte_suffixes) - 1;
         i++, bytes /= 1024) {
      dblBytes = bytes / 1024.0;
    }
  }

  fprintf(f, "%.02lf %s", dblBytes, byte_suffixes[i]);
}

const char *qualtity_suffixes[] = {"", "thousand", "million", "billion"};

static void print_amount(FILE *f, uint64_t amt) {
  stringstream ss;
  ss_init_immovable(&ss);
  fprintf(ss.stream, "%" PRIu64, amt);
  ss_finalize(&ss);
  size_t len = strlen(ss.string);
  for (size_t i = 0; i < len; i++) {
    if (i > 0 && (len - i) % 3 == 0) {
      putc('_', f);
    }
    putc(ss.string[i], f);
  }
  free(ss.string);
}

static uint64_t timespec_to_nanos(struct timespec ts) {
  return (uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * 1e9;
}

static void print_timespan_nanos(FILE *f, uint64_t ns) {
  if (ns < 1000) {
    fprintf(f, "%luns", ns);
    return;
  }
  static const char *time_suffixes[] = {
    "µ",
    "m",
    "",
  };
  uint64_t before = ns / 1000;
  uint64_t after = ns % 1000;
  for (unsigned i = 0; i < STATIC_LEN(time_suffixes); i++) {
    if (before < 1000) {
      fprintf(
        f, "%" PRIu64 ".%03" PRIu64 "%ss", before, after, time_suffixes[i]);
      return;
    }
    after = before % 1000;
    before /= 1000;
  }
}

static void print_timespan_timespec(FILE *f, struct timespec ts) {
  print_timespan_nanos(f, timespec_to_nanos(ts));
}

static void run_tests(test_state *state) {
  test_vec(state);
  test_bitset(state);
  test_strint(state);
  test_utils(state);
  test_diagnostics(state);
  test_scanner(state);
  test_parse_tree(state);
  test_parser(state);
  test_traverse(state);
  test_typecheck(state);
  test_ir(state);
  test_semantics(state);
  test_llvm(state);
}

HEDLEY_NEVER_INLINE
static void newline(FILE *f) { putc('\n', f); }

enum {
  SUB_NONE = -1,
  SUB_BENCH = 0,
} subcommand;

int main(int argc, const char **argv) {
  initialise();
  test_config conf = {
    .junit = false,
    .bench = false,
    .lite = false,
    .filter_str = NULL,
    .times = 1,
  };

  argument args[] = {
    [SUB_BENCH] = {
      .tag = ARG_SUBCOMMAND,
      .subcommand_name = "bench",
      .short_name = 0,
      .flag_data = &conf.bench,
      .description = "Run benchmark suite",
    },
    {
      .tag = ARG_FLAG,
      .long_name = "lite",
      .short_name = 'l',
      .flag_data = &conf.lite,
      .description = "Turn off stress tests",
    },
    {.tag = ARG_FLAG,
     .long_name = "junit",
     .short_name = 'j',
     .flag_data = &conf.junit,
     .description = "Create JUnit compatible test-results.xml file"},
    {
      .tag = ARG_INT,
      .long_name = "times",
      .short_name = 't',
      .int_data = &conf.times,
      .description = "Run the test suite more than once (for benchmarking)",
    },
    {
      .tag = ARG_STRING,
      .long_name = "match",
      .short_name = 'm',
      .string_data = &conf.filter_str,
      .description = "filter tests by name. Matches on <group>.<group>.<test>",
    },
  };

  argument_bag root = {
    .amt = STATIC_LEN(args),
    .args = args,
    .subcommand_chosen = SUB_NONE,
  };

  program_args pa = {
    .root = &root,
    .preamble = "Lang tests v1.0.0\n",
  };

  parse_args(pa, argc, argv);

  test_state state = test_state_new(conf);

  llvm_init();

  switch (root.subcommand_chosen) {
    case SUB_NONE:
      run_tests(&state);
      break;
    case SUB_BENCH:
      run_benchmarks(&state);
      break;
  }

  test_state_finalize(&state);

  print_failures(&state);

  printf("Tests passed: %" PRIu32 "/%" PRIu32 "\n",
         state.tests_passed,
         state.tests_run);

#ifdef TIME_ANY
  vec_string timing_blocks = VEC_NEW;

  puts("\n--- Timings:\n");
#endif

#ifdef TIME_TOKENIZER
  if (state.total_bytes_tokenized > 0) {
    stringstream timing_ss;
    ss_init_immovable(&timing_ss);

    fputs("Time spent tokenizing: ", timing_ss.stream);
    print_timespan_timespec(timing_ss.stream, state.total_tokenization_time);
    newline(timing_ss.stream);

    fputs("Total bytes tokenized: ", timing_ss.stream);
    print_byte_amount(timing_ss.stream, state.total_bytes_tokenized);
    newline(timing_ss.stream);

    fputs("Total tokens produced: ", timing_ss.stream);
    print_amount(timing_ss.stream, state.total_tokens);
    newline(timing_ss.stream);

    {
      fputs("Tokenization time per token produced: ", timing_ss.stream);
      double nanos_per_token =
        timespec_to_nanos(state.total_tokenization_time) / state.total_tokens;
      print_timespan_nanos(timing_ss.stream, nanos_per_token);
      newline(timing_ss.stream);
    }
    {
      fputs("Tokenization time per byte: ", timing_ss.stream);
      double nanos_per_byte = timespec_to_nanos(state.total_tokenization_time) /
                              state.total_bytes_tokenized;
      print_timespan_nanos(timing_ss.stream, nanos_per_byte);
    }
    ss_finalize(&timing_ss);
    VEC_PUSH(&timing_blocks, timing_ss.string);
  }
#endif

#ifdef TIME_PARSER
  if (state.total_parse_nodes_produced > 0) {
    stringstream timing_ss;
    ss_init_immovable(&timing_ss);
    fputs("Time spent parsing: ", timing_ss.stream);
    print_timespan_timespec(timing_ss.stream, state.total_parser_time);
    newline(timing_ss.stream);

    fputs("Total tokens parsed: ", timing_ss.stream);
    print_amount(timing_ss.stream, state.total_tokens_parsed);
    newline(timing_ss.stream);

    fputs("Total parse nodes produced: ", timing_ss.stream);
    print_amount(timing_ss.stream, state.total_parse_nodes_produced);
    newline(timing_ss.stream);

    fprintf(timing_ss.stream,
            "Parse nodes produced per token: %.3f\n",
            (double)state.total_parse_nodes_produced /
              (double)state.total_tokens_parsed);

    {
      fputs("Parse time per token: ", timing_ss.stream);
      double nanos_per_token =
        timespec_to_nanos(state.total_parser_time) / state.total_tokens_parsed;
      print_timespan_nanos(timing_ss.stream, nanos_per_token);
      newline(timing_ss.stream);
    }
    {
      fputs("Parse time per parse node produced: ", timing_ss.stream);
      double nanos_per_token = timespec_to_nanos(state.total_parser_time) /
                               state.total_parse_nodes_produced;
      print_timespan_nanos(timing_ss.stream, nanos_per_token);
    }
    ss_finalize(&timing_ss);
    VEC_PUSH(&timing_blocks, timing_ss.string);
  }
#endif

#ifdef TIME_TYPECHECK
  if (state.total_parse_nodes_typechecked > 0) {
    stringstream timing_ss;
    ss_init_immovable(&timing_ss);
    fputs("Time spent typechecking: ", timing_ss.stream);
    print_timespan_timespec(timing_ss.stream, state.total_typecheck_time);
    newline(timing_ss.stream);

    fputs("Total parse nodes typechecked: ", timing_ss.stream);
    print_amount(timing_ss.stream, state.total_parse_nodes_typechecked);
    newline(timing_ss.stream);

    {
      fputs("Typecheck time per parse node: ", timing_ss.stream);
      double nanos_per_token = timespec_to_nanos(state.total_typecheck_time) /
                               state.total_parse_nodes_typechecked;
      print_timespan_nanos(timing_ss.stream, nanos_per_token);
    }
    ss_finalize(&timing_ss);
    VEC_PUSH(&timing_blocks, timing_ss.string);
  }
#endif

#ifdef TIME_CODEGEN
  if (state.total_parse_nodes_codegened > 0) {
    stringstream timing_ss;
    ss_init_immovable(&timing_ss);
    fputs("Time spent building LLVM IR: ", timing_ss.stream);
    print_timespan_timespec(timing_ss.stream,
                            state.total_llvm_ir_generation_time);

    newline(timing_ss.stream);
    fputs("Time spent performing codegen: ", timing_ss.stream);
    print_timespan_timespec(timing_ss.stream, state.total_codegen_time);
    newline(timing_ss.stream);

    fputs("Total parse nodes turned into IR: ", timing_ss.stream);
    print_amount(timing_ss.stream, state.total_parse_nodes_codegened);
    newline(timing_ss.stream);

    {
      fputs("LLVM IR building time per parse node: ", timing_ss.stream);
      double nanos_per_token =
        timespec_to_nanos(state.total_llvm_ir_generation_time) /
        state.total_parse_nodes_codegened;
      print_timespan_nanos(timing_ss.stream, nanos_per_token);
      newline(timing_ss.stream);
    }
    {
      fputs("Codegen time per parse node: ", timing_ss.stream);
      double nanos_per_token = timespec_to_nanos(state.total_codegen_time) /
                               state.total_parse_nodes_codegened;
      print_timespan_nanos(timing_ss.stream, nanos_per_token);
    }
    ss_finalize(&timing_ss);
    VEC_PUSH(&timing_blocks, timing_ss.string);
  }
#endif

#ifdef TIME_ANY
  for (unsigned i = 0; i < timing_blocks.len; i++) {
    if (i > 0) {
      newline(stdout);
    }
    puts(VEC_GET(timing_blocks, i));
    free(VEC_GET(timing_blocks, i));
  }
  VEC_FREE(&timing_blocks);
#endif

  if (conf.junit) {
    write_test_results(&state);
    VEC_FREE(&state.actions);
    VEC_FREE(&state.strs);
  }

  for (uint32_t i = 0; i < state.failures.len; i++) {
    failure f = VEC_GET(state.failures, i);
    free(f.reason);
    VEC_FREE(&f.path);
  }

  VEC_FREE(&state.failures);
  VEC_FREE(&state.path);
}
