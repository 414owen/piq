#include <stdio.h>

#include "parse_tree.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_STR,
  PRINT_SOURCE,
} print_action;

VEC_DECL(print_action);

const char *const parse_node_strings[] = {
  [PT_CALL] = "Call",
  [PT_CONSTRUCTION] = "Constructor",
  [PT_FUN] = "Fun",
  [PT_FUN_BODY] = "Fun body",
  [PT_IF] = "If",
  [PT_INT] = "Int",
  [PT_LIST] = "List",
  [PT_LOWER_NAME] = "Lower name",
  [PT_ROOT] = "Root",
  [PT_STRING] = "String",
  [PT_TOP_LEVEL] = "Top level",
  [PT_TUP] = "Tuple",
  [PT_AS] = "Typed",
  [PT_UNIT] = "Unit",
  [PT_UPPER_NAME] = "Upper name",
};

typedef struct {
  vec_print_action actions;
  vec_node_ind node_stack;
  vec_string string_stack;
  parse_tree tree;
  source_file file;
  FILE *out;
} printer_state;

static void push_str(printer_state *s, char *str) {
  VEC_PUSH(&s->actions, PRINT_STR);
  VEC_PUSH(&s->string_stack, str);
}

static void push_node(printer_state *s, NODE_IND_T node) {
  VEC_PUSH(&s->actions, PRINT_NODE);
  VEC_PUSH(&s->node_stack, node);
}

static void push_source(printer_state *s, NODE_IND_T node) {
  VEC_PUSH(&s->actions, PRINT_SOURCE);
  VEC_PUSH(&s->node_stack, node);
}

static void print_compound(printer_state *s, char *prefix, char *sep,
                           char *terminator, parse_node node) {
  fputs(prefix, s->out);
  for (uint16_t i = 0; i < node.sub_amt; i++) {
    if (i > 0)
      push_str(s, sep);
    push_node(s, s->tree.inds.data[node.subs_start + i]);
  }
  push_str(s, terminator);
}

static void print_node(printer_state *s, NODE_IND_T node_ind) {
  parse_node node = s->tree.nodes.data[node_ind];
  switch (node.type) {
    case PT_UNIT: {
      fputs("()", s->out);
      break;
    }
    case PT_ROOT:
      for (size_t i = 0; i < node.sub_amt; i++) {
        if (i > 0)
          push_str(s, "\n");
        push_node(s, s->tree.inds.data[node.subs_start + i]);
      }
      break;
    case PT_TOP_LEVEL:
      fputs("(Let ", s->out);
      VEC_PUSH(&s->actions, PRINT_SOURCE);
      VEC_PUSH(&s->node_stack, s->tree.inds.data[node.subs_start]);
      push_str(s, " ");
      push_node(s, s->tree.inds.data[node.subs_start + 1]);
      push_str(s, ")");
      break;
    case PT_FUN:
      print_compound(s, "(Fun ", " ", ")", node);
      break;
    case PT_FN:
      print_compound(s, "(Fn ", " ", ")", node);
      break;
    case PT_CONSTRUCTION: {
      print_compound(s, "(Construct ", " ", ")", node);
      break;
    }
    case PT_FUN_BODY: {
      print_compound(s, "(Body ", " ", ")", node);
      break;
    }
    case PT_CALL:
      print_compound(s, "(Call ", " ", ")", node);
      break;
    case PT_INT:
      fprintf(s->out, "(Int %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_UPPER_NAME:
      fprintf(s->out, "(Uname %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_LOWER_NAME:
      fprintf(s->out, "(Lname %.*s)", 1 + node.end - node.start,
              s->file.data + node.start);
      break;
    case PT_IF:
      print_compound(s, "(If ", " ", ")", node);
      break;
    case PT_TUP:
      print_compound(s, "(", ", ", ")", node);
      break;
    case PT_AS:
      print_compound(s, "(As ", " ", ")", node);
      break;
    case PT_LIST:
      print_compound(s, "[", ", ", "]", node);
      break;
    case PT_STRING:
      push_source(s, node_ind);
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
    .out = f,
  };
  push_node(&s, tree.root_ind);

  while (s.actions.len > 0) {
    fflush(f);
    print_action action = VEC_POP(&s.actions);
    switch (action) {
      case PRINT_SOURCE: {
        parse_node node = tree.nodes.data[VEC_POP(&s.node_stack)];
        fprintf(f, "%.*s", 1 + node.end - node.start, file.data + node.start);
        break;
      }
      case PRINT_STR:
        fputs(VEC_POP(&s.string_stack), f);
        break;
      case PRINT_NODE: {
        NODE_IND_T node = VEC_POP(&s.node_stack);
        uint32_t first_action = s.actions.len;
        uint32_t first_node = s.node_stack.len;
        uint32_t first_string = s.string_stack.len;

        print_node(&s, node);

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
    }
  }

  VEC_FREE(&s.actions);
  VEC_FREE(&s.string_stack);
  VEC_FREE(&s.node_stack);
}
