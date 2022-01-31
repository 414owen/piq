#include <stdio.h>

#include "parse_tree.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_STR,
} print_action;

VEC_DECL(print_action);

typedef struct {
  vec_print_action actions;
  vec_node_ind node_stack;
  vec_string string_stack;
  parse_tree tree;
  source_file file;
} printer_state;

static void push_str(printer_state *s, char *str) {
  VEC_PUSH(&s->actions, PRINT_STR);
  VEC_PUSH(&s->string_stack, str);
}

static void push_node(printer_state *s, NODE_IND_T node) {
  VEC_PUSH(&s->actions, PRINT_NODE);
  VEC_PUSH(&s->node_stack, node);
}

static void print_compound(FILE *f, printer_state *s, char *prefix, char *sep,
                           parse_node node) {
  fputs(prefix, f);
  for (uint16_t i = 0; i < node.sub_amt; i++) {
    if (i > 0)
      push_str(s, sep);
    push_node(s, s->tree.inds.data[node.subs_start + i]);
  }
  push_str(s, ")");
}

static void print_node(FILE *f, printer_state *s, NODE_IND_T node_ind) {
  parse_node node = s->tree.nodes.data[node_ind];
  switch (node.type) {
    case PT_ROOT:
      for (size_t i = 0; i < node.sub_amt; i++) {
        if (i > 0)
          push_str(s, "\n");
        push_node(s, s->tree.inds.data[node.subs_start + i]);
      }
      break;
    case PT_FN:
      fputs("(Fn (", f);
      for (size_t i = 0; i < (node.sub_amt - 1) / 2; i++) {
        if (i > 0)
          push_str(s, ", ");
        NODE_IND_T param = s->tree.inds.data[node.subs_start + i * 2];
        NODE_IND_T type = s->tree.inds.data[node.subs_start + i * 2 + 1];
        push_node(s, param);
        push_str(s, ": ");
        push_node(s, type);
      }
      push_str(s, ") ");
      push_node(s, s->tree.inds.data[node.subs_start + node.sub_amt - 1]);
      push_str(s, ")");
      break;
    case PT_CALL:
      print_compound(f, s, "(Call ", " ", node);
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
      print_compound(f, s, "(If ", " ", node);
      break;
    case PT_TUP:
      print_compound(f, s, "(Tup ", " ", node);
      break;
    case PT_TYPED:
      print_compound(f, s, "(", ": ", node);
      break;
  }
}

void print_parse_tree(FILE *f, source_file file, parse_tree tree) {
  printer_state s = {
    .actions = VEC_NEW,
    .node_stack = VEC_NEW,
    .string_stack = VEC_NEW,
    .tree = tree,
    .file = file,
  };
  push_node(&s, tree.root_ind);

  while (s.actions.len > 0) {
    print_action action = VEC_POP(&s.actions);
    switch (action) {
      case PRINT_STR:
        fputs(VEC_POP(&s.string_stack), f);
        break;
      case PRINT_NODE: {
        NODE_IND_T node = VEC_POP(&s.node_stack);
        uint32_t first_action = s.actions.len;
        uint32_t first_node = s.node_stack.len;
        uint32_t first_string = s.string_stack.len;

        print_node(f, &s, node);

        reverse_arbitrary(&s.node_stack.data[first_node],
                          MAX(first_node, s.node_stack.len) - first_node,
                          sizeof(NODE_IND_T));
        reverse_arbitrary(&s.string_stack.data[first_string],
                          MAX(first_string, s.string_stack.len) - first_string,
                          sizeof(string));
        reverse_arbitrary(&s.actions.data[first_action],
                          MAX(first_action, s.actions.len) - first_action,
                          sizeof(print_action));
        break;
      }
      default:
        break;
    }
  }

  VEC_FREE(&s.actions);
  VEC_FREE(&s.node_stack);
}
