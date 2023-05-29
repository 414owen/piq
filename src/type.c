// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>

#include "ast_meta.h"
#include "consts.h"
#include "types.h"
#include "vec.h"

// TODO rename to print_type.c

typedef struct {
  enum {
    PRINT_STRING,
    PRINT_TYPE,
  } tag;

  union {
    node_ind_t type_ind;
    char *str;
  } data;
} print_action;

VEC_DECL(print_action);

static void push_node(vec_print_action *stack, node_ind_t type_ind) {
  print_action act = {
    .tag = PRINT_TYPE,
    .data.type_ind = type_ind,
  };
  VEC_PUSH(stack, act);
}

static void push_str(vec_print_action *stack, char *str) {
  print_action act = {
    .tag = PRINT_STRING,
    .data.str = str,
  };
  VEC_PUSH(stack, act);
}

static const tree_node_repr type_repr_arr[] = {
#define X(_, repr, ...) repr,
  DECL_TYPES
#undef X
};

const tree_node_repr *const restrict type_reprs = type_repr_arr;

bool inline_types_eq(type a, type b) {
  return a.tag.check == b.tag.check && a.data.type_var == b.data.type_var &&
         a.data.two_subs.a == b.data.two_subs.a &&
         a.data.two_subs.b == b.data.two_subs.b;
}

static char *type_head_strs[] = {
#define X(name, repr, llvm_name, str) str,
  DECL_TYPES
#undef X
};

static void print_type_head(FILE *f, type_check_tag head) {
  fputs(type_head_strs[head], f);
}

void print_type_head_placeholders(FILE *f, type_check_tag head) {
  switch (head) {
    case TC_TUP:
      fputs("(a1, a2)", f);
      break;
    default:
      print_type_head(f, head);
      break;
  }
}

void print_type(FILE *f, type *types, node_ind_t *inds, node_ind_t root) {
  vec_print_action stack = VEC_NEW;
  push_node(&stack, root);
  while (stack.len > 0) {
    print_action action;
    VEC_POP(&stack, &action);
    switch (action.tag) {
      case PRINT_STRING:
        fputs(action.data.str, f);
        break;
      case PRINT_TYPE: {
        type node = types[action.data.type_ind];
        size_t stack_top = stack.len;
        switch (node.tag.check) {
          case TC_VAR: {
            fprintf(f, "(TypeVar %d)", node.data.type_var);
            break;
          }
          case TC_OR:
            fputs("(", f);
            push_node(&stack, T_OR_SUB_IND(inds, node, 0));
            for (node_ind_t i = 1; i < T_OR_SUB_AMT(node); i++) {
              push_str(&stack, " or ");
              push_node(&stack, T_OR_SUB_IND(inds, node, i));
            }
            push_str(&stack, ")");
            break;
          case TC_FN:
            fputs("(Fn ", f);
            for (node_ind_t i = 0; i < T_FN_PARAM_AMT(node); i++) {
              push_node(&stack, T_FN_PARAM_IND(inds, node, i));
              push_str(&stack, " -> ");
            }
            push_node(&stack, T_FN_RET_IND(inds, node));
            push_str(&stack, ")");
            break;
          case TC_TUP:
            putc('(', f);
            push_node(&stack, T_TUP_SUB_A(node));
            push_str(&stack, ", ");
            push_node(&stack, T_TUP_SUB_B(node));
            push_str(&stack, ")");
            break;
          case TC_LIST:
            fputs("[", f);
            push_node(&stack, node.data.one_sub.ind);
            push_str(&stack, "]");
            break;
          default:
            assert(type_reprs[node.tag.check] == SUBS_NONE);
            print_type_head(f, node.tag.check);
            break;
        }
        reverse_arbitrary(&VEC_DATA_PTR(&stack)[stack_top],
                          stack.len - stack_top,
                          sizeof(print_action));
      }
    }
  }
  VEC_FREE(&stack);
}

char *print_type_str(type *types, node_ind_t *inds, node_ind_t root) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_type(ss.stream, types, inds, root);
  ss_finalize(&ss);
  return ss.string;
}
