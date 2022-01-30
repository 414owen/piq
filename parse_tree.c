#include <stdio.h>

#include "parse_tree.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_CLOSE_PAREN,
  PRINT_SPACE,
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
  VEC_PUSH(&s->actions, PRINT_CLOSE_PAREN);
  for (uint16_t i = 0; i < node.sub_amt; i++) {
    if (i > 0)
      VEC_PUSH(&s->actions, PRINT_SPACE);
    VEC_PUSH(&s->actions, PRINT_NODE);
    VEC_PUSH(&s->node_stack,
             s->tree.inds.data[node.subs_start + node.sub_amt - 1 - i]);
  }
}

static void print_node(FILE *f, printer_state *s, NODE_IND_T node_ind) {
  parse_node node = s->tree.nodes.data[node_ind];
  switch (node.type) {
    case PT_ROOT:
      for (size_t i = 0; i < node.sub_amt; i++) {
        if (i > 0)
          VEC_PUSH(&s->actions, PRINT_NL);
        VEC_PUSH(&s->actions, PRINT_NODE);
        VEC_PUSH(&s->node_stack,
                 s->tree.inds.data[node.subs_start + node.sub_amt - 1 - i]);
      }
      break;
    case PT_FN:
      fputs("(Fn (", f);
      for (size_t i = 0; i < node.sub_amt - 1; i++) {
        if (i > 0)
          putc(' ', f);
        parse_node param =
          s->tree.nodes.data[s->tree.inds.data[node.subs_start + i]];
        fprintf(f, "%.*s", 1 + param.end - param.start,
                s->file.data + param.start);
      }
      fputs(") ", f);
      VEC_PUSH(&s->node_stack,
               s->tree.inds.data[node.subs_start + node.sub_amt - 1]);
      static const print_action actions[] = {PRINT_CLOSE_PAREN, PRINT_NODE};
      VEC_APPEND(&s->actions, STATIC_LEN(actions), actions);
      break;
    case PT_CALL:
      print_compound(f, s, "(Call ", node);
      break;
    case PT_INT:
      fprintf(f, "(Int %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_NAME:
      fprintf(f, "(Name %.*s)", 1 + node.end - node.start,
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
      case PRINT_NODE:
        print_node(f, &s, VEC_POP(&s.node_stack));
        break;
    }
  }
  VEC_FREE(&s.actions);
  VEC_FREE(&s.node_stack);
}
