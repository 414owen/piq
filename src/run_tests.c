#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

static void run_tests(test_config conf) {

  test_state state = test_state_new(conf);

  // TODO just add the config to state -_-

  test_vec(&state);
  test_bitset(&state);
  test_utils(&state);
  test_scanner(&state);
  test_parser(&state);
  test_typecheck(&state);

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
  // if (state.failures.len > 0)
  //   exit(1);
}

int main(int argc, char **argv) {
  test_config conf = {
    .junit = false,
    .lite = false,
  };

  int times = 1;

  for (int i = 0; i < argc; i++) {
    char *arg = argv[i];
    if (strcmp(arg, "--lite") == 0) {
      puts("Test lite mode");
      conf.lite = true;
    }
    if (strcmp(arg, "--junit") == 0) {
      conf.junit = true;
    }
    static const char *times_str = "--times=";
    if (prefix(times_str, arg)) {
      times = atoi(arg + strlen(times_str));
    }
  }

  for (int i = 0; i < times; i++) {
    run_tests(conf);
  }
}
