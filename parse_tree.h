#pragma once

#include <stdio.h>

#include "consts.h"
#include "parser.h"
#include "token.h"
#include "vec.h"

#define NODE_IND_T BUF_IND_T

typedef enum {
  SUBS_EXTERNAL = -1,
  SUBS_NONE = 0,
  SUBS_ONE = 1,
  SUBS_TWO = 2,
} SUBS_TYPE;

typedef enum {
  PT_CALL,
  PT_CONSTRUCTION,
  // Overloaded for type and lambda expression
  PT_FN,
  PT_FUN,
  PT_FUN_BODY,
  PT_IF,
  PT_INT,
  PT_LIST,
  PT_LOWER_NAME,
  PT_ROOT,
  PT_STRING,
  PT_TUP,
  PT_AS,
  PT_UNIT,
  PT_UPPER_NAME,
  PT_SIG,
} parse_node_type;

#define PT_FUN_BINDING_IND(inds, node) inds[node.subs_start]
#define PT_FUN_PARAM_IND(inds, node, i) inds[node.subs_start + i + 1]
#define PT_FUN_BODY_IND(inds, node) inds[node.subs_start + 1 + PT_FUN_PARAM_AMT(node)]

// lambda expression
#define PT_FN_PARAM_IND(node) node.sub_a
#define PT_FN_BODY_IND(node) node.sub_b

#define PT_FN_TYPE_SUB_AMT 2
#define PT_FN_TYPE_PARAM_IND(node) node.sub_a
#define PT_FN_TYPE_RETURN_IND(node) node.sub_b

#define PT_CALL_CALLEE_IND(inds, node) inds[node.subs_start]
#define PT_CALL_PARAM_AMT(node, i) node.sub_amt - 1
#define PT_CALL_PARAM_IND(inds, node, i) inds[node.subs_start + 1 + i]

#define PT_FUN_BODY_STMT_AMT(inds, node) node.sub_amt
#define PT_FUN_BODY_STMT_ind(inds, node, i) inds[node.subs_start + i]

#define PT_IF_GET_COND(inds, node) inds[node.subs_start]
#define PT_IF_GET_A(inds, node) inds[node.subs_start + 1]
#define PT_IF_GET_B(inds, node) inds[node.subs_start + 2]

// PT_INT

#define PT_LIST_SUB_AMT(node) node.sub_amt
#define PT_LIST_SUB_IND(inds, node, i) inds[node.subs_start + i]

// PT_LOWER_NAME

#define PT_ROOT_SUB_AMT(node) node.sub_amt
#define PT_ROOT_SUB_IND(inds, node, i) inds[node.subs_start + i]

// PT_STRING

#define PT_SIG_BINDING_IND(node) node.sub_a
#define PT_SIG_TYPE_IND(node) node.sub_b

extern const char *const parse_node_strings[];

typedef struct {
  parse_node_type type;

  BUF_IND_T start;
  BUF_IND_T end;

  union {
    struct {
      NODE_IND_T subs_start;
      NODE_IND_T sub_amt;
    };
    struct {
      NODE_IND_T sub_a;
      NODE_IND_T sub_b;
    };
  };
} parse_node;

typedef NODE_IND_T node_ind;

VEC_DECL(parse_node);
VEC_DECL(node_ind);
VEC_DECL(token_type);

typedef struct {
  parse_node *nodes;
  NODE_IND_T *inds;
  NODE_IND_T root_ind;
  NODE_IND_T node_amt;
} parse_tree;

typedef struct {
  bool succeeded;

  // TODO restore union when I figure out how to disable lemon's error recovery
  // union {
    parse_tree tree;
    struct {
      NODE_IND_T error_pos;
      uint8_t expected_amt;
      token_type *expected;
    };
  // };
} parse_tree_res;

parse_tree_res parse(token *tokens, size_t token_amt);

void print_parse_tree(FILE *f, source_file file, parse_tree t);
SUBS_TYPE subs_type(parse_node_type type);
