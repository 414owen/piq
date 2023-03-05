#include "llvm.h"
#include "types.h"
#include "util.h"

// I'm gonna be compiling things. I neeed everything to be set up.
void initialise(void) {
  initialise_util();
  llvm_init();
}
