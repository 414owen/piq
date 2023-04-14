// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>

#include "ast_meta.h"
#include "diagnostic.h"
#include "bitset.h"
#include "parse_tree.h"
#include "traverse.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_STR,
  POP_TUP_BS,
} print_action;

VEC_DECL(print_action);

static const tree_node_repr pt_subs_type_arr[] = {
  [PT_ALL_LEN] = SUBS_NONE,
  [PT_ALL_EX_INT] = SUBS_NONE,
  [PT_ALL_MULTI_TERM_NAME] = SUBS_NONE,
  [PT_ALL_MULTI_TYPE_PARAM_NAME] = SUBS_NONE,
  [PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME] = SUBS_NONE,
  [PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME] = SUBS_NONE,
  [PT_ALL_EX_TERM_NAME] = SUBS_NONE,
  [PT_ALL_EX_UPPER_NAME] = SUBS_NONE,
  [PT_ALL_EX_STRING] = SUBS_NONE,
  [PT_ALL_EX_UNIT] = SUBS_NONE,
  [PT_ALL_PAT_WILDCARD] = SUBS_NONE,
  [PT_ALL_PAT_UNIT] = SUBS_NONE,
  [PT_ALL_PAT_STRING] = SUBS_NONE,
  [PT_ALL_PAT_LIST] = SUBS_NONE,
  [PT_ALL_PAT_INT] = SUBS_NONE,
  [PT_ALL_PAT_DATA_CONSTRUCTOR_NAME] = SUBS_NONE,
  [PT_ALL_TY_UNIT] = SUBS_NONE,
  [PT_ALL_TY_CONSTRUCTOR_NAME] = SUBS_NONE,
  [PT_ALL_TY_PARAM_NAME] = SUBS_NONE,

  [PT_ALL_TY_LIST] = SUBS_ONE,

  [PT_ALL_EX_AS] = SUBS_TWO,
  [PT_ALL_EX_TUP] = SUBS_TWO,
  [PT_TY_TUP] = SUBS_TWO,
  [PT_TY_CONSTRUCTION] = SUBS_TWO,
  [PT_ALL_STATEMENT_SIG] = SUBS_TWO,
  [PT_ALL_STATEMENT_LET] = SUBS_TWO,
  [PT_ALL_PAT_TUP] = SUBS_TWO,
  [PT_ALL_PAT_CONSTRUCTION] = SUBS_TWO,

  [PT_ALL_TY_FN] = SUBS_EXTERNAL,
  [PT_ALL_EX_CALL] = SUBS_EXTERNAL,
  [PT_ALL_EX_FN] = SUBS_EXTERNAL,
  [PT_ALL_EX_IF] = SUBS_EXTERNAL,
  [PT_ALL_EX_LIST] = SUBS_EXTERNAL,
  [PT_ALL_EX_FUN_BODY] = SUBS_EXTERNAL,
  [PT_ALL_STATEMENT_FUN] = SUBS_EXTERNAL,
  [PT_ALL_STATEMENT_DATA_DECLARATION] = SUBS_EXTERNAL,
  [PT_ALL_MULTI_DATA_CONSTRUCTORS] = SUBS_EXTERNAL,
  [PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL] = SUBS_EXTERNAL,
  [PT_ALL_MULTI_TYPE_PARAMS] = SUBS_EXTERNAL,
};

const tree_node_repr *pt_subs_type = pt_subs_type_arr;

