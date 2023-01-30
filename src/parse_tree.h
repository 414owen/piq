#pragma once

#include <stdio.h>
#include <time.h>

#include "ast_meta.h"
#include "binding.h"
#include "bitset.h"
#include "consts.h"
#include "span.h"
#include "token.h"
#include "vec.h"

#define node_ind_t buf_ind_t

VEC_DECL_CUSTOM(node_ind_t, vec_node_ind);

typedef enum {
  PT_C_NONE,
  PT_C_EXPRESSION,
  PT_C_PATTERN,
  PT_C_STATEMENT,
  PT_C_TYPE,
  PT_C_TOPLEVEL,
} parse_node_category;

// TODO `type` is way too overloaded here.
// Maybe we should use `tag`?

// When editing, make sure to add the category in
// parse_tree.c.
typedef enum {
  PT_ALL_EX_CALL,
  PT_ALL_EX_FN,
  PT_ALL_EX_FUN_BODY,
  PT_ALL_EX_IF,
  PT_ALL_EX_INT,
  PT_ALL_EX_AS,
  PT_ALL_EX_TERM_NAME,
  PT_ALL_EX_UPPER_NAME,
  PT_ALL_EX_LIST,
  PT_ALL_EX_STRING,
  PT_ALL_EX_TUP,
  PT_ALL_EX_UNIT,

  // stuff that is context-agnostic, and that shouldn't really
  // be a node at all...
  // Maybe the fact that these aren't "near" other
  // parse node categories' enums affects the switch output?
  PT_ALL_MULTI_TERM_NAME,
  PT_ALL_MULTI_TYPE_PARAMS,
  PT_ALL_MULTI_TYPE_PARAM_NAME,
  PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME,
  PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME,
  PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL,
  PT_ALL_MULTI_DATA_CONSTRUCTORS,

  PT_ALL_PAT_WILDCARD,
  PT_ALL_PAT_TUP,
  PT_ALL_PAT_UNIT,
  PT_ALL_PAT_DATA_CONSTRUCTOR_NAME,
  PT_ALL_PAT_CONSTRUCTION,
  PT_ALL_PAT_STRING,
  PT_ALL_PAT_INT,
  PT_ALL_PAT_LIST,

  // sig and fun two are also top levels
  PT_ALL_STATEMENT_SIG,
  PT_ALL_STATEMENT_FUN,
  PT_ALL_STATEMENT_LET,
  PT_ALL_STATEMENT_DATA_DECLARATION,

  PT_ALL_TY_CONSTRUCTION,
  PT_ALL_TY_LIST,
  PT_ALL_TY_FN,
  PT_ALL_TY_TUP,
  PT_ALL_TY_UNIT,
  PT_ALL_TY_PARAM_NAME,
  PT_ALL_TY_CONSTRUCTOR_NAME,

  PT_ALL_LEN,
} parse_node_type_all;

typedef enum {
  PT_EX_CALL = PT_ALL_EX_CALL,
  PT_EX_FN = PT_ALL_EX_FN,
  PT_EX_FUN_BODY = PT_ALL_EX_FUN_BODY,
  PT_EX_IF = PT_ALL_EX_IF,
  PT_EX_INT = PT_ALL_EX_INT,
  PT_EX_LIST = PT_ALL_EX_LIST,
  PT_EX_STRING = PT_ALL_EX_STRING,
  PT_EX_TUP = PT_ALL_EX_TUP,
  PT_EX_AS = PT_ALL_EX_AS,
  PT_EX_UNIT = PT_ALL_EX_UNIT,
  PT_EX_LOWER_VAR = PT_ALL_EX_TERM_NAME,
  PT_EX_UPPER_NAME = PT_ALL_EX_UPPER_NAME,
} parse_node_expression_type;

typedef enum {
  PT_PAT_WILDCARD = PT_ALL_PAT_WILDCARD,
  PT_PAT_TUP = PT_ALL_PAT_TUP,
  PT_PAT_UNIT = PT_ALL_PAT_UNIT,
  PT_PAT_CONSTRUCTION = PT_ALL_PAT_CONSTRUCTION,
  PT_PAT_STRING = PT_ALL_PAT_STRING,
  PT_PAT_INT = PT_ALL_PAT_INT,
  PT_PAT_LIST = PT_ALL_PAT_LIST,
  PT_PAT_DATA_CONSTRUCTOR_NAME = PT_ALL_PAT_DATA_CONSTRUCTOR_NAME,
} parse_node_pattern_type;

