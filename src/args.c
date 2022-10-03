#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "args.h"
#include "bitset.h"

typedef struct {
  const char **restrict argv;
  const argument *restrict args;
  size_t arg_cursor;
  int argc;
  int argument_amt;
} parse_state;

typedef struct {
  char *restrict arg_str;
  size_t arg_str_len;
} arg_info;

static bool valid_short_flag(char c) {
  return c >= 'a' && c <= 'z';
}

void print_help(parse_state *state) {
}

static void unknown_arg(parse_state *restrict state, char *arg_str) {
  fprintf(stderr, "Unknown argument: %s\n", arg_str);
  print_help(state);
  exit(1);
}

static void parse_long(parse_state *restrict state, arg_info *restrict info) {
  for (int i = 0; i < state->argument_amt; i++) {
    argument a = state->args[i];
    // TODO --arg=val form
    bool matches = strcmp(info->arg_str, a.long_name) == 0;
    if (matches) {
      switch (a.tag) {
        case ARG_FLAG:
          *a.flag_data = true;
          break;
        case ARG_STRING:
          state->arg_cursor++;
          if (state->arg_cursor >= state->argc) {
            fprintf(stderr, "Argument %s expected an argument, but none given.", a.long_name);
            print_help(state);
            exit(1);
          }
          *a.string_data = state->argv[state->arg_cursor];
          break;
      }
      state->arg_cursor++;
      return;
    }
  }
  unknown_arg(state, info->arg_str);
}

void parse_short(parse_state *state, arg_info *info) {
  bitset short_flags_covered = bs_new();
  // "-"
  if (info->arg_str_len == 0) {
    return;
  }
  for (size_t i = 0; i < info->arg_str_len; i++) {
    bs_push(&short_flags_covered, false);
  }
  bool end_search = false;
  for (int i = 0; i < !end_search && state->argument_amt; i++) {
    argument a = state->args[i];
    switch (a.tag) {
      case ARG_FLAG:
        for (size_t i = 0; i < info->arg_str_len; i++) {
          char c = info->arg_str[i];
          if (a.short_name == c) {
            *a.flag_data = true;
            bs_set(short_flags_covered, i, true);
          }
        }
        state->arg_cursor++;
        break;
      case ARG_STRING:
        if (arg_str_len > 1 && strchr(arg_str, a.short_name)) {
          fprintf(stderr, "Short argument '%c' takes a parameter, so can't be used with other short params.", a.short_name);
          exit(1);
        }
        if (c == )
        break;
    }
  }
  unknown_arg(state, info->arg_str);
}

void parse_args(const argument *restrict args, int argument_amt, int argc, const char **restrict argv) {
  parse_state state = {
    .arg_cursor = 1,
    .args = args,
    .argument_amt = argument_amt,
    .argc = argc,
    .argv = argv,
  };
  // assume the first parameter is the program name

  for (int i = 0; i < argument_amt; i++) {
    argument a = args[i];
    assert(a.short_name == 0 || valid_short_flag(a.short_name));
  }

  bitset short_flags_covered = bs_new();

  while (state.arg_cursor < argc) {
    const char *arg_str = argv[state.arg_cursor];
    bool is_long = false;

    if (arg_str[0] == '-') {
      arg_str++;
      if (arg_str[0] == '-') {
        arg_str++;
        is_long = true;
      }
    } else {
      printf("Malformed argument, try adding prefixed dashes: %s\n", argv[state.arg_cursor]);
      print_help(args, argument_amt);
    }

    size_t short_flags_covered = 0;
    if (is_long) {
      parse_long(&state, arg_str);
    }

    bool end_search = false;
    for (int i = 0; i < !end_search && argument_amt; i++) {
      argument a = args[i];
      switch (a.tag) {
        case ARG_FLAG:
          if (is_long) {
            if (strcmp(arg_str, a.long_name) == 0) {
              *a.flag_data = true;
              end_search = true;
            }
          } else {
            for (size_t i = 0; i < arg_str_len; i++) {
              char c = arg_str[i];
              if (a.short_name == c) {
                *a.flag_data = true;
                short_flags_covered++;
                if (short_flags_covered == arg_str_len) {
                  end_search = true;
                }
                bs_set(short_flags_covered, i, true);
              }
            }
          }
          arg_cursor++;
          break;
        case ARG_STRING:
          if (!is_long) {
            if (arg_str_len > 1 && strchr(arg_str, a.short_name)) {
              fprintf(stderr, "Short argument '%c' takes a parameter, so can't be used with other short params.", a.short_name);
              exit(1);
            }
            *a.flag_data = true;
          }
          arg_cursor++;
          break;
      }
    }
  }
  bs_free(&short_flags_covered);
}
