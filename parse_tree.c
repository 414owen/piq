#include <stdio.h>

#include "parse_tree.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_CLOSE_PAREN,
  PRINT_SPACE,
  PRINT_COMMA_SPACE,
  PRINT_COLON_SPACE,
  PRINT_NL,
} print_action;

VEC_DECL(print_action);

typedef struct {
  vec_print_action actions;
  vec_node_ind node_stack;
  parse_tree tree;
  source_file file;
} printer_state;

static void print_compound(FILE *f, printer_state *s, char *prefix,
                           parse_node node) {
  fputs(prefix, f);
  for (uint16_t i = 0; i < node.sub_amt; i++) {
    if (i > 0)
      VEC_PUSH(&s->actions, PRINT_SPACE);
    VEC_PUSH(&s->actions, PRINT_NODE);
    VEC_PUSH(&s->node_stack, s->tree.inds.data[node.subs_start + i]);
  }
  VEC_PUSH(&s->actions, PRINT_CLOSE_PAREN);
}

static void print_node(FILE *f, printer_state *s, NODE_IND_T node_ind) {
  parse_node node = s->tree.nodes.data[node_ind];
  switch (node.type) {
    case PT_ROOT:
      for (size_t i = 0; i < node.sub_amt; i++) {
        if (i > 0)
          VEC_PUSH(&s->actions, PRINT_NL);
        VEC_PUSH(&s->actions, PRINT_NODE);
        VEC_PUSH(&s->node_stack, s->tree.inds.data[node.subs_start + i]);
      }
      break;
    case PT_FN:
      fputs("(Fn (", f);
      for (size_t i = 0; i < (node.sub_amt - 1) / 2; i++) {
        if (i > 0)
          VEC_PUSH(&s->actions, PRINT_COMMA_SPACE);
        NODE_IND_T param = s->tree.inds.data[node.subs_start + i * 2];
        NODE_IND_T type = s->tree.inds.data[node.subs_start + i * 2 + 1];
        static const print_action param_actions[] = {
          PRINT_NODE, PRINT_COLON_SPACE, PRINT_NODE};
        VEC_APPEND(&s->actions, STATIC_LEN(param_actions), param_actions);
        VEC_PUSH(&s->node_stack, param);
        VEC_PUSH(&s->node_stack, type);
      }
      VEC_PUSH(&s->node_stack,
               s->tree.inds.data[node.subs_start + node.sub_amt - 1]);
      static const print_action actions[] = {PRINT_CLOSE_PAREN, PRINT_SPACE,
                                             PRINT_NODE};
      VEC_APPEND(&s->actions, STATIC_LEN(actions), actions);
      VEC_PUSH(&s->actions, PRINT_CLOSE_PAREN);
      break;
    case PT_CALL:
      print_compound(f, s, "(Call ", node);
      break;
    case PT_INT:
      fprintf(f, "(Int %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_UPPER_NAME:
      fprintf(f, "(Uname %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_LOWER_NAME:
      fprintf(f, "(Lname %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_IF:
      print_compound(f, s, "(If ", node);
      break;
    case PT_TUP:
      print_compound(f, s, "(Tup ", node);
      break;
  }
}

void print_parse_tree(FILE *f, source_file file, parse_tree tree) {
  printer_state s = {
    .actions = VEC_NEW,
    .node_stack = VEC_NEW,
    .tree = tree,
    .file = file,
  };
  VEC_PUSH(&s.actions, PRINT_NODE);
  VEC_PUSH(&s.node_stack, tree.root_ind);

  while (s.actions.len > 0) {
    print_action action = VEC_POP(&s.actions);
    uint32_t first_action = s.actions.len;
    switch (action) {
      case PRINT_NL:
        putc('\n', f);
        break;
      case PRINT_SPACE:
        putc(' ', f);
        break;
      case PRINT_CLOSE_PAREN:
        putc(')', f);
        break;
      case PRINT_NODE: {
        NODE_IND_T node = VEC_POP(&s.node_stack);
        uint32_t first_node = s.node_stack.len;
        print_node(f, &s, node);
        reverse_arbitrary(&s.node_stack.data[first_node],
                          MAX(first_node, s.node_stack.len) - first_node,
                          sizeof(NODE_IND_T));
        break;
      }
      case PRINT_COLON_SPACE:
        fputs(": ", f);
        break;
      case PRINT_COMMA_SPACE:
        fputs(", ", f);
        break;
      default:
        break;
    }
    reverse_arbitrary(&s.actions.data[first_action],
                      MAX(first_action, s.actions.len) - first_action,
                      sizeof(print_action));
  }

  VEC_FREE(&s.actions);
  VEC_FREE(&s.node_stack);
}