typedef enum {
  PT_STATEMENT_SIG = PT_ALL_STATEMENT_SIG,
  PT_STATEMENT_FUN = PT_ALL_STATEMENT_FUN,
  PT_STATEMENT_LET = PT_ALL_STATEMENT_LET,
  PT_STATEMENT_DATA_DECLARATION = PT_ALL_STATEMENT_DATA_DECLARATION,
} parse_node_statement_type;

typedef enum {
  PT_TY_CONSTRUCTION = PT_ALL_TY_CONSTRUCTION,
  PT_TY_LIST = PT_ALL_TY_LIST,
  PT_TY_FN = PT_ALL_TY_FN,
  PT_TY_TUP = PT_ALL_TY_TUP,
  PT_TY_UNIT = PT_ALL_TY_UNIT,
  PT_TY_PARAM_NAME = PT_ALL_TY_PARAM_NAME,
  PT_TY_CONSTRUCTOR_NAME = PT_ALL_TY_CONSTRUCTOR_NAME,
} parse_node_type_type;

typedef enum {
  PT_BIND_LET = PT_ALL_STATEMENT_LET,
  PT_BIND_FUN = PT_ALL_STATEMENT_FUN,
  PT_BIND_WILDCARD = PT_ALL_PAT_WILDCARD,
} parse_node_binding_introducer_type;

typedef enum {
  PT_TL_SIG = PT_STATEMENT_SIG,
  PT_TL_FUN = PT_STATEMENT_FUN,
  // TODO let
} parse_node_toplevel_type;

typedef union {
  parse_node_type_all all;
  parse_node_toplevel_type top_level;
  parse_node_statement_type statement;
  parse_node_type_type type;
  parse_node_expression_type expression;
  parse_node_pattern_type pattern;
  parse_node_binding_introducer_type binding;
} parse_node_type;

#define PT_FUN_BINDING_IND(inds, node) inds[node.subs_start + 0]
#define PT_FUN_PARAM_AMT(node) (node.sub_amt - 2)
#define PT_FUN_PARAM_IND(inds, node, i) inds[node.subs_start + 1 + i]
#define PT_FUN_BODY_IND(inds, node) inds[node.subs_start + node.sub_amt - 1]

#define PT_DATA_DECL_TYPENAME_IND(inds, node) inds[node.subs_start + 0]
#define PT_DATA_DECL_TYPEPARAMS_IND(inds, node) inds[node.subs_start + 1]
#define PT_DATA_DECL_DATA_CONSTRUCTORS_IND(inds, node) inds[node.subs_start + 2]

// lambda expression
#define PT_FN_PARAM_AMT(node) ((node).sub_amt - 1)
#define PT_FN_PARAM_IND(inds, node, i) inds[(node).subs_start + i]
#define PT_FN_BODY_IND(inds, node) inds[(node).subs_start + (node).sub_amt - 1]

#define PT_FN_TYPE_PARAM_AMT(node) ((node).sub_amt - 1)
#define PT_FN_TYPE_PARAM_IND(inds, node, i) (inds)[(node).subs_start + i]
#define PT_FN_TYPE_RETURN_IND(inds, node)                                      \
  inds[(node).subs_start + (node).sub_amt - 1]

#define PT_CALL_CALLEE_IND(inds, node) (inds)[(node).subs_start]
#define PT_CALL_PARAM_AMT(node) ((node).sub_amt - 1)
#define PT_CALL_PARAM_IND(inds, node, i) (inds)[(node).subs_start + i + 1]

#define PT_TY_CONSTRUCTION_CALLEE_IND(node) node.sub_a
#define PT_TY_CONSTRUCTION_PARAM_IND(node) node.sub_b

#define PT_IF_COND_IND(inds, node) inds[node.subs_start]
#define PT_IF_A_IND(inds, node) inds[node.subs_start + 1]
#define PT_IF_B_IND(inds, node) inds[node.subs_start + 2]

#define PT_LIST_SUB_AMT(node) node.sub_amt
#define PT_LIST_SUB_IND(inds, node, i) inds[node.subs_start + i]

#define PT_LIST_TYPE_SUB(node) node.sub_a

#define PT_BLOCK_SUBS_START(node) node.subs_start
#define PT_BLOCK_SUB_AMT(node) node.sub_amt
#define PT_BLOCK_SUB_IND(inds, node, i) inds[node.subs_start + i]
#define PT_BLOCK_LAST_SUB_IND(inds, node)                                      \
  inds[node.subs_start + node.sub_amt - 1]

