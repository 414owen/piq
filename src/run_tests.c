#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "args.h"

static void run_tests(test_config conf) {

  test_state state = test_state_new(conf);

  test_vec(&state);
  test_bitset(&state);
  test_utils(&state);
  test_scanner(&state);
  test_parser(&state);
  test_typecheck(&state);
  test_llvm(&state);

  test_state_finalize(&state);

  print_failures(&state);
  printf("Tests passed: %" PRIu32 "/%" PRIu32 "\n", state.tests_passed,
         state.tests_run);

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

int main(int argc, const char **argv) {
  test_config conf = {
    .junit = false,
    .lite = false,
    .filter_str = NULL,
  };

  int times = 1;

  argument args[] = {{
    .long_name = "lite",
    .short_name = 'l',
    .flag_data = &conf.lite,
  }, {
    .long_name = "junit",
    .short_name = 'j',
    .flag_data = &conf.junit,
  }, {
    .long_name = "times",
    .short_name = 't',
    .int_data = &times,
  }};

  parse_args(args, STATIC_LEN(args), argc, argv);

  for (int i = 0; i < times; i++) {
    run_tests(conf);
  }
}
