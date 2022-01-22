#pragma once

#include "consts.h"
#include "vec.h"

#define NODE_IND_T BUF_IND_T

typedef enum __attribute__ ((__packed__)) {
  PT_PARENS,
  PT_NAME,
  PT_NUM,
} parse_node_type;

// parens
typedef struct {
  uint16_t sub_num;
  NODE_IND_T subs_start;
} paren_node;

// name, num
typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
} leaf_node;

typedef NODE_IND_T node_ind;

VEC_DECL(node_ind);
VEC_DECL(parse_node_type);
VEC_DECL(paren_node);
VEC_DECL(leaf_node);

typedef struct {
  parse_node_type root_type;
  NODE_IND_T root_ind;
  vec_parse_node_type types;
  vec_paren_node parens;
  vec_leaf_node leaves;
  vec_node_ind inds;
} parse_tree;
