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
  [TC_IS_C_ABI] = SUBS_NONE,
  [TC_VAR] = SUBS_NONE,
  [TC_UNIT] = SUBS_NONE,
  [TC_I8] = SUBS_NONE,
  [TC_U8] = SUBS_NONE,
  [TC_I16] = SUBS_NONE,
  [TC_U16] = SUBS_NONE,
  [TC_I32] = SUBS_NONE,
  [TC_U32] = SUBS_NONE,
  [TC_I64] = SUBS_NONE,
  [TC_U64] = SUBS_NONE,
  [TC_BOOL] = SUBS_NONE,

  [TC_LIST] = SUBS_ONE,

  // TODO shouldn't this be exernal
  [TC_CALL] = SUBS_TWO,
  [TC_TUP] = SUBS_TWO,

  [TC_OR] = SUBS_EXTERNAL,
  [TC_CFN] = SUBS_EXTERNAL,
  [TC_FN] = SUBS_EXTERNAL,

};

const tree_node_repr *const restrict type_reprs = type_repr_arr;

bool inline_types_eq(type a, type b) {
  return a.tag.check == b.tag.check && a.data.type_var == b.data.type_var &&
         a.data.two_subs.a == b.data.two_subs.a &&
         a.data.two_subs.b == b.data.two_subs.b;
}

static void print_type_head(FILE *f, type_check_tag head) {
  static const char *str;
  switch (head) {
    case TC_OR:
      str = "OR";
      break;
    case TC_UNIT:
      str = "()";
      break;
    case TC_I8:
      str = "I8";
      break;
    case TC_U8:
      str = "U8";
      break;
    case TC_I16:
      str = "I16";
      break;
    case TC_U16:
      str = "U16";
      break;
    case TC_I32:
      str = "I32";
      break;
    case TC_U32:
      str = "U32";
      break;
    case TC_I64:
      str = "I64";
      break;
    case TC_U64:
      str = "U64";
      break;
    case TC_VAR:
      str = "TypeVar";
      break;
    case TC_BOOL:
      str = "Bool";
      break;
    case TC_CFN:
      str = "CFn";
      break;
    case TC_FN:
      str = "Fn";
      break;
    case TC_TUP:
      str = "Tuple";
      break;
    case TC_LIST:
      str = "List";
      break;
    case TC_CALL:
      str = "Call";
      break;
    case TC_IS_C_ABI:
      str = "CAbi";
      break;
  }
  fputs(str, f);
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
            assert(type_repr(node.tag.check) == SUBS_ONE);
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
