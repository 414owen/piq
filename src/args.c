#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <hedley.h>

#include "args.h"
#include "bitset.h"
#include "util.h"
#include "vec.h"

typedef struct {
  const char **restrict argv;
  const argument *restrict args;
  int arg_cursor;
  int argc;
  int argument_amt;
} parse_state;

static bool valid_short_flag(char c) {
  return c >= 'a' && c <= 'z';
}

void print_help(parse_state *state) {
  int max_len = 0;
  for (int i = 0; i < state->argument_amt; i++) {
    argument a = state->args[i];
    max_len = MAX(max_len, a.long_len);
  }
  for (int i = 0; i < state->argument_amt; i++) {
    argument a = state->args[i];
    printf("  -%c      %*s--%s", a.short_name, max_len - a.long_len, "", a.long_name);
    switch (a.tag) {
      case ARG_FLAG:
        putc('\n', stdout);
        break;
      case ARG_STRING:
        puts("   STRING");
        break;
      case ARG_INT:
        puts("   INT");
        break;
    }
  }
}

HEDLEY_NO_RETURN
static void unknown_arg(parse_state *restrict state, const char *restrict arg_str) {
  fprintf(stderr, "Unknown argument: %s\n", arg_str);
  print_help(state);
  exit(1);
}

HEDLEY_NO_RETURN
static void expected_argument(parse_state *restrict state, const char *restrict arg_name) {
  fprintf(stderr, "Argument %s expected a value, but none given", arg_name);
  print_help(state);
  exit(1);
}

static void parse_long(parse_state *restrict state, const char *restrict arg_str, int arg_str_len) {
  if (strcmp(arg_str, "help") == 0) {
    print_help(state);
    exit(0);
  }
  for (int i = 0; i < state->argument_amt; i++) {
    argument a = state->args[i];
    // TODO --arg=val form
    bool prefix_matches = arg_str_len >= a.long_len && strncmp(arg_str, a.long_name, a.long_len) == 0;
    bool matches = prefix_matches && arg_str_len == a.long_len;
    bool matched = false;
    switch (a.tag) {
      case ARG_FLAG:
        if (matches) {
          *a.flag_data = true;
          matched = true;
        }
        break;
      case ARG_INT:
      case ARG_STRING:
        if (matches) {
          if (state->arg_cursor >= state->argc) {
            expected_argument(state, a.long_name);
          }
          state->arg_cursor++;
          *a.string_data = state->argv[state->arg_cursor];
          matched = true;
        } else if (prefix_matches && arg_str[a.long_len] == '=') {
          *a.string_data = state->argv[state->arg_cursor];
          matched = true;
        }
        if (a.tag == ARG_INT) {
          const char *str = *a.string_data;
          *a.int_data = atoi(str);;
        }
        break;
    }
    if (matched) {
      state->arg_cursor++;
      return;
    }
  }
  unknown_arg(state, arg_str);
}

void parse_short(parse_state *state, const char *restrict arg_str, size_t arg_str_len) {
  if (strchr(arg_str, 'h')) {
    print_help(state);
    exit(0);
  }

  // just ignore "-"
  if (arg_str_len == 0) {
    return;
  }
  vec_char unmatched = VEC_NEW;
  for (size_t i = 0; i < arg_str_len; i++) {
    char c = arg_str[i];
    bool matched = false;
    for (int i = 0; i < !matched && state->argument_amt; i++) {
      argument a = state->args[i];
      matched |= c == a.short_name;
      switch (a.tag) {
        case ARG_FLAG:
          if (matched) {
            *a.flag_data = true;
          }
          break;
        case ARG_INT:
        case ARG_STRING:
          if (matched) {
            if (arg_str_len > 1) {
              fprintf(stderr, "Short argument '%c' takes a parameter, so can't be used with other short params.", a.short_name);
              exit(1);
            }
            if (state->arg_cursor >= state->argc) {
              char s[2] = { a.short_name, '0' };
              expected_argument(state, s);
            }
            state->arg_cursor++;
            *a.string_data = state->argv[state->arg_cursor];
          }
          if (a.tag == ARG_INT) {
            const char *str = *a.string_data;
            *a.int_data = atoi(str);;
          }
          break;
      }
    }
    if (!matched) {
      VEC_PUSH(&unmatched, c);
    }
  }
  if (unmatched.len > 0) {
    fputs("Unknown short arguments: ", stderr);
    fprintf(stderr, "%c", VEC_GET(unmatched, 0));
    for (size_t i = 1; i < unmatched.len; i++) {
      fprintf(stderr, ", %c", VEC_GET(unmatched, i));
    }
    exit(1);
  }
  state->arg_cursor++;
}

void parse_args(argument *restrict args, int argument_amt, int argc, const char **restrict argv) {

  // preprocess
  for (int i = 0; i < argument_amt; i++) {
    args[i].long_len = strlen(args[i].long_name);
  }

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
      print_help(&state);
    }

    if (is_long) {
      parse_long(&state, arg_str, strlen(arg_str));
    } else {
      parse_short(&state, arg_str, strlen(arg_str));
    }
  }
}
