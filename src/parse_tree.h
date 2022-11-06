#pragma once

#include <stdio.h>

#include "ast_meta.h"
#include "consts.h"
#include "parser.h"
#include "span.h"
#include "token.h"
#include "vec.h"

#define node_ind_t buf_ind_t

// TODO `type` is way too overloaded here.
// Maybe we should use `tag`?

// When editing, make sure to add the category in
// parse_tree.c.
typedef enum {
  PT_ALL_EX_CALL = 0,
  PT_ALL_EX_FN = 1,
  PT_ALL_EX_FUN_BODY = 2,
  PT_ALL_EX_IF = 3,
  PT_ALL_EX_INT = 4,
  PT_ALL_EX_LIST = 5,
  PT_ALL_EX_LOWER_NAME = 6,
  PT_ALL_EX_STRING = 7,
  PT_ALL_EX_TUP = 8,
  PT_ALL_EX_AS = 9,
  PT_ALL_EX_UNIT = 10,
  PT_ALL_EX_UPPER_NAME = 11,

  PT_ALL_PAT_WILDCARD = 12,
  PT_ALL_PAT_TUP = 13,
  PT_ALL_PAT_UNIT = 14,
  PT_ALL_PAT_CONSTRUCTION = 15,
  PT_ALL_PAT_STRING = 16,
  PT_ALL_PAT_INT = 17,
  PT_ALL_PAT_LIST = 18,
  PT_ALL_PAT_UPPER_NAME = 19,

  PT_ALL_STMT_SIG = 20,
  PT_ALL_STMT_FUN = 21,
  PT_ALL_STMT_LET = 22,

  PT_ALL_TY_CONSTRUCTION = 23,
  PT_ALL_TY_LIST = 24,
  PT_ALL_TY_FN = 25,
  PT_ALL_TY_TUP = 26,
  PT_ALL_TY_UPPER_NAME = 27,
  PT_ALL_TY_UNIT = 28,

  PT_ALL_TL_SIG = 29,
  PT_ALL_TL_FUN = 30,
} parse_node_type_all;

typedef enum {
  PT_C_EXPRESSION,
  PT_C_PATTERN,
  PT_C_STATEMENT,
  PT_C_TYPE,
  PT_C_TOPLEVEL,
} parse_node_category;

typedef enum {
  PT_EX_CALL = 0,
  PT_EX_FN = 1,
  PT_EX_FUN_BODY = 2,
  PT_EX_IF = 3,
  PT_EX_INT = 4,
  PT_EX_LIST = 5,
  PT_EX_LOWER_NAME = 6,
  PT_EX_STRING = 7,
  PT_EX_TUP = 8,
  PT_EX_AS = 9,
  PT_EX_UNIT = 10,
  PT_EX_UPPER_NAME = 11,
} parse_node_expr_type;

typedef enum {
  PT_PAT_WILDCARD = 12,
  PT_PAT_TUP = 13,
  PT_PAT_UNIT = 14,
  PT_PAT_CONSTRUCTION = 15,
  PT_PAT_STRING = 16,
  PT_PAT_INT = 17,
  PT_PAT_LIST = 18,
  PT_PAT_UPPER_NAME = 19,
} parse_node_pattern_type;

typedef enum {
  PT_STMT_SIG = 20,
  PT_STMT_FUN = 21,
  PT_STMT_LET = 22,
} parse_node_statement_type;

typedef enum {
  PT_TY_CONSTRUCTION = 23,
  PT_TY_LIST = 24,
  PT_TY_FN = 25,
  PT_TY_TUP = 26,
  PT_TY_UPPER_NAME = 27,
  PT_TY_UNIT = 28,
} parse_node_type_type;

typedef enum {
  PT_TL_SIG = 29,
  PT_TL_FUN = 30,
  // TODO let
} parse_node_toplevel_type;

typedef union {
  parse_node_type_all all;
  parse_node_toplevel_type top_level;
  parse_node_statement_type statement;
  parse_node_type_type type;
  parse_node_expr_type expression;
  parse_node_pattern_type pattern;
} parse_node_type;

#define PT_FUN_BINDING_IND(inds, node) inds[node.subs_start + 0]
// TODO use twine here
#define PT_FUN_PARAM_IND(inds, node) inds[node.subs_start + 1]
#define PT_FUN_BODY_IND(inds, node) inds[node.subs_start + 2]

// lambda expression
#define PT_FN_PARAM_IND(node) node.sub_a
#define PT_FN_BODY_IND(node) node.sub_b

#define PT_FN_TYPE_SUB_AMT 2
#define PT_FN_TYPE_PARAM_IND(node) node.sub_a
#define PT_FN_TYPE_RETURN_IND(node) node.sub_b

#define PT_CALL_CALLEE_IND(node) node.sub_a
#define PT_CALL_PARAM_IND(node) node.sub_b

#define PT_IF_COND_IND(inds, node) inds[node.subs_start]
#define PT_IF_A_IND(inds, node) inds[node.subs_start + 1]
#define PT_IF_B_IND(inds, node) inds[node.subs_start + 2]

#define PT_LIST_SUB_AMT(node) node.sub_amt
#define PT_LIST_SUB_IND(inds, node, i) inds[node.subs_start + i]

#define PT_LIST_TYPE_SUB(node) node.sub_a

#define PT_BLOCK_SUB_AMT(node) node.sub_amt
#define PT_BLOCK_SUB_IND(inds, node, i) inds[node.subs_start + i]

#define PT_ROOT_SUB_AMT(node) PT_BLOCK_SUB_AMT(node)
#define PT_ROOT_SUB_IND(inds, node, i) PT_BLOCK_SUB_IND(inds, node, i)

#define PT_FUN_BODY_SUB_AMT(node) PT_BLOCK_SUB_AMT(node)
#define PT_FUN_BODY_SUB_IND(inds, node, i) PT_BLOCK_SUB_IND(inds, node, i)

#define PT_SIG_BINDING_IND(node) node.sub_a
#define PT_SIG_TYPE_IND(node) node.sub_b

#define PT_TUP_SUB_AMT(node) 2
#define PT_TUP_SUB_A(node) node.sub_a
#define PT_TUP_SUB_B(node) node.sub_b

#define PT_LET_BND_IND(node) node.sub_a
#define PT_LET_VAL_IND(node) node.sub_b

#define PT_AS_TYPE_IND(node) node.sub_a
#define PT_AS_VAL_IND(node) node.sub_b

const char *parse_node_string(parse_node_type type);

typedef struct {
  parse_node_type type;

  span span;

  union {
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
VEC_DECL_CUSTOM(node_ind_t, vec_node_ind);
VEC_DECL(token_type);

typedef struct {
  const parse_node *nodes;
  const node_ind_t *inds;
  node_ind_t root_subs_start;
  node_ind_t root_subs_amt;
  node_ind_t node_amt;
} parse_tree;

typedef struct {
  // TODO restore union when I figure out how to disable lemon's error recovery
  // union {
  parse_tree tree;
  // struct {
  token_type *expected;
  node_ind_t error_pos;
  uint8_t expected_amt;
  // };
  bool succeeded;
  // };
} parse_tree_res;

parse_tree_res parse(token *tokens, size_t token_amt);

void print_parse_tree(FILE *f, const char *input, const parse_tree tree);
char *print_parse_tree_str(const char *input, const parse_tree tree);
// tree_node_repr subs_type(parse_node_type type);
void free_parse_tree(parse_tree tree);
void free_parse_tree_res(parse_tree_res res);
