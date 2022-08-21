#pragma once

#include "typecheck.h"

void gen_and_print_module(source_file source, parse_tree tree, tc_res in,
                          FILE *out);
