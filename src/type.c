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
  };
} print_action;

VEC_DECL(print_action);

static void push_node(vec_print_action *stack, node_ind_t type_ind) {
  print_action act = {
    .tag = PRINT_TYPE,
    .type_ind = type_ind,
  };
  VEC_PUSH(stack, act);
}

static void push_str(vec_print_action *stack, char *str) {
  print_action act = {
    .tag = PRINT_STRING,
    .str = str,
  };
  VEC_PUSH(stack, act);
}

tree_node_repr type_repr(type_check_tag tag) {
  tree_node_repr res = SUBS_EXTERNAL;
  switch (tag) {
    case TC_VAR:
    case TC_UNIT:
    case TC_I8:
    case TC_U8:
    case TC_I16:
    case TC_U16:
    case TC_I32:
    case TC_U32:
    case TC_I64:
    case TC_U64:
    case TC_BOOL:
      res = SUBS_NONE;
      break;
    case TC_LIST:
      res = SUBS_ONE;
      break;
    case TC_CALL:
    case TC_TUP:
      res = SUBS_TWO;
      break;
    case TC_OR:
    case TC_FN:
      res = SUBS_EXTERNAL;
      break;
  }
  return res;
}

bool inline_types_eq(type a, type b) {
  return a.tag == b.tag && a.type_var == b.type_var && a.sub_a == b.sub_a &&
         a.sub_b == b.sub_b;
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
        fputs(action.str, f);
        break;
      case PRINT_TYPE: {
        type node = types[action.type_ind];
        size_t stack_top = stack.len;
        switch (node.all_tag) {
          case TC_VAR: {
            fprintf(f, "(TypeVar %d)", node.type_var);
            break;
          }
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
            push_node(&stack, node.sub_a);
            push_str(&stack, "]");
            break;
          default:
            print_type_head(f, node.all_tag);
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
