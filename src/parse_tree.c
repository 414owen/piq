#include <stdio.h>

#include "ast_meta.h"
#include "diagnostic.h"
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

parse_node_category parse_node_categories[] = {
  [PT_ALL_EX_CALL] = PT_C_EXPRESSION,
  [PT_ALL_EX_FN] = PT_C_EXPRESSION,
  [PT_ALL_EX_FUN_BODY] = PT_C_EXPRESSION,
  [PT_ALL_EX_IF] = PT_C_EXPRESSION,
  [PT_ALL_EX_INT] = PT_C_EXPRESSION,
  [PT_ALL_EX_LIST] = PT_C_EXPRESSION,
  [PT_ALL_EX_STRING] = PT_C_EXPRESSION,
  [PT_ALL_EX_TUP] = PT_C_EXPRESSION,
  [PT_ALL_EX_AS] = PT_C_EXPRESSION,
  [PT_ALL_EX_UNIT] = PT_C_EXPRESSION,

  [PT_ALL_MULTI_UPPER_NAME] = PT_C_NONE,
  [PT_ALL_MULTI_LOWER_NAME] = PT_C_NONE,

  [PT_ALL_PAT_WILDCARD] = PT_C_PATTERN,
  [PT_ALL_PAT_TUP] = PT_C_PATTERN,
  [PT_ALL_PAT_UNIT] = PT_C_PATTERN,
  [PT_ALL_PAT_CONSTRUCTION] = PT_C_PATTERN,
  [PT_ALL_PAT_STRING] = PT_C_PATTERN,
  [PT_ALL_PAT_INT] = PT_C_PATTERN,
  [PT_ALL_PAT_LIST] = PT_C_PATTERN,

  [PT_ALL_STATEMENT_SIG] = PT_C_STATEMENT,
  [PT_ALL_STATEMENT_FUN] = PT_C_STATEMENT,
  [PT_ALL_STATEMENT_LET] = PT_C_STATEMENT,

  [PT_ALL_TY_CONSTRUCTION] = PT_C_TYPE,
  [PT_ALL_TY_LIST] = PT_C_TYPE,
  [PT_ALL_TY_FN] = PT_C_TYPE,
  [PT_ALL_TY_TUP] = PT_C_TYPE,
  [PT_ALL_TY_UNIT] = PT_C_TYPE,
};

// the reason we have a `node.type.all` at all, and the
// stable numbering, is so far basically because of this
// function.
static tree_node_repr subs_type(parse_node_type type) {
  tree_node_repr res = SUBS_NONE;
  switch (type.all) {
    case PT_ALL_EX_INT:
    case PT_ALL_MULTI_LOWER_NAME:
    case PT_ALL_MULTI_UPPER_NAME:
    case PT_ALL_EX_STRING:
    case PT_ALL_EX_UNIT:
    case PT_ALL_PAT_WILDCARD:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_PAT_STRING:
    case PT_ALL_PAT_LIST:
    case PT_ALL_PAT_INT:
    case PT_ALL_TY_UNIT:
      res = SUBS_NONE;
      break;
    case PT_ALL_TY_LIST:
      res = SUBS_ONE;
      break;
    case PT_ALL_EX_CALL:
    case PT_ALL_EX_AS:
    case PT_ALL_EX_FN:
    case PT_ALL_EX_TUP:
    case PT_ALL_TY_FN:
    case PT_TY_TUP:
    case PT_TY_CONSTRUCTION:
    case PT_ALL_STATEMENT_SIG:
    case PT_ALL_STATEMENT_LET:
    case PT_ALL_PAT_TUP:
    case PT_ALL_PAT_CONSTRUCTION:
      res = SUBS_TWO;
      break;
    case PT_ALL_EX_IF:
    case PT_ALL_EX_LIST:
    case PT_ALL_EX_FUN_BODY:
    case PT_ALL_STATEMENT_FUN:
    case PT_ALL_STATEMENT_DATA_DECLARATION:
    // can have multiple types
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
    case PT_ALL_MULTI_TYPE_PARAMS:
      res = SUBS_EXTERNAL;
      break;
  }
  return res;
}

VEC_DECL(print_action);

