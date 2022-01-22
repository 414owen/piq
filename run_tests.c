#include "test.h"

int main(int argc, char **argv) {
  test_state state = test_state_new();
  test_scanner(&state);
  print_failures(&state);
}
