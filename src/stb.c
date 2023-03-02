#include <time.h>

#define STB_DS_IMPLEMENTATION
#include "stb/stb_ds.h"

void initialise_stb(void) {
  stbds_rand_seed(time(NULL));  
}