static const char *parse_node_strings_arr[] = {
  [PT_ALL_LEN] = "",
  [PT_ALL_PAT_DATA_CONSTRUCTOR_NAME] = "DataConstructorName",
  [PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME] = "DataConstructorName",
  [PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME] = "TypeConstructorName",
  [PT_ALL_MULTI_TYPE_PARAM_NAME] = "TypeParamName",
  [PT_ALL_TY_CONSTRUCTOR_NAME] = "TypeConstructorName",
  [PT_ALL_TY_PARAM_NAME] = "TypeVariable",
  [PT_ALL_PAT_WILDCARD] = "WildcardPattern",
  [PT_ALL_STATEMENT_LET] = "LetStatement",
  [PT_ALL_EX_TERM_NAME] = "TermNameExpression",
  [PT_ALL_EX_UPPER_NAME] = "ConstructorNameExpression",
  [PT_ALL_EX_AS] = "AsExpression",
  [PT_ALL_EX_CALL] = "CallExpression",
  [PT_ALL_TY_CONSTRUCTION] = "TypeConstructionType",
  [PT_ALL_PAT_CONSTRUCTION] = "DataConstructionPattern",
  [PT_ALL_EX_FN] = "AnonymousFunctionExpression",
  [PT_ALL_TY_FN] = "FunctionType",
  [PT_ALL_EX_FUN_BODY] = "FunctionBodyExpression",
  [PT_ALL_STATEMENT_FUN] = "FunctionStatement",
  [PT_ALL_EX_IF] = "IfExpression",
  [PT_ALL_PAT_INT] = "IntPattern",
  [PT_ALL_EX_INT] = "IntExpression",
  [PT_ALL_PAT_LIST] = "ListPattern",
  [PT_ALL_EX_LIST] = "ListExpression",
  [PT_ALL_TY_LIST] = "ListType",
  [PT_ALL_MULTI_TERM_NAME] = "TermName",
  [PT_ALL_STATEMENT_SIG] = "TypeSignatureStatement",
  [PT_ALL_PAT_STRING] = "StringPattern",
  [PT_ALL_EX_STRING] = "StringExpression",
  [PT_ALL_PAT_TUP] = "TuplePattern",
  [PT_ALL_TY_TUP] = "TupleType",
  [PT_ALL_EX_TUP] = "TupleExpression",
  [PT_ALL_TY_UNIT] = "UnitType",
  [PT_ALL_PAT_UNIT] = "UnitPattern",
  [PT_ALL_EX_UNIT] = "UnitExpression",
  [PT_ALL_STATEMENT_DATA_DECLARATION] = "DataDeclarationStatement",
  [PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL] = "DataConstructorDeclaration",
  [PT_ALL_MULTI_TYPE_PARAMS] = "TypeParameters",
  [PT_ALL_MULTI_DATA_CONSTRUCTORS] = "DataConstructors",
};

const char **parse_node_strings = parse_node_strings_arr;

static const parse_node_category parse_node_categories_arr[] = {
  [PT_ALL_EX_AS] = PT_C_EXPRESSION,
  [PT_ALL_EX_CALL] = PT_C_EXPRESSION,
  [PT_ALL_EX_FN] = PT_C_EXPRESSION,
  [PT_ALL_EX_FUN_BODY] = PT_C_EXPRESSION,
  [PT_ALL_EX_IF] = PT_C_EXPRESSION,
  [PT_ALL_EX_INT] = PT_C_EXPRESSION,
  [PT_ALL_EX_LIST] = PT_C_EXPRESSION,
  [PT_ALL_EX_STRING] = PT_C_EXPRESSION,
  [PT_ALL_EX_TERM_NAME] = PT_C_EXPRESSION,
  [PT_ALL_EX_TUP] = PT_C_EXPRESSION,
  [PT_ALL_EX_UNIT] = PT_C_EXPRESSION,
  [PT_ALL_EX_UPPER_NAME] = PT_C_EXPRESSION,

  [PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL] = PT_C_NONE,
  [PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME] = PT_C_NONE,
  [PT_ALL_MULTI_DATA_CONSTRUCTORS] = PT_C_NONE,
  [PT_ALL_MULTI_TERM_NAME] = PT_C_NONE,
  [PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME] = PT_C_NONE,
  [PT_ALL_MULTI_TYPE_PARAM_NAME] = PT_C_NONE,
  [PT_ALL_MULTI_TYPE_PARAMS] = PT_C_NONE,

  [PT_ALL_PAT_CONSTRUCTION] = PT_C_PATTERN,
  [PT_ALL_PAT_DATA_CONSTRUCTOR_NAME] = PT_C_PATTERN,
  [PT_ALL_PAT_INT] = PT_C_PATTERN,
  [PT_ALL_PAT_LIST] = PT_C_PATTERN,
  [PT_ALL_PAT_STRING] = PT_C_PATTERN,
  [PT_ALL_PAT_TUP] = PT_C_PATTERN,
  [PT_ALL_PAT_UNIT] = PT_C_PATTERN,
  [PT_ALL_PAT_WILDCARD] = PT_C_PATTERN,

  [PT_ALL_STATEMENT_DATA_DECLARATION] = PT_C_STATEMENT,
  [PT_ALL_STATEMENT_FUN] = PT_C_STATEMENT,
  [PT_ALL_STATEMENT_LET] = PT_C_STATEMENT,
  [PT_ALL_STATEMENT_SIG] = PT_C_STATEMENT,

  [PT_ALL_TY_CONSTRUCTION] = PT_C_TYPE,
  [PT_ALL_TY_CONSTRUCTOR_NAME] = PT_C_TYPE,
  [PT_ALL_TY_FN] = PT_C_TYPE,
  [PT_ALL_TY_LIST] = PT_C_TYPE,
  [PT_ALL_TY_PARAM_NAME] = PT_C_TYPE,
  [PT_ALL_TY_TUP] = PT_C_TYPE,
  [PT_ALL_TY_UNIT] = PT_C_TYPE,
  [PT_ALL_LEN] = 0,
};

