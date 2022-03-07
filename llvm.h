#pragma once

#include "llvm-c/Core.h"

#include "typecheck.h"

void gen_and_print_module(tc_res in, FILE *out);
