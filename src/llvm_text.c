// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "parse_tree.h"
#include "llvm_text.h"
#include "typecheck.h"
#include "vec.h"

typedef struct {
  vec_string toplevels;
} llvm_text_state;

static void print_type_llvm_text(FILE *out, type_info types, type_ref root) {}

static void gen_letrec_llvm_text(FILE *out, type_info types, type_ref root) {
  for (node_ind_t di = 0; di < tree.root_subs_amt; di++) {
    node_ind_t node_ind = tree.root_subs_start + di;
    parse_node node = tree.nodes[node_ind];
    switch (node.type.all) {
      case PT_ALL_STATEMENT_FUN:
        fputs("define ", out);
        type_ref type_ind = types.node_types[node_ind];
        type t = types.tree.nodes[type_ind];
        assert(t.tag.normal == T_FN);
        type_ref type_subs_start = t.data.more_subs.start;
        type_ref type_subs_amt = t.data.more_subs.amt;
        type_ref return_type_ind =
          types.tree.inds[type_subs_start + type_subs_amt - 1];
        break;
      default:
        break;
    }
  }
}

typedef struct {
  char *buffer;
  buf_ind_t *starts;
  token_len_t *ends;
} llvm_text_types;

const char *llvm_primitive_typenames[] = {

};

static llvm_text_types gen_types_llvm_text(FILE *out, type_info types) {
  vec_char refs_buf = VEC_NEW;
  VEC_LEN_T ref_ind = 0;

  llvm_text_types res = {
    .buffer = NULL,
    .starts = malloc(sizeof(buf_ind_t) * types.type_amt),
    .ends = malloc(sizeof(token_len_t) * types.type_amt),
  };

  for (type_ref i = 0; i < types.type_amt; i++) {
    type t = types.tree.nodes[i];
    switch (t.tag.normal) {
      case T_I16:
        break;
        // TODO
    }
  }

  for (type_ref i = 0; i < types.type_amt; i++) {
    res.starts[i] = UINT32_MAX;
    res.ends[i] = UINT16_MAX;
  }

  for (type_ref i = 0; i < types.type_amt; i++) {
    vec_type_ref stack = VEC_NEW;
    VEC_PUSH(&stack, i);
    while (stack.len > 0) {
      type_ref tref;
      VEC_POP(&stack, &tref);
      if (res.starts[tref] != UINT32_MAX) {
        // we already have an answer
        continue;
      }
    }
  }
}

void codegen_llvm_module_text(const char *mod_name, parse_tree tree,
                              type_info types, FILE *out) {}
