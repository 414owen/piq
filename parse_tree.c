#include <stdio.h>

#include "parse_tree.h"
#include "vec.h"
#include "util.h"

typedef enum {
  PRINT_NODE,
  PRINT_CLOSE_PAREN,
  PRINT_SPACE,
} print_action;

VEC_DECL(print_action);

typedef struct {
  vec_print_action actions;
  vec_node_ind node_stack;
  parse_tree tree;
} printer_state;

static void print_compound_subs(FILE *f, printer_state s, parse_node node) {
  VEC_PUSH(&s.actions, PRINT_CLOSE_PAREN);
  for (uint16_t i = 0; i < node.sub_amt; i++) {
    if (i > 0) VEC_PUSH(&s.actions, PRINT_SPACE);
    VEC_PUSH(&s.actions, PRINT_NODE);
    VEC_PUSH(&s.node_stack, s.tree.inds.data[node.subs_start + i]);
  }
}

static void print_node(FILE *f, printer_state s, NODE_IND_T node_ind) {
  parse_node node = s.tree.nodes.data[node_ind];
  switch (node.type) {
    case PT_COMPOUND:
      fputs("(Call ", f);
      print_compound_subs(f, s, node);
      break;
    case PT_NUM:
      fprintf(f, "(Num %.*s)", 1 + node.end - node.start, s.tree.file.data + node.start);
      break;
    case PT_NAME:
      fprintf(f, "(Name %.*s)", 1 + node.end - node.start, s.tree.file.data + node.start);
      break;
    case PT_IF:
      fputs("(If ", f);
      print_compound_subs(f, s, node);
      break;
  }
}

void print_parse_tree(FILE *f, parse_tree tree) {
  printer_state s = {
    .actions = VEC_NEW, 
    .node_stack = VEC_NEW,
    .tree = tree,
  };
  VEC_PUSH(&s.actions, PRINT_NODE);
  VEC_PUSH(&s.node_stack, tree.root_ind);
  while (s.node_stack.len > 0) {
    print_action action = VEC_POP(&s.actions);
    switch (action) {
      case PRINT_SPACE:
        putc(' ', f);
        break;
      case PRINT_CLOSE_PAREN:
        putc(')', f);
        break;
      case PRINT_NODE:
        print_node(f, s, VEC_POP(&s.node_stack));
        break;
    }
  }
  VEC_FREE(&s.node_stack);
}
