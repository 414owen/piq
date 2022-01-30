#pragma once

#include <stdio.h>

#include "consts.h"
#include "parser.h"
#include "token.h"
#include "vec.h"

#define NODE_IND_T BUF_IND_T

typedef enum __attribute__ ((__packed__)) {
  PT_CALL,
  PT_NAME,
  PT_INT,
  PT_IF,
  PT_TUP,
  PT_ROOT,
  PT_FN,
} parse_node_type;

typedef struct {
  parse_node_type type;

  BUF_IND_T start;
  BUF_IND_T end;

  NODE_IND_T subs_start;
  uint16_t sub_amt;
} parse_node;

typedef NODE_IND_T node_ind;

VEC_DECL(parse_node);
VEC_DECL(node_ind);
VEC_DECL(token_type);

typedef struct {
  NODE_IND_T root_ind;
  vec_parse_node nodes;
  vec_node_ind inds;
} parse_tree;

typedef struct {
  bool succeeded;
  union {
    parse_tree tree;
    struct {
      NODE_IND_T error_pos;
      uint8_t expected_amt;
      token_type *expected;
    };
  };
} parse_tree_res;

parse_tree_res parse(vec_token tokens);

void print_parse_tree(FILE *f, source_file file, parse_tree t);
