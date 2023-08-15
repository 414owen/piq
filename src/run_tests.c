// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "args.h"
#include "defs.h"
#include "benchmark.h"
#include "global_settings.h"
#include "initialise.h"
#include "llvm.h"
#include "perf.h"
#include "test.h"
#include "tests.h"
#include "timespec.h"

static void run_tests(test_state *state) {
  test_vec(state);
  test_bitset(state);
  test_hashmap(state);
  test_strint(state);
  test_utils(state);
  test_diagnostics(state);
  test_scanner(state);
  test_parser(state);
  test_traverse(state);
  test_typecheck(state);
  test_semantics(state);
  // CI is currently throwing std::bad_allow when
  // jitting...
#ifndef BUILD_FOR_CI
  test_llvm(state);
#endif
}

enum {
  SUB_NONE = -1,
  SUB_BENCH = 0,
} subcommand;

#ifdef TIME_ANY

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

  fprintf(f, "%.02lf%s", dblBytes, byte_suffixes[i]);
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

static void print_timespan_nanos(FILE *f, uint64_t ns) {
  if (ns < 1000) {
    fprintf(f, "%" PRIu64 "ns", ns);
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

static void print_timespan_timespec(FILE *f, timespec ts) {
  print_timespan_nanos(f, timespec_to_nanoseconds(ts));
}

static void print_timespan_float_nanos(FILE *f, double nanos) {
  if (nanos < 1000) {
    fprintf(f, "%.3fns", nanos);
  } else {
    print_timespan_nanos(f, (uint64_t)nanos);
  }
}

HEDLEY_NEVER_INLINE
static void newline(FILE *f) { putc('\n', f); }

typedef enum {
  METRIC_H_TOKENIZER,
  METRIC_H_PARSER,
  METRIC_H_RESOLVE_NAMES,
  METRIC_H_TYPECHECK,
  METRIC_H_CODEGEN,
} metric_heading;

typedef struct {
  struct {
    bool print_json;
    FILE *json_file;
  } params;
  bool start;
  metric_heading heading;
  metric_heading last_heading;
} put_metric_state;

typedef struct {
  char *name;
  uint64_t amount;
} amount_metric;

typedef struct {
  char *name;
  timespec time;
} time_metric;

typedef struct {
  char *name;
  double nanoseconds;
} float_time_metric;

typedef struct {
  char *name;
  double amount;
} float_metric;

static void put_metric_preamble(put_metric_state *state, const char *name) {
  if (!state->start && state->heading != state->last_heading) {
    newline(stdout);
  }
  fputs(name, stdout);
  fputs(": ", stdout);
  if (state->params.print_json) {
    if (!state->start) {
      fputc(',', state->params.json_file);
    }
    fprintf(state->params.json_file,
            "\n"
            " {\n"
            "  \"name\": \"%s\",",
            name);
  }
  state->start = false;
  state->last_heading = state->heading;
}

static void print_metric_postamble(put_metric_state *state) {
  if (state->params.print_json) {
    fputs("\n }", state->params.json_file);
  }
}
static void put_metric_amount(put_metric_state *state, amount_metric m) {
  put_metric_preamble(state, m.name);
  print_amount(stdout, m.amount);
  newline(stdout);
  if (state->params.print_json) {
    fprintf(state->params.json_file,
            "  \"unit\": \"amount\",\n"
            "  \"value\": %" PRIu64 "",
            m.amount);
  }
  print_metric_postamble(state);
}

static void put_metric_byte_amount(put_metric_state *state, amount_metric m) {
  put_metric_preamble(state, m.name);
  print_byte_amount(stdout, m.amount);
  newline(stdout);
  if (state->params.print_json) {
    fprintf(state->params.json_file,
            "  \"unit\": \"bytes\",\n"
            "  \"value\": %" PRIu64,
            m.amount);
  }
  print_metric_postamble(state);
}

static void put_metric_time(put_metric_state *state, time_metric m) {
  put_metric_preamble(state, m.name);
  print_timespan_timespec(stdout, m.time);
  newline(stdout);
  if (state->params.print_json) {
    fprintf(state->params.json_file,
            "  \"unit\": \"ns\",\n"
            "  \"value\": %" PRIu64,
            timespec_to_nanoseconds(m.time));
  }
  print_metric_postamble(state);
}

static void put_metric_time_float(put_metric_state *state,
                                  float_time_metric m) {
  put_metric_preamble(state, m.name);
  print_timespan_float_nanos(stdout, m.nanoseconds);
  newline(stdout);
  if (state->params.print_json) {
    fprintf(state->params.json_file,
            "  \"unit\": \"ns\",\n"
            "  \"value\": %.3f",
            m.nanoseconds);
  }
  print_metric_postamble(state);
}

static void put_metric_float(put_metric_state *state, float_metric m) {
  put_metric_preamble(state, m.name);
  printf("%.3f\n", m.amount);
  if (state->params.print_json) {
    fprintf(state->params.json_file,
            "  \"unit\": \"amount\",\n"
            "  \"value\": %.3f",
            m.amount);
  }
  print_metric_postamble(state);
}

static void put_perf_timings(put_metric_state *state, const char *operation,
                             perf_values values) {
  {
    char *desc = format_to_string("Time spent in %s", operation);
    time_metric m = {
      .name = desc,
      .time = values.time_taken,
    };
    put_metric_time(state, m);
    free(desc);
  }
#ifdef PREDEF_OS_LINUX
  {
    char *desc = format_to_string("Cycles spent in %s", operation);
    amount_metric m = {
      .name = desc,
      .amount = values.hw_cpu_cycles,
    };
    put_metric_amount(state, m);
    free(desc);
  }
  {
    char *desc = format_to_string("Instructions retired in %s", operation);
    amount_metric m = {
      .name = desc,
      .amount = values.hw_retired_instructions,
    };
    put_metric_amount(state, m);
    free(desc);
  }
  {
    char *desc = format_to_string("Branches retired in %s", operation);
    amount_metric m = {
      .name = desc,
      .amount = values.hw_retired_branches,
    };
    put_metric_amount(state, m);
    free(desc);
  }
  {
    char *desc = format_to_string("Branch mispredictions in %s", operation);
    amount_metric m = {
      .name = desc,
      .amount = values.hw_branch_mispredictions,
    };
    put_metric_amount(state, m);
    free(desc);
  }
  {
    char *desc = format_to_string("Cache misses in %s", operation);
    amount_metric m = {
      .name = desc,
      .amount = values.hw_cache_misses,
    };
    put_metric_amount(state, m);
    free(desc);
  }
#endif
}

static void put_perf_per_thing(put_metric_state *state, const char *operation,
                               const char *object, uint64_t number,
                               perf_values values) {

  {
    double nanos_per_parse_node =
      (double)timespec_to_nanoseconds(values.time_taken) / (double)number;
    char *desc = format_to_string("%s time per %s", operation, object);
    float_time_metric m = {
      .name = desc,
      .nanoseconds = nanos_per_parse_node,
    };
    put_metric_time_float(state, m);
    free(desc);
  }
#ifdef PREDEF_OS_LINUX
  {
    double cycles_per_parse_node =
      (double)values.hw_cpu_cycles / (double)number;
    char *desc = format_to_string("%s cycles per %s", operation, object);
    float_metric m = {
      .name = desc,
      .amount = cycles_per_parse_node,
    };
    put_metric_float(state, m);
    free(desc);
  }
  {
    double instructions_retired_per_parse_node =
      (double)values.hw_retired_instructions / (double)number;
    char *desc =
      format_to_string("%s instructions retired per %s", operation, object);
    float_metric m = {
      .name = desc,
      .amount = instructions_retired_per_parse_node,
    };
    put_metric_float(state, m);
    free(desc);
  }
  {
    double branches_retired_per_parse_node =
      (double)values.hw_retired_branches / (double)number;
    char *desc =
      format_to_string("%s branches retired per %s", operation, object);
    float_metric m = {
      .name = desc,
      .amount = branches_retired_per_parse_node,
    };
    put_metric_float(state, m);
    free(desc);
  }
  {
    double cache_misses_per_parse_node =
      (double)values.hw_cache_misses / (double)number;
    char *desc = format_to_string("%s cache misses per %s", operation, object);
    float_metric m = {
      .name = desc,
      .amount = cache_misses_per_parse_node,
    };
    put_metric_float(state, m);
    free(desc);
  }
  {
    double branch_mispredictions_per_parse_node =
      (double)values.hw_branch_mispredictions / (double)number;
    char *desc = format_to_string("%s branch misses per %s", operation, object);
    float_metric m = {
      .name = desc,
      .amount = branch_mispredictions_per_parse_node,
    };
    put_metric_float(state, m);
    free(desc);
  }
#endif
}
#endif

typedef struct {
  bool verbose;
  bool extra_verbose;
} global_arguments;

int main(int argc, char **argv) {
  initialise();

  global_arguments global_args = {
    .verbose = false,
    .extra_verbose = false,
  };

  test_config conf = {
    .junit = false,
    .lite = false,
    .filter_str = NULL,
    .times = 1,
    .write_json = false,
  };

  argument bench_args[] = {
    {
      .tag = ARG_FLAG,
      .names.long_name = "write-json",
      .short_name = 'j',
      .data.flag_val = &conf.write_json,
      .description = "Write a json report (for continuous benchmarking CI)",
    },
  };

  argument_bag bench_arg_bag = {
    .amt = STATIC_LEN(bench_args),
    .args = bench_args,
    .subcommand_chosen = SUB_NONE,
  };

  argument args[] = {
    [SUB_BENCH] =
      {
        .tag = ARG_SUBCOMMAND,
        .names.subcommand_name = "bench",
        .data.subcommand =
          {
            .subs = bench_arg_bag,
          },
        .short_name = 0,
        .description = "Run benchmark suite",
      },
    {
      .tag = ARG_FLAG,
      .short_name = 'v',
      .names.long_name = "verbose",
      .data.flag_val = &global_args.verbose,
      .description = "print extra information for debugging purposes",
    },
    {
      .tag = ARG_FLAG,
      .names.long_name = "extra-verbose",
      .data.flag_val = &global_args.extra_verbose,
      .description = "print a lot of extra information for debugging purposes",
    },
    {
      .tag = ARG_FLAG,
      .names.long_name = "lite",
      .short_name = 'l',
      .data.flag_val = &conf.lite,
      .description = "Turn off stress tests",
    },
    {.tag = ARG_FLAG,
     .names.long_name = "junit",
     .short_name = 'j',
     .data.flag_val = &conf.junit,
     .description = "Create JUnit compatible test-results.xml file"},
    {
      .tag = ARG_INT,
      .names.long_name = "times",
      .short_name = 't',
      .data.int_val = &conf.times,
      .description = "Run the test suite more than once (for benchmarking)",
    },
    {
      .tag = ARG_STRING,
      .names.long_name = "match",
      .short_name = 'm',
      .data.string_val = &conf.filter_str,
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

  if (global_args.verbose) {
    global_settings.verbosity = VERBOSE_SOME;
  }
  if (global_args.extra_verbose) {
    global_settings.verbosity = VERBOSE_VERY;
  }

  test_state state = test_state_new(conf);

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
  puts("\n--- Metrics:\n");

  put_metric_state metric_state = {
    .params =
      {
        .print_json = conf.write_json,
        .json_file = conf.write_json ? fopen("bench.json", "w") : NULL,
      },
    .start = true,
  };

  if (metric_state.params.print_json) {
    fputc('[', metric_state.params.json_file);
  }
#endif

#ifdef TIME_TOKENIZER
  if (state.total_bytes_tokenized > 0) {
    metric_state.heading = METRIC_H_TOKENIZER;

    put_perf_timings(&metric_state, "tokenizer", state.total_tokenization_perf);

    {
      amount_metric m = {
        .name = "Total bytes tokenized",
        .amount = state.total_bytes_tokenized,
      };
      put_metric_byte_amount(&metric_state, m);
    }

    {
      amount_metric m = {
        .name = "Total tokens produced",
        .amount = state.total_tokens_produced,
      };
      put_metric_amount(&metric_state, m);
    }

    {
      const char *name = "Tokenizer";
      put_perf_per_thing(&metric_state,
                         name,
                         "byte",
                         state.total_bytes_tokenized,
                         state.total_tokenization_perf);
      put_perf_per_thing(&metric_state,
                         name,
                         "token produced",
                         state.total_tokens_produced,
                         state.total_tokenization_perf);
    }
  }
#endif

#ifdef TIME_PARSER
  if (state.total_parse_nodes_produced > 0) {
    metric_state.heading = METRIC_H_PARSER;

    put_perf_timings(&metric_state, "parser", state.total_parser_perf);

    {
      amount_metric m = {
        .name = "Total tokens parsed",
        .amount = state.total_tokens_parsed,
      };
      put_metric_amount(&metric_state, m);
    }

    {
      amount_metric m = {
        .name = "Total parse nodes produced",
        .amount = state.total_parse_nodes_produced,
      };
      put_metric_amount(&metric_state, m);
    }

    {
      double ratio = (double)state.total_parse_nodes_produced /
                     (double)state.total_tokens_parsed;
      float_metric m = {
        .name = "Parse nodes produced per token",
        .amount = ratio,
      };
      put_metric_float(&metric_state, m);
    }

    {
      const char *name = "Parser";
      put_perf_per_thing(&metric_state,
                         name,
                         "byte",
                         state.total_tokens_parsed,
                         state.total_parser_perf);
      put_perf_per_thing(&metric_state,
                         name,
                         "token produced",
                         state.total_parse_nodes_produced,
                         state.total_parser_perf);
    }
  }
#endif

#ifdef TIME_NAME_RESOLUTION
  if (state.total_names_looked_up > 0) {
    metric_state.heading = METRIC_H_RESOLVE_NAMES;

    put_perf_timings(
      &metric_state, "name resolver", state.total_name_resolution_perf);

    {
      amount_metric m = {
        .name = "Total names looked up",
        .amount = state.total_names_looked_up,
      };
      put_metric_amount(&metric_state, m);
    }

    {
      const char *name = "Name resolver";
      put_perf_per_thing(&metric_state,
                         name,
                         "name looked up",
                         state.total_names_looked_up,
                         state.total_name_resolution_perf);
      // TODO add
      // put_perf_per_thing(&metric_state, name, "parse node",
      // state.total_parse_nodes_resolved, state.total_name_resolution_time);
    }
  }
#endif

#ifdef TIME_TYPECHECK
  if (state.total_parse_nodes_typechecked > 0) {
    metric_state.heading = METRIC_H_TYPECHECK;

    put_perf_timings(&metric_state, "typechecker", state.total_typecheck_perf);

    {
      amount_metric m = {
        .name = "Total parse nodes typechecked",
        .amount = state.total_parse_nodes_typechecked,
      };
      put_metric_amount(&metric_state, m);
    }

    {
      const char *name = "Typecheck";
      put_perf_per_thing(&metric_state,
                         name,
                         "parse node",
                         state.total_parse_nodes_typechecked,
                         state.total_typecheck_perf);
    }
  }
#endif

#ifdef TIME_CODEGEN
  if (state.total_parse_nodes_codegened > 0) {
    metric_state.heading = METRIC_H_CODEGEN;

    put_perf_timings(
      &metric_state, "llvm ir generation", state.total_llvm_ir_generation_perf);
    put_perf_timings(&metric_state, "llvm codegen", state.total_codegen_perf);

    {
      amount_metric m = {
        .name = "Total parse nodes turned into LLVM IR",
        .amount = state.total_parse_nodes_codegened,
      };
      put_metric_amount(&metric_state, m);
    }

    {
      put_perf_per_thing(&metric_state,
                         "LLVM IR Build",
                         "parse node",
                         state.total_parse_nodes_codegened,
                         state.total_llvm_ir_generation_perf);
      put_perf_per_thing(&metric_state,
                         "Codegen",
                         "parse node",
                         state.total_parse_nodes_codegened,
                         state.total_codegen_perf);
    }
  }
#endif

#ifdef TIME_ANY
  if (metric_state.params.print_json) {
    fputs("\n]\n", metric_state.params.json_file);
  }
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
