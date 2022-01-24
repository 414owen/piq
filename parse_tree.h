#pragma once

#include <stdio.h>

#include "consts.h"
#include "parser.h"
#include "token.h"
#include "vec.h"

#define NODE_IND_T BUF_IND_T

typedef enum __attribute__ ((__packed__)) {
  PT_COMPOUND,
  PT_NAME,
  PT_NUM,
  PT_IF,
} parse_node_type;

typedef struct {
  parse_node_type type;
  union {
    // parens
    struct {
      NODE_IND_T subs_start;
      uint16_t sub_amt;
    };

    // name, num
    struct {
      BUF_IND_T start;
      BUF_IND_T end;
    };
  };
} parse_node;

typedef NODE_IND_T node_ind;

VEC_DECL(parse_node);
VEC_DECL(node_ind);

typedef struct {
  source_file file;
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

void print_parse_tree(FILE *f, parse_tree t);