#define PT_ROOT_SUB_AMT(node) PT_BLOCK_SUB_AMT(node)
#define PT_ROOT_SUB_IND(inds, node, i) PT_BLOCK_SUB_IND(inds, node, i)

#define PT_FUN_BODY_SUBS_START(node) PT_BLOCK_SUBS_START(node)
#define PT_FUN_BODY_SUB_AMT(node) PT_BLOCK_SUB_AMT(node)
#define PT_FUN_BODY_SUB_IND(inds, node, i) PT_BLOCK_SUB_IND(inds, node, i)
#define PT_FUN_BODY_LAST_SUB_IND(inds, node) PT_BLOCK_LAST_SUB_IND(inds, node)

#define PT_SIG_BINDING_IND(node) node.sub_a
#define PT_SIG_TYPE_IND(node) node.sub_b

#define PT_TUP_SUB_AMT(node) 2
#define PT_TUP_SUB_A(node) node.sub_a
#define PT_TUP_SUB_B(node) node.sub_b

#define PT_LET_BND_IND(node) node.sub_a
#define PT_LET_VAL_IND(node) node.sub_b

#define PT_AS_TYPE_IND(node) node.sub_a
#define PT_AS_VAL_IND(node) node.sub_b

const char **parse_node_strings;

typedef struct {
  parse_node_type type;

  span span;

  union {
    struct {
      // position of the binding in the parse tree
      // this probably won't be used.
      node_ind_t variable_node_ind;
      // position of the binding in the current scope
      // vector
      node_ind_t variable_index;
    };
    struct {
      node_ind_t subs_start;
      node_ind_t sub_amt;
    };
    struct {
      node_ind_t sub_a;
      node_ind_t sub_b;
    };
  };
} parse_node;

VEC_DECL(parse_node);

typedef struct {
  parse_node *nodes;
  const node_ind_t *inds;
  node_ind_t root_subs_start;
  node_ind_t root_subs_amt;
  // this is set even if parsing errored, for benchmarking
  node_ind_t node_amt;
} parse_tree;

typedef struct {
  parse_tree tree;
  token_type *expected;
  node_ind_t error_pos;
  uint8_t expected_amt;
  bool succeeded;
  struct timespec time_taken;
} parse_tree_res;

parse_tree_res parse(token *tokens, size_t token_amt);

void print_parse_tree(FILE *f, const char *input, const parse_tree tree);
char *print_parse_tree_str(const char *input, const parse_tree tree);
void print_parse_tree_error(FILE *f, const char *input, const token *tokens,
                            const parse_tree_res pres);
char *print_parse_tree_error_string(const char *input, const token *tokens,
                                    const parse_tree_res pres);
void free_parse_tree(parse_tree tree);
void free_parse_tree_res(parse_tree_res res);
const tree_node_repr *pt_subs_type;

typedef enum {
  TR_PUSH_SCOPE_VAR,
  TR_VISIT_IN,
  // TR_VISIT_OUT,
  TR_POP_TO,
  TR_END,
  // used in constraint generation
  TR_LINK_SIG,
} scoped_traverse_action;

VEC_DECL(scoped_traverse_action);

typedef struct {
  const parse_node *nodes;
  const node_ind_t *inds;
  // const node_ind_t node_amt;
  vec_scoped_traverse_action actions;
  node_ind_t environment_amt;
  union {
    vec_node_ind node_stack;
    vec_node_ind amt_stack;
  };
} pt_traversal;

typedef struct {
  node_ind_t node_index;
  parse_node node;
} traversal_node_data;

typedef struct {
  node_ind_t sig_index;
  node_ind_t linked_index;
} traversal_link_sig_data;

typedef union {
  traversal_node_data node_data;
  traversal_link_sig_data link_sig_data;
  node_ind_t new_environment_amount;
} pt_trav_elem_data;

typedef struct {
  scoped_traverse_action action;
  pt_trav_elem_data data;
} pt_traverse_elem;

pt_traversal pt_scoped_traverse(parse_tree tree);
pt_traverse_elem pt_scoped_traverse_next(pt_traversal *traversal);

typedef struct {
  binding *bindings;
  node_ind_t binding_amt;
} resolution_errors;

resolution_errors resolve_bindings(parse_tree tree, const char *restrict input);