const char *parse_node_string(parse_node_type type) {
  const char *res = NULL;
  switch (type.all) {
    case PT_ALL_PAT_WILDCARD:
      res = "Wildcard";
      break;
    case PT_ALL_STATEMENT_LET:
      res = "Let";
      break;
    case PT_ALL_EX_AS:
      res = "Typed";
      break;
    case PT_ALL_EX_CALL:
      res = "Call";
      break;
    case PT_ALL_TY_CONSTRUCTION:
    case PT_ALL_PAT_CONSTRUCTION:
      res = "Construction";
      break;
    case PT_ALL_EX_FN:
      res = "Fn";
      break;
    case PT_ALL_TY_FN:
      res = "Fn type";
      break;
    case PT_ALL_EX_FUN_BODY:
      res = "Fun body";
      break;
    case PT_ALL_STATEMENT_FUN:
      res = "Fun";
      break;
    case PT_ALL_EX_IF:
      res = "If";
      break;
    case PT_ALL_PAT_INT:
    case PT_ALL_EX_INT:
      res = "Int";
      break;
    case PT_ALL_PAT_LIST:
    case PT_ALL_EX_LIST:
      res = "List";
      break;
    case PT_ALL_TY_LIST:
      res = "List";
      break;
    case PT_ALL_MULTI_LOWER_NAME:
      res = "Lower name";
      break;
    case PT_ALL_STATEMENT_SIG:
      res = "Sig";
      break;
    case PT_ALL_PAT_STRING:
    case PT_ALL_EX_STRING:
      res = "String";
      break;
    case PT_ALL_PAT_TUP:
    case PT_ALL_TY_TUP:
    case PT_ALL_EX_TUP:
      res = "Tuple";
      break;
    case PT_ALL_TY_UNIT:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_EX_UNIT:
      res = "Unit";
      break;
    case PT_ALL_MULTI_UPPER_NAME:
      res = "Upper name";
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
      res = "Data constructor";
      break;
    case PT_ALL_MULTI_TYPE_PARAMS:
      res = "Type params";
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
  const char *input;
  FILE *out;
} printer_state;

static void push_str(printer_state *s, char *str) {
  print_action action = PRINT_STR;
  VEC_PUSH(&s->actions, action);
  VEC_PUSH(&s->string_stack, str);
}

static void push_node(printer_state *s, node_ind_t node) {
  print_action action = PRINT_NODE;
  VEC_PUSH(&s->actions, action);
  VEC_PUSH(&s->node_stack, node);
}

static void push_source(printer_state *s, node_ind_t node) {
  print_action action = PRINT_SOURCE;
  VEC_PUSH(&s->actions, action);
  VEC_PUSH(&s->node_stack, node);
}

static bool is_tuple(parse_node_type t) {
  switch (t.all) {
    case PT_ALL_EX_TUP:
      return true;
    case PT_ALL_PAT_TUP:
      return true;
    case PT_ALL_TY_TUP:
      return true;
    default:
      return false;
  }
}

static void print_compound(printer_state *s, char *prefix, char *sep,
                           char *terminator, parse_node node) {
  fputs(prefix, s->out);
  bs_push(&s->in_tuple, is_tuple(node.type));
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
  print_action action = POP_TUP_BS;
  VEC_PUSH(&s->actions, action);
  push_str(s, terminator);
}

static void print_node(printer_state *s, node_ind_t node_ind) {
  parse_node node = s->tree.nodes[node_ind];
  switch (node.type.all) {
    case PT_ALL_STATEMENT_SIG: {
      print_compound(s, "(Sig ", " ", ")", node);
      break;
    }
    case PT_ALL_STATEMENT_LET: {
      print_compound(s, "(Let ", " ", ")", node);
      break;
    }
    case PT_ALL_TY_UNIT:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_EX_UNIT: {
      fputs("()", s->out);
      break;
    }
    case PT_ALL_STATEMENT_FUN:
      print_compound(s, "(Fun ", " ", ")", node);
      break;
    case PT_ALL_TY_FN:
    case PT_ALL_EX_FN:
      print_compound(s, "(Fn ", " ", ")", node);
      break;
    case PT_ALL_PAT_CONSTRUCTION:
    case PT_ALL_TY_CONSTRUCTION: {
      print_compound(s, "(Construct ", " ", ")", node);
      break;
    }
    case PT_ALL_EX_FUN_BODY: {
      print_compound(s, "(Body ", " ", ")", node);
      break;
    }
    case PT_ALL_EX_CALL:
      print_compound(s, "(Call ", " ", ")", node);
      break;
    case PT_ALL_PAT_INT:
    case PT_ALL_EX_INT:
      fprintf(s->out, "(Int %.*s)", node.span.len, s->input + node.span.start);
      break;
    case PT_ALL_MULTI_UPPER_NAME:
      fprintf(
        s->out, "(Uname %.*s)", node.span.len, s->input + node.span.start);
      break;
    case PT_PAT_WILDCARD:
      fprintf(
        s->out, "(Wildcard %.*s)", node.span.len, s->input + node.span.start);
      break;
    case PT_EX_LOWER_NAME:
      fprintf(
        s->out, "(Lname %.*s)", node.span.len, s->input + node.span.start);
      break;
    case PT_ALL_EX_IF:
      print_compound(s, "(If ", " ", ")", node);
      break;
    case PT_ALL_PAT_TUP:
    case PT_ALL_TY_TUP:
    case PT_ALL_EX_TUP:
      if (bs_peek(&s->in_tuple)) {
        print_compound(s, "", ", ", "", node);
      } else {
        print_compound(s, "(", ", ", ")", node);
      }
      break;
    case PT_ALL_EX_AS:
      print_compound(s, "(As ", " ", ")", node);
      break;
    case PT_ALL_TY_LIST:
    case PT_ALL_PAT_LIST:
    case PT_ALL_EX_LIST:
      print_compound(s, "[", ", ", "]", node);
      break;
    case PT_ALL_PAT_STRING:
    case PT_ALL_EX_STRING:
      push_source(s, node_ind);
      break;
    case PT_ALL_STATEMENT_DATA_DECLARATION:
      print_compound(s, "(Data Decl:", ", ", ")", node);
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
      print_compound(s, "(Data Constructor:", ", ", ")", node);
      break;
    case PT_ALL_MULTI_TYPE_PARAMS:
      print_compound(s, "(Type params:", ", ", ")", node);
      break;
  }
}

void print_parse_tree(FILE *f, const char *restrict input, parse_tree tree) {
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
  for (node_ind_t i = 0; i < tree.root_subs_amt; i++) {
    node_ind_t ind = tree.root_subs_start + tree.root_subs_amt - 1 - i;
    push_node(&s, tree.inds[ind]);
    if (i != tree.root_subs_amt - 1) {
      push_str(&s, "\n");
    }
  }

  while (s.actions.len > 0) {
    fflush(f);
    print_action action = VEC_POP(&s.actions);
    switch (action) {
      case PRINT_SOURCE: {
        parse_node node = tree.nodes[VEC_POP(&s.node_stack)];
        fprintf(f, "%.*s", node.span.len, input + node.span.start);
        break;
      }
      case PRINT_STR:
        fputs(VEC_POP(&s.string_stack), f);
        break;
      case PRINT_NODE: {
        node_ind_t node = VEC_POP(&s.node_stack);
        uint32_t first_action = s.actions.len;
        uint32_t first_node = s.node_stack.len;
        uint32_t first_string = s.string_stack.len;

        print_node(&s, node);

        reverse_arbitrary(VEC_GET_PTR(s.node_stack, first_node),
                          MAX(first_node, s.node_stack.len) - first_node,
                          sizeof(node_ind_t));
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

char *print_parse_tree_str(const char *restrict input, const parse_tree tree) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_parse_tree(ss.stream, input, tree);
  ss_finalize(&ss);
  return ss.string;
}

void free_parse_tree(parse_tree tree) {
  free((void *)tree.inds);
  free((void *)tree.nodes);
}

void free_parse_tree_res(parse_tree_res res) {
  if (res.succeeded) {
    free_parse_tree(res.tree);
  } else {
    free(res.expected);
  }
}

// TODO add line/col number
void print_parse_tree_error(FILE *f, const char *restrict input,
                            const token *restrict tokens,
                            const parse_tree_res pres) {
  fputs("Parsing failed:\n", f);
  token t = tokens[pres.error_pos];
  format_error_ctx(f, input, t.start, t.len);
  fputs("\nExpected one of: ", f);
  print_tokens(f, pres.expected, pres.expected_amt);
}

MALLOC_ATTR_2(free, 1)
char *print_parse_tree_error_string(const char *restrict input,
                                    const token *restrict tokens,
                                    const parse_tree_res pres) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_parse_tree_error(ss.stream, input, tokens, pres);
  ss_finalize(&ss);
  return ss.string;
}
