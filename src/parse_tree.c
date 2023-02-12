#include <stdio.h>

#include "ast_meta.h"
#include "diagnostic.h"
#include "bitset.h"
#include "builtins.h"
#include "parse_tree.h"
#include "scope.h"
#include "traverse.h"
#include "util.h"
#include "vec.h"

typedef enum {
  PRINT_NODE,
  PRINT_STR,
  PRINT_SOURCE,
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
  [PT_ALL_TY_CONSTRUCTOR_NAME] = "TypeName",
  [PT_ALL_TY_PARAM_NAME] = "TVar",
  [PT_ALL_PAT_WILDCARD] = "Wildcard",
  [PT_ALL_STATEMENT_LET] = "Let",
  [PT_ALL_EX_TERM_NAME] = "TermNameExpression",
  [PT_ALL_EX_UPPER_NAME] = "Constructor",
  [PT_ALL_EX_AS] = "Typed",
  [PT_ALL_EX_CALL] = "Call",
  [PT_ALL_TY_CONSTRUCTION] = "Construction",
  [PT_ALL_PAT_CONSTRUCTION] = "Construction",
  [PT_ALL_EX_FN] = "AnonymousFunction",
  [PT_ALL_TY_FN] = "FunctionType",
  [PT_ALL_EX_FUN_BODY] = "FunctionBody",
  [PT_ALL_STATEMENT_FUN] = "FunctionStatement",
  [PT_ALL_EX_IF] = "If",
  [PT_ALL_PAT_INT] = "Int",
  [PT_ALL_EX_INT] = "Int",
  [PT_ALL_PAT_LIST] = "List",
  [PT_ALL_EX_LIST] = "List",
  [PT_ALL_TY_LIST] = "List",
  [PT_ALL_MULTI_TERM_NAME] = "TermName",
  [PT_ALL_STATEMENT_SIG] = "Sig",
  [PT_ALL_PAT_STRING] = "String",
  [PT_ALL_EX_STRING] = "String",
  [PT_ALL_PAT_TUP] = "Tuple",
  [PT_ALL_TY_TUP] = "Tuple",
  [PT_ALL_EX_TUP] = "Tuple",
  [PT_ALL_TY_UNIT] = "UnitType",
  [PT_ALL_PAT_UNIT] = "UnitPattern",
  [PT_ALL_EX_UNIT] = "UnitExpression",
  [PT_ALL_STATEMENT_DATA_DECLARATION] = "Data declaration",
  [PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL] = "Data constructor",
  [PT_ALL_MULTI_TYPE_PARAMS] = "Type params",
  [PT_ALL_MULTI_DATA_CONSTRUCTORS] = "Data constructors",
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

static void print_separated(printer_state *s, const node_ind_t *restrict inds,
                            node_ind_t amt, const char *sep) {
  for (uint16_t i = 0; i < amt; i++) {
    if (i > 0)
      push_str(s, sep);
    push_node(s, inds[i]);
  }
}

static void print_compound(printer_state *s, char *prefix, char *sep,
                           char *terminator, parse_node node) {
  fputs(prefix, s->out);
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

static void print_node(printer_state *s, node_ind_t node_ind) {
  parse_node node = s->tree.nodes[node_ind];
  switch (node.type.all) {
    case PT_ALL_LEN:
      // TODO impossible
      break;
    case PT_ALL_PAT_DATA_CONSTRUCTOR_NAME: {
      fprintf(s->out,
              "(DataConstructorName %.*s)",
              node.span.len,
              s->input + node.span.start);
      break;
    }
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME: {
      fprintf(s->out,
              "(DataConstructorName %.*s)",
              node.span.len,
              s->input + node.span.start);
      break;
    }
    case PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME: {
      fprintf(s->out,
              "(TypeConstructorName %.*s)",
              node.span.len,
              s->input + node.span.start);
      break;
    }
    case PT_ALL_MULTI_TYPE_PARAM_NAME: {
      fprintf(s->out,
              "(TypeParamName %.*s)",
              node.span.len,
              s->input + node.span.start);
      break;
    }
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
      push_str(s, "(Fun ");
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
    case PT_ALL_TY_FN:
    case PT_ALL_EX_FN: {
      push_str(s, "(Fn (");
      print_separated(
        s, &PT_FN_PARAM_IND(s->tree.inds, node, 0), PT_FN_PARAM_AMT(node), " ");
      push_str(s, ") ");
      push_node(s, PT_FN_BODY_IND(s->tree.inds, node));
      push_str(s, ")");
      break;
    }
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
    case PT_ALL_TY_PARAM_NAME:
      fprintf(
        s->out, "(TypeVar %.*s)", node.span.len, s->input + node.span.start);
      break;
    case PT_ALL_TY_CONSTRUCTOR_NAME:
      fprintf(s->out,
              "(TypeConstructorName %.*s)",
              node.span.len,
              s->input + node.span.start);
      break;
    case PT_ALL_EX_UPPER_NAME:
      fprintf(s->out,
              "(Constructor %.*s)",
              node.span.len,
              s->input + node.span.start);
      break;
    case PT_ALL_PAT_WILDCARD:
      fprintf(
        s->out, "(Wildcard %.*s)", node.span.len, s->input + node.span.start);
      break;
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_MULTI_TERM_NAME:
      fprintf(
        s->out, "(TermName %.*s)", node.span.len, s->input + node.span.start);
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
      print_compound(s, "(DataDecl ", " ", ")", node);
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTORS:
      print_compound(s, "", "", "", node);
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
      print_compound(s, "", ", ", "", node);
      break;
    case PT_ALL_MULTI_TYPE_PARAMS:
      print_compound(s, "(Type params: [", ", ", "])", node);
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
      case PRINT_SOURCE: {
        node_ind_t node_ind;
        VEC_POP(&s.node_stack, &node_ind);
        parse_node node = tree.nodes[node_ind];
        fprintf(f, "%.*s", node.span.len, input + node.span.start);
        break;
      }
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
}

void free_parse_tree_res(parse_tree_res res) {
  if (res.type == PRT_SUCCESS) {
    free_parse_tree(res.tree);
  } else {
    free(res.expected);
  }
}

static void print_honest_parse_error(FILE *f, const char *restrict input,
                                     const token *restrict tokens,
                                     const parse_tree_res pres) {
  fputs("Parsing failed:\n", f);
  token t = tokens[pres.error_pos];
  format_error_ctx(f, input, t.start, t.len);
  fputs("\nExpected one of: ", f);
  print_tokens(f, pres.expected, pres.expected_amt);
}

static void print_semantic_error(FILE *f, const char *restrict input,
                                 const parse_tree tree,
                                 const semantic_error err) {
  switch (err.type) {
    case SEMERR_OUT_OF_PLACE_SIG: {
      fprintf(f, "Expected binding to match signature:\n");
      parse_node node = tree.nodes[err.sig_index];
      format_error_ctx(f, input, node.span.start, node.span.len);
      break;
    }
  }
}

static void print_semantic_errors(FILE *f, const char *restrict input,
                                  const parse_tree_res pres) {
  for (uint32_t i = 0; i < pres.semantic_errors.len; i++) {
    print_semantic_error(f, input, pres.tree, VEC_GET(pres.semantic_errors, i));
  }
}

// TODO add line/col number
void print_parse_errors(FILE *f, const char *restrict input,
                        const token *restrict tokens,
                        const parse_tree_res pres) {
  switch (pres.type) {
    case PRT_SUCCESS:
      // impossible
      break;
    case PRT_PARSE_ERROR:
      print_honest_parse_error(f, input, tokens, pres);
      break;
    case PRT_SEMANTIC_ERRORS:
      print_semantic_errors(f, input, pres);
      break;
  }
}

MALLOC_ATTR_2(free, 1)
char *print_parse_errors_string(const char *restrict input,
                                const token *restrict tokens,
                                const parse_tree_res pres) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_parse_errors(ss.stream, input, tokens, pres);
  ss_finalize(&ss);
  return ss.string;
}

typedef struct {
  vec_binding not_found;
  parse_tree tree;
  const char *restrict input;
  pt_traverse_elem elem;
  scope environment;
  scope type_environment;
} scope_calculator_state;

// Setting the binding's bariable_index to the thing we're about to push
// is theoretically unnecessary work, but it's nit to have a concreate
// index to look things up with in eg. the llvm stage.
static void precalculate_scope_push(scope_calculator_state *state) {
  switch (state->elem.data.node_data.node.type.binding) {
    case PT_BIND_FUN: {
      node_ind_t binding_ind =
        PT_FUN_BINDING_IND(state->tree.inds, state->elem.data.node_data.node);
      parse_node *binding_node = &state->tree.nodes[binding_ind];
      binding b = binding_node->span;
      binding_node->variable_index = state->environment.bindings.len;
      scope_push(&state->environment, b);
      break;
    }
    case PT_BIND_WILDCARD: {
      parse_node *node =
        &state->tree.nodes[state->elem.data.node_data.node_index];
      node->variable_index = state->environment.bindings.len;
      binding b = node->span;
      scope_push(&state->environment, b);
      break;
    }
    case PT_BIND_LET: {
      node_ind_t binding_ind = PT_LET_BND_IND(state->elem.data.node_data.node);
      parse_node *node = &state->tree.nodes[binding_ind];
      node->variable_index = state->environment.bindings.len;
      binding b = node->span;
      scope_push(&state->environment, b);
      break;
    }
  }
}

static void precalculate_scope_visit(scope_calculator_state *state) {
  parse_node node = state->elem.data.node_data.node;
  scope scope;
  switch (node.type.all) {
    case PT_ALL_TY_PARAM_NAME:
    case PT_ALL_TY_CONSTRUCTOR_NAME: {
      scope = state->type_environment;
      break;
    }
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_EX_UPPER_NAME: {
      scope = state->environment;
      break;
    }
    default:
      return;
  }
  node_ind_t index = lookup_str_ref(state->input, scope, node.span);
  if (index == scope.bindings.len) {
    VEC_PUSH(&state->not_found, node.span);
  }
  state->tree.nodes[state->elem.data.node_data.node_index].variable_index =
    index;
}

resolution_errors resolve_bindings(parse_tree tree,
                                   const char *restrict input) {
  pt_traversal traversal = pt_traverse(tree, TRAVERSE_RESOLVE_BINDINGS);
  scope_calculator_state state = {
    .not_found = VEC_NEW,
    .tree = tree,
    .input = input,
    .environment = scope_new(),
    .type_environment = scope_new(),
  };
  bs_push_true_n(&state.type_environment.is_builtin, named_builtin_type_amount);
  bs_push_true_n(&state.environment.is_builtin, builtin_term_amount);
  for (node_ind_t i = 0; i < named_builtin_type_amount; i++) {
    str_ref s = {.builtin = builtin_type_names[i]};
    VEC_PUSH(&state.type_environment.bindings, s);
  }
  for (node_ind_t i = 0; i < builtin_term_amount; i++) {
    str_ref s = {.builtin = builtin_term_names[i]};
    VEC_PUSH(&state.environment.bindings, s);
  }
  while (true) {
    state.elem = pt_traverse_next(&traversal);
    switch (state.elem.action) {
      case TR_NEW_BLOCK:
      case TR_LINK_SIG:
        break;
      case TR_POP_TO:
        state.environment.bindings.len = state.elem.data.new_environment_amount;
        break;
      case TR_END: {
        const VEC_LEN_T len = state.not_found.len;
        resolution_errors res = {
          .binding_amt = len,
          .bindings = VEC_FINALIZE(&state.not_found),
        };
        scope_free(state.environment);
        scope_free(state.type_environment);
        return res;
      }
      case TR_PREDECLARE_FN:
      case TR_PUSH_SCOPE_VAR:
        precalculate_scope_push(&state);
        break;
      case TR_VISIT_IN:
        precalculate_scope_visit(&state);
        break;
      case TR_VISIT_OUT:
        break;
    }
  }
}
