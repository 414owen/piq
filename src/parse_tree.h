#pragma once

#include <stdio.h>

#include "ast_meta.h"
#include "consts.h"
#include "parser.h"
#include "token.h"
#include "vec.h"

#define node_ind_t buf_ind_t

typedef enum {
  PT_CALL,
  PT_CONSTRUCTION,
  // Overloaded for type and lambda expression
  PT_FN,
  PT_FN_TYPE,
  PT_FUN,
  PT_FUN_BODY,
  PT_IF,
  PT_INT,
  PT_LIST,
  PT_LIST_TYPE,
  PT_LOWER_NAME,
  PT_ROOT,
  PT_STRING,
  PT_TUP,
  PT_AS,
  PT_UNIT,
  PT_UPPER_NAME,
  PT_SIG,
  PT_LET,
} parse_node_type;

#define PT_FUN_BINDING_IND(inds, node) inds[node.subs_start + 0]
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

  buf_ind_t start;
  buf_ind_t end;

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
  parse_node *nodes;
  node_ind_t *inds;
  node_ind_t root_ind;
  node_ind_t node_amt;
} parse_tree;

typedef struct {
  bool succeeded;

  // TODO restore union when I figure out how to disable lemon's error recovery
  // union {
  parse_tree tree;
  struct {
    node_ind_t error_pos;
    uint8_t expected_amt;
    token_type *expected;
  };
  // };
} parse_tree_res;

parse_tree_res parse(token *tokens, size_t token_amt);

void print_parse_tree(FILE *f, char *input, parse_tree t);
char *print_parse_tree_str(char *input, parse_tree tree);
tree_node_repr subs_type(parse_node_type type);
void free_parse_tree_res(parse_tree_res res);
