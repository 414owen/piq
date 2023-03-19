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
  PT_C_NONE = 1,
  PT_C_EXPRESSION = 2,
  PT_C_PATTERN = 3,
  PT_C_STATEMENT = 4,
  PT_C_TYPE = 5,
  PT_C_TOPLEVEL = 6,
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
  // TODO this is badly named because constructor names are also terms
  PT_ALL_EX_TERM_NAME,
  // constructor names
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

VEC_DECL(parse_node_type_all);

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

extern const char **parse_node_strings;
extern const parse_node_category *parse_node_categories;

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
      environment_ind_t variable_index;
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

typedef enum {
  PRT_SUCCESS,
  PRT_PARSE_ERROR,
} parse_result_type;

typedef struct {
  union {
    parse_tree tree;
    struct {
      token_type *expected;
      node_ind_t error_pos;
      uint8_t expected_amt;
    };
  };
  parse_result_type type;
  struct timespec time_taken;
} parse_tree_res;

parse_tree_res parse(token *tokens);

void print_parse_tree(FILE *f, const char *input, const parse_tree tree);
char *print_parse_tree_str(const char *input, const parse_tree tree);
void print_parse_errors(FILE *f, const char *input, const token *tokens,
                        const parse_tree_res pres);
char *print_parse_errors_string(const char *input, const token *tokens,
                                const parse_tree_res pres);
void free_parse_tree(parse_tree tree);
void free_parse_tree_res(parse_tree_res res);
extern const tree_node_repr *pt_subs_type;

typedef struct {
  binding *bindings;
  node_ind_t binding_amt;
} resolution_errors;

typedef struct {
  resolution_errors not_found;
#ifdef TIME_NAME_RESOLUTION
  struct timespec time_taken;
  u32 num_names_looked_up;
#endif
} resolution_res;

resolution_res resolve_bindings(parse_tree tree, const char *restrict input);

#define PT_EXPRESSION_CASES                                                    \
  PT_ALL_EX_CALL:                                                              \
  case PT_ALL_EX_FN:                                                           \
  case PT_ALL_EX_FUN_BODY:                                                     \
  case PT_ALL_EX_IF:                                                           \
  case PT_ALL_EX_INT:                                                          \
  case PT_ALL_EX_AS:                                                           \
  case PT_ALL_EX_TERM_NAME:                                                    \
  case PT_ALL_EX_UPPER_NAME:                                                   \
  case PT_ALL_EX_LIST:                                                         \
  case PT_ALL_EX_STRING:                                                       \
  case PT_ALL_EX_TUP:                                                          \
  case PT_ALL_EX_UNIT

#define PT_MULTI_CASES                                                         \
  PT_ALL_MULTI_TERM_NAME:                                                      \
  case PT_ALL_MULTI_TYPE_PARAMS:                                               \
  case PT_ALL_MULTI_TYPE_PARAM_NAME:                                           \
  case PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME:                                     \
  case PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME:                                     \
  case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:                                     \
  case PT_ALL_MULTI_DATA_CONSTRUCTORS

#define PT_PATTERN_CASES                                                       \
  PT_ALL_PAT_WILDCARD:                                                         \
  case PT_ALL_PAT_TUP:                                                         \
  case PT_ALL_PAT_UNIT:                                                        \
  case PT_ALL_PAT_DATA_CONSTRUCTOR_NAME:                                       \
  case PT_ALL_PAT_CONSTRUCTION:                                                \
  case PT_ALL_PAT_STRING:                                                      \
  case PT_ALL_PAT_INT:                                                         \
  case PT_ALL_PAT_LIST

#define PT_STATEMENT_CASES                                                     \
  PT_ALL_STATEMENT_SIG:                                                        \
  case PT_ALL_STATEMENT_FUN:                                                   \
  case PT_ALL_STATEMENT_LET:                                                   \
  case PT_ALL_STATEMENT_DATA_DECLARATION

#define PT_TYPE_CASES                                                          \
  PT_ALL_TY_CONSTRUCTION:                                                      \
  case PT_ALL_TY_LIST:                                                         \
  case PT_ALL_TY_FN:                                                           \
  case PT_ALL_TY_TUP:                                                          \
  case PT_ALL_TY_UNIT:                                                         \
  case PT_ALL_TY_PARAM_NAME:                                                   \
  case PT_ALL_TY_CONSTRUCTOR_NAME
