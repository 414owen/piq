#pragma once

#include "stdlib.h"
#include "vec.h"
#include "binding.h"
#include "bitset.h"

// TODO this shouldn't exist, let's just move it into parse_tree.c
typedef struct {
  bitset is_builtin;
  vec_str_ref bindings;
} scope;

node_ind_t lookup_str_ref(const char *source_file, scope scope, binding bnd);

scope scope_new(void);

void scope_push(scope *s, binding b);
void scope_free(scope s);
