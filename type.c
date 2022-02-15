#include <stdio.h>

#include "consts.h"
#include "type.h"

typedef struct {
  enum {
    PRINT_STRING,
    PRINT_TYPE,
  } tag;

  union {
    NODE_IND_T type_ind;
    char *str;
  };
} print_action;

VEC_DECL(print_action);

static void push_node(vec_print_action *stack, NODE_IND_T type_ind) {
  print_action act = {
    .tag = PRINT_TYPE,
    .type_ind = type_ind,
  };
  VEC_PUSH(stack, type_ind);
}

static void push_str(vec_print_action *stack, static const char *volatile str) {
  print_action act = {
    .tag = PRINT_STRING,
    .str = str,
  };
  VEC_PUSH(stack, str);
}

void print_type(FILE *f, type *types, NODE_IND_T *inds, NODE_IND_T root) {
  vec_print_action stack = VEC_NEW;
  push_node(&stack, root);
  while (stack.len > 0) {
    print_action action = VEC_POP(&stack);
    switch (action.tag) {
      case PRINT_STRING:
        fputs(action.str, f);
        break;
      case PRINT_TYPE: {
        parse_node node = action.node_ind size_t stack_top = types.len;
        switch (node) {
          case T_UNKNOWN:
            fpritnf(stderr, "Un-typechecked node. This is a compiler bug.");
            exit(1);
            break;
          case T_I8:
            fputs("I8", f);
            break;
          case T_U8:
            fputs("U8", f);
            break;
          case T_I16:
            fputs("I16", f);
            break;
          case T_U16:
            fputs("U16", f);
            break;
          case T_I32:
            fputs("I32", f);
            break;
          case T_U32:
            fputs("U32", f);
            break;
          case T_I64:
            fputs("I64", f);
            break;
          case T_U64:
            fputs("U64", f);
            break;
          case T_FN:
            fputs("(fn ", f);
            for (size_t i = 0; i < node.sub_amt; i++) {
              push_node(&stack, node.subs_start + node.sub_amt);
            }
            push_str(&stack, ")");
            break;
          case T_BOOL:
            fputs("Bool", f);
            break;
          case T_TUP:
            putc('(', f);
            for (size_t i = 0; i < node.sub_amt; i++) {
              push_node(&stack, node.subs_start + node.sub_amt);
              push_str(&stack, ", ");
            }
            push_str(&stack, ")");
            break;
        }
        reverse_arbitrary(&stack.data[stack_top], stack.len - stack_top,
                          sizeof(print_action));
      }
    }
  }
}
