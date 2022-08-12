#include <stdio.h>

#include "ast_meta.h"
#include "bitset.h"
#include "parse_tree.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_STR,
  PRINT_SOURCE,
  POP_TUP_BS,
} print_action;

// Maybe at some point we'll need context?
tree_node_repr subs_type(parse_node_type type) {
  tree_node_repr res;
  switch (type) {
    case PT_INT:
    case PT_LOWER_NAME:
    case PT_UPPER_NAME:
    case PT_STRING:
    case PT_UNIT:
      res = SUBS_NONE;
      break;
    case PT_LIST_TYPE:
      res = SUBS_ONE;
      break;
    case PT_CALL:
    case PT_AS:
    case PT_FN:
    case PT_FN_TYPE:
    case PT_SIG:
    case PT_TUP:
      res = SUBS_TWO;
      break;
    case PT_CONSTRUCTION:
    case PT_FUN:
    case PT_FUN_BODY:
    case PT_IF:
    case PT_LIST:
    case PT_ROOT:
    case PT_LET:
      res = SUBS_EXTERNAL;
      break;
  }
  return res;
}

VEC_DECL(print_action);

const char *parse_node_string(parse_node_type type) {
  const char *res = NULL;
  switch (type) {
    case PT_LET:
      res = "Let";
      break;
    case PT_AS:
      res = "Typed";
      break;
    case PT_CALL:
      res = "Call";
      break;
    case PT_CONSTRUCTION:
      res = "Constructor";
      break;
    case PT_FN:
      res = "Fn";
      break;
    case PT_FN_TYPE:
      res = "Fn type";
      break;
    case PT_FUN_BODY:
      res = "Fun body";
      break;
    case PT_FUN:
      res = "Fun";
      break;
    case PT_IF:
      res = "If";
      break;
    case PT_INT:
      res = "Int";
      break;
    case PT_LIST:
      res = "List";
      break;
    case PT_LIST_TYPE:
      res = "List type";
      break;
    case PT_LOWER_NAME:
      res = "Lower name";
      break;
    case PT_ROOT:
      res = "Root";
      break;
    case PT_SIG:
      res = "Sig";
      break;
    case PT_STRING:
      res = "String";
      break;
    case PT_TUP:
      res = "Tuple";
      break;
    case PT_UNIT:
      res = "Unit";
      break;
    case PT_UPPER_NAME:
      res = "Upper name";
      break;
  }
  return res;
};

typedef struct {
  vec_print_action actions;
  vec_node_ind node_stack;
  vec_string string_stack;
  bitset in_tuple;
  parse_tree tree;
  char *input;
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
  bs_push(&s->in_tuple, node.type == PT_TUP);
  switch (subs_type(node.type)) {
    case SUBS_NONE:
      break;
    case SUBS_ONE:
      push_node(s, node.sub_a);
      break;
    case SUBS_TWO:
      push_node(s, node.sub_a);
      push_str(s, sep);
      push_node(s, node.sub_b);
      break;
    case SUBS_EXTERNAL:
      for (uint16_t i = 0; i < node.sub_amt; i++) {
        if (i > 0)
          push_str(s, sep);
        push_node(s, s->tree.inds[node.subs_start + i]);
      }
      break;
  }
  VEC_PUSH(&s->actions, POP_TUP_BS);
  push_str(s, terminator);
}

static void print_node(printer_state *s, NODE_IND_T node_ind) {
  parse_node node = s->tree.nodes[node_ind];
  switch (node.type) {
    case PT_SIG: {
      print_compound(s, "(Sig ", " ", ")", node);
      break;
    }
    case PT_LET: {
      print_compound(s, "(Let ", " ", ")", node);
      break;
    }
    case PT_UNIT: {
      fputs("()", s->out);
      break;
    }
    case PT_ROOT:
      for (size_t i = 0; i < node.sub_amt; i++) {
        if (i > 0)
          push_str(s, "\n");
        push_node(s, s->tree.inds[node.subs_start + i]);
      }
      break;
    case PT_FUN:
      print_compound(s, "(Fun ", " ", ")", node);
      break;
    case PT_FN_TYPE:
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
              s->input + node.start);
      break;
    case PT_UPPER_NAME:
      fprintf(s->out, "(Uname %.*s)", 1 + node.end - node.start,
              s->input + node.start);
      break;
    case PT_LOWER_NAME:
      fprintf(s->out, "(Lname %.*s)", 1 + node.end - node.start,
              s->input + node.start);
      break;
    case PT_IF:
      print_compound(s, "(If ", " ", ")", node);
      break;
    case PT_TUP:
      if (bs_peek(&s->in_tuple)) {
        print_compound(s, "", ", ", "", node);
      } else {
        print_compound(s, "(", ", ", ")", node);
      }
      break;
    case PT_AS:
      print_compound(s, "(As ", " ", ")", node);
      break;
    case PT_LIST:
      print_compound(s, "[", ", ", "]", node);
      break;
    case PT_LIST_TYPE:
      print_compound(s, "[", ", ", "]", node);
      break;
    case PT_STRING:
      push_source(s, node_ind);
      break;
  }
}

void print_parse_tree(FILE *f, char *input, parse_tree tree) {
  printer_state s = {
    .actions = VEC_NEW,
    .node_stack = VEC_NEW,
    .string_stack = VEC_NEW,
    .in_tuple = bs_new(),
    .tree = tree,
    .input = input,
    .out = f,
  };
  bs_push(&s.in_tuple, false);
  push_node(&s, tree.root_ind);

  while (s.actions.len > 0) {
    fflush(f);
    print_action action = VEC_POP(&s.actions);
    switch (action) {
      case PRINT_SOURCE: {
        parse_node node = tree.nodes[VEC_POP(&s.node_stack)];
        fprintf(f, "%.*s", 1 + node.end - node.start, input + node.start);
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

        reverse_arbitrary(VEC_GET_PTR(s.node_stack, first_node),
                          MAX(first_node, s.node_stack.len) - first_node,
                          sizeof(NODE_IND_T));
        reverse_arbitrary(VEC_GET_PTR(s.string_stack, first_string),
                          MAX(first_string, s.string_stack.len) - first_string,
                          sizeof(string));
        reverse_arbitrary(VEC_GET_PTR(s.actions, first_action),
                          MAX(first_action, s.actions.len) - first_action,
                          sizeof(print_action));
        break;
      }
      case POP_TUP_BS:
        bs_pop(&s.in_tuple);
        break;
    }
  }

  VEC_FREE(&s.actions);
  VEC_FREE(&s.string_stack);
  VEC_FREE(&s.node_stack);
  bs_free(&s.in_tuple);
}

char *print_parse_tree_str(char *input, parse_tree tree) {
  stringstream *ss = ss_init();
  print_parse_tree(ss->stream, input, tree);
  return ss_finalize(ss);
}

void free_parse_tree_res(parse_tree_res res) {
  if (res.succeeded) {
    free(res.tree.inds);
    free(res.tree.nodes);
  } else {
    free(res.expected);
  }
}
