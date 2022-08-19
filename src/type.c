#include <stdio.h>

#include "ast_meta.h"
#include "consts.h"
#include "types.h"
#include "vec.h"

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

tree_node_repr type_repr(type_tag tag) {
  tree_node_repr res = SUBS_EXTERNAL;
  switch (tag) {
    case T_UNKNOWN:
    case T_UNIT:
    case T_I8:
    case T_U8:
    case T_I16:
    case T_U16:
    case T_I32:
    case T_U32:
    case T_I64:
    case T_U64:
    case T_BOOL:
      res = SUBS_NONE;
      break;
    case T_LIST:
      res = SUBS_ONE;
      break;
    case T_CALL:
    case T_FN:
    case T_TUP:
      res = SUBS_TWO;
      break;
      /*
        res = SUBS_EXTERNAL;
        break;
      */
  }
  return res;
}

bool inline_types_eq(type a, type b) {
  bool res = true;
  res &= a.tag == b.tag;
  res &= a.sub_a == b.sub_a;
  res &= a.sub_b == b.sub_b;
  return res;
}

static void print_type_head(FILE *f, type_tag head) {
  static const char *str;
  switch (head) {
    case T_UNIT:
      str = "()";
      break;
    case T_I8:
      str = "I8";
      break;
    case T_U8:
      str = "U8";
      break;
    case T_I16:
      str = "I16";
      break;
    case T_U16:
      str = "U16";
      break;
    case T_I32:
      str = "I32";
      break;
    case T_U32:
      str = "U32";
      break;
    case T_I64:
      str = "I64";
      break;
    case T_U64:
      str = "U64";
      break;
    case T_UNKNOWN:
      str = "<unknown>";
      break;
    case T_BOOL:
      str = "Bool";
      break;
    case T_FN:
      str = "Fn";
      break;
    case T_TUP:
      str = "Tuple";
      break;
    case T_LIST:
      str = "List";
      break;
    case T_CALL:
      str = "Call";
      break;
  }
  fputs(str, f);
}

void print_type_head_placeholders(FILE *f, type_tag head) {
  switch (head) {
    case T_TUP:
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
    print_action action = VEC_POP(&stack);
    switch (action.tag) {
      case PRINT_STRING:
        fputs(action.str, f);
        break;
      case PRINT_TYPE: {
        type node = types[action.type_ind];
        size_t stack_top = stack.len;
        switch (node.tag) {
          case T_FN:
            fputs("(Fn ", f);
            push_node(&stack, node.sub_a);
            push_str(&stack, " -> ");
            push_node(&stack, node.sub_b);
            push_str(&stack, ")");
            break;
          case T_TUP:
            putc('(', f);
            push_node(&stack, T_TUP_SUB_A(node));
            push_str(&stack, ", ");
            push_node(&stack, T_TUP_SUB_B(node));
            push_str(&stack, ")");
            break;
          case T_LIST:
            fputs("[", f);
            push_node(&stack, node.sub_a);
            push_str(&stack, "]");
            break;
          default:
            print_type_head(f, node.tag);
            break;
        }
        reverse_arbitrary(&VEC_DATA_PTR(&stack)[stack_top],
                          stack.len - stack_top, sizeof(print_action));
      }
    }
  }
  VEC_FREE(&stack);
}
