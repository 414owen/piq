#pragma once

#include "stdlib.h"
#include "vec.h"
#include "binding.h"
#include "bitset.h"

size_t lookup_bnd(const char *source_file, vec_str_ref bnds,
                  bitset is_builtin, binding bnd);