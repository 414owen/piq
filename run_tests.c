#include "test.h"

int main(int argc, char **argv) {
  test_state state = test_state_new();
  test_scanner(&state);
  test_parser(&state);
  print_failures(&state);
  for (int i = 0; i < state.failures.len; i++) {
    failure f = state.failures.data[i];
    free(f.reason);
    VEC_FREE(&f.path);
  }
  VEC_FREE(&state.failures);
  VEC_FREE(&state.path);
  if (state.failures.len > 0)
    exit(1);
}
