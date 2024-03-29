// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "parse_tree.h"
#include "vec.h"

// If traversing the AST ever becomes a bottleneck (unlikely)
// we could consider specializing the traversal to these cases
typedef enum {

  // - [x] traverse patterns on the way in
  // - [x] traverse patterns on the way out
  // - [x] traverse expressions on the way in
  // - [x] traverse expressions on the way out
  // - [ ] push scope
  // - [ ] pop scope
  // - [ ] link signatures
  // - [ ] block operations
  TRAVERSE_PRINT_TREE = 0,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse expressions on the way in
  // - [ ] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [ ] link signatures
  // - [ ] block operations
  TRAVERSE_RESOLVE_BINDINGS = 1,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse expressions on the way in
  // - [ ] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [x] link signatures
  // - [ ] block operations
  TRAVERSE_TYPECHECK = 2,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse expressions on the way in (eg FN)
  // - [x] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [ ] link signatures
  // - [x] block operations
  TRAVERSE_CODEGEN = 3,

} traverse_mode;

typedef enum {
  TR_ACT_PREDECLARE_FN,
  TR_ACT_PUSH_SCOPE_VAR,
  TR_ACT_NEW_BLOCK,
  TR_ACT_VISIT_IN,
  TR_ACT_VISIT_OUT,
  TR_ACT_POP_TO,
  TR_ACT_END,
  TR_ACT_ANNOTATE,

  // First time we see a node
  TR_ACT_INITIAL,
  // When we enter a new block
  TR_ACT_BACKUP_SCOPE,
} traverse_action_internal;

typedef enum {
  TR_PREDECLARE_FN = TR_ACT_PREDECLARE_FN,
  TR_PUSH_SCOPE_VAR = TR_ACT_PUSH_SCOPE_VAR,
  TR_NEW_BLOCK = TR_ACT_NEW_BLOCK,
  TR_VISIT_IN = TR_ACT_VISIT_IN,
  TR_VISIT_OUT = TR_ACT_VISIT_OUT,
  TR_POP_TO = TR_ACT_POP_TO,
  TR_END = TR_ACT_END,
  TR_ANNOTATE = TR_ACT_ANNOTATE,
} traverse_action;

typedef union {
  traverse_action action;
  traverse_action_internal action_internal;
} traverse_action_union;

VEC_DECL_CUSTOM(traverse_action_internal, vec_traverse_action);
// VEC_DECL_CUSTOM(traverse_action, vec_traverse_action);

typedef struct {
  uint8_t traverse_patterns_in : 1;
  uint8_t traverse_patterns_out : 1;
  uint8_t traverse_expressions_in : 1;
  uint8_t traverse_expressions_out : 1;
  uint8_t edit_environment : 1;
  uint8_t annotate : 1;
  uint8_t add_blocks : 1;
} traversal_wanted_actions;

typedef struct {
  const parse_node *restrict nodes;
  const node_ind_t *restrict inds;
  vec_traverse_action actions;
  environment_ind_t environment_amt;
  vec_environment_ind environment_len_stack;
  // two names for the same stack
  vec_node_ind node_stack;
  traverse_mode mode;
  traversal_wanted_actions wanted_actions;
} pt_traversal;

typedef struct {
  node_ind_t node_index;
  parse_node node;
} traversal_node_data;

typedef struct {
  node_ind_t annotation_index;
  node_ind_t target_index;
} traversal_annotate_data;

typedef union {
  traversal_node_data node_data;
  traversal_annotate_data annotation_data;
  node_ind_t new_environment_amount;
} pt_trav_elem_data;

typedef struct {
  traverse_action action;
  pt_trav_elem_data data;
} pt_traverse_elem;