const parse_node_category *parse_node_categories = parse_node_categories_arr;

typedef struct {
  vec_print_action actions;
  vec_node_ind node_stack;
  vec_string string_stack;
  bitset in_tuple;
  parse_tree tree;
  const char *input;
  FILE *out;
} printer_state;

static void push_str(printer_state *s, const char *str) {
  print_action action = PRINT_STR;
  VEC_PUSH(&s->actions, action);
  VEC_PUSH(&s->string_stack, str);
}

static void push_node(printer_state *s, node_ind_t node) {
  print_action action = PRINT_NODE;
  VEC_PUSH(&s->actions, action);
  VEC_PUSH(&s->node_stack, node);
}

static void print_source(printer_state *s, span span) {
  fprintf(s->out, "%.*s", span.len, s->input + span.start);
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

static void print_separated(printer_state *s, const node_ind_t *restrict inds,
                            node_ind_t amt, const char *sep) {
  for (uint16_t i = 0; i < amt; i++) {
    if (i > 0)
      push_str(s, sep);
    push_node(s, inds[i]);
  }
}

HEDLEY_PRINTF_FORMAT(2, 6)
static void print_compound(printer_state *restrict s,
                           const char *restrict prefix,
                           const char *restrict sep, char *terminator,
                           parse_node node, ...) {
  va_list rest;
  va_start(rest, node);
  vfprintf(s->out, prefix, rest);
  va_end(rest);

  bs_push(&s->in_tuple, is_tuple(node.type));
  switch (pt_subs_type[node.type.all]) {
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
      print_separated(s, &s->tree.inds[node.subs_start], node.sub_amt, sep);
      break;
  }
  print_action action = POP_TUP_BS;
  VEC_PUSH(&s->actions, action);
  push_str(s, terminator);
}

static void print_compound_normal(printer_state *restrict s, parse_node node) {
  print_compound(
    s, "(%s ", " ", ")", node, parse_node_strings_arr[node.type.all]);
}

static void print_atom_normal(printer_state *restrict s,
                              parse_node_type_all node_type, span span) {
  fprintf(s->out,
          "(%s %.*s)",
          parse_node_strings_arr[node_type],
          span.len,
          s->input + span.start);
}

static void print_node(printer_state *s, node_ind_t node_ind) {
  parse_node node = s->tree.nodes[node_ind];
  span span = s->tree.spans != NULL ? s->tree.spans[node_ind] : node.data.span;
  switch (node.type.all) {
    case PT_ALL_LEN:
      // TODO impossible
      break;
    case PT_ALL_PAT_INT:
    case PT_ALL_EX_INT:
    case PT_ALL_TY_PARAM_NAME:
    case PT_ALL_TY_CONSTRUCTOR_NAME:
    case PT_ALL_EX_UPPER_NAME:
    case PT_ALL_PAT_WILDCARD:
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_MULTI_TERM_NAME:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME:
    case PT_ALL_MULTI_TYPE_PARAM_NAME:
    case PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME:
    case PT_ALL_PAT_DATA_CONSTRUCTOR_NAME:
      print_atom_normal(s, node.type.all, span);
      break;
    case PT_ALL_EX_AS:
    case PT_ALL_EX_IF:
    case PT_ALL_PAT_CONSTRUCTION:
    case PT_ALL_TY_CONSTRUCTION:
    case PT_ALL_EX_FUN_BODY:
    case PT_ALL_EX_CALL:
    case PT_ALL_STATEMENT_LET:
    case PT_ALL_STATEMENT_SIG:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
    case PT_ALL_MULTI_DATA_CONSTRUCTORS:
    case PT_ALL_TY_FN:
    case PT_ALL_EX_FN:
    case PT_ALL_STATEMENT_DATA_DECLARATION:
      print_compound_normal(s, node);
      break;
    case PT_ALL_MULTI_TYPE_PARAMS:
      if (node.sub_amt > 0) {
        print_compound_normal(s, node);
      } else {
        // can only be followed by a string (space)
        VEC_POP_(&s->actions);
        VEC_POP_(&s->string_stack);
      }
      break;
    case PT_ALL_TY_UNIT:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_EX_UNIT: {
      fputs("()", s->out);
      break;
    }
    case PT_ALL_STATEMENT_FUN:
      fprintf(s->out, "(%s ", parse_node_strings_arr[node.type.all]);
      push_node(s, PT_FUN_BINDING_IND(s->tree.inds, node));
      if (PT_FUN_PARAM_AMT(node) > 0) {
        push_str(s, " ");
      }
      print_separated(s,
                      &PT_FUN_PARAM_IND(s->tree.inds, node, 0),
                      PT_FUN_PARAM_AMT(node),
                      " ");
      push_str(s, " ");
      push_node(s, PT_FN_BODY_IND(s->tree.inds, node));
      push_str(s, ")");
      break;
    case PT_ALL_PAT_TUP:
    case PT_ALL_TY_TUP:
    case PT_ALL_EX_TUP:
      if (bs_peek(&s->in_tuple)) {
        // right-associated 2-tuples are written as n-tuples
        print_compound(s, "", ", ", "", node);
      } else {
        print_compound(s, "(", ", ", ")", node);
      }
      break;
    case PT_ALL_TY_LIST:
    case PT_ALL_PAT_LIST:
    case PT_ALL_EX_LIST:
      print_compound(s, "[", ", ", "]", node);
      break;
    case PT_ALL_PAT_STRING:
    case PT_ALL_EX_STRING:
      print_source(s, span);
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
    print_action action;
    VEC_POP(&s.actions, &action);
    switch (action) {
      case PRINT_STR: {
        char *str;
        VEC_POP(&s.string_stack, &str);
        fputs(str, f);
        break;
      }
      case PRINT_NODE: {
        node_ind_t node;
        VEC_POP(&s.node_stack, &node);
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
  free((void *)tree.spans);
}

void free_parse_tree_res(parse_tree_res res) {
  if (res.success) {
    free_parse_tree(res.tree);
  } else {
    free(res.errors.expected);
  }
}

void print_parse_errors(FILE *f, const char *restrict input,
                        const token *restrict tokens, parse_errors errors) {
  fputs("Parsing failed:\n", f);
  token t = tokens[errors.error_pos];
  format_error_ctx(f, input, t.start, t.len);
  fputs("\nExpected one of: ", f);
  print_tokens(f, errors.expected, errors.expected_amt);
}

char *print_parse_errors_string(const char *restrict input,
                                const token *restrict tokens,
                                const parse_tree_res pres) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_parse_errors(ss.stream, input, tokens, pres.errors);
  ss_finalize(&ss);
  return ss.string;
}
