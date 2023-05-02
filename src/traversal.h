// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "parse_tree.h"
#include "vec.h"

#define TR_VEC_DECL(type) TR_VEC_DECL_CUSTOM(type, vec_ ## type)

#ifdef PRECALC_MODE

  #define TR_VEC(name) vec_ ## name
  #define TR_VEC_DECL_CUSTOM(type, name) VEC_DECL_CUSTOM(type, name)

#else

#ifdef NDEBUG

  #define TR_VEC_DECL_CUSTOM(type, name) \
    typedef struct { \
      type *data; \
      VEC_LEN_T len; \
    } fixed_ ## name

#else

  #define TR_VEC_DECL_CUSTOM(type, name) \
    typedef struct { \
      type *data; \
      VEC_LEN_T len; \
      VEC_LEN_T cap; \
    } fixed_ ## name

#endif

  #define TR_VEC(vec_name) fixed_vec_ ## vec_name

  TR_VEC_DECL_CUSTOM(node_ind_t, vec_node_ind);

#endif

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

  // - [x] traverse patterns on the way in
  // - [x] traverse patterns on the way out
  // - [x] traverse expressions on the way in (eg FN)
  // - [x] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [x] link signatures
  // - [x] block operations
  TRAVERSE_PRECALCULATE = 4,

} traverse_mode;

typedef enum {
  TR_ACT_PREDECLARE_FN,
  TR_ACT_PUSH_SCOPE_VAR,
  TR_ACT_NEW_BLOCK,
  TR_ACT_VISIT_IN,
  TR_ACT_VISIT_OUT,
  TR_ACT_POP_SCOPE_TO,
  TR_ACT_END,
  TR_ACT_LINK_SIG,

  // First time we see a node
  TR_ACT_INITIAL,
  // When we enter a new block
  TR_ACT_BACKUP_SCOPE,
} traverse_action_internal;

TR_VEC_DECL_CUSTOM(traverse_action_internal, vec_traverse_action);

typedef enum {
  TR_PREDECLARE_FN = TR_ACT_PREDECLARE_FN,
  TR_PUSH_SCOPE_VAR = TR_ACT_PUSH_SCOPE_VAR,
  TR_NEW_BLOCK = TR_ACT_NEW_BLOCK,
  TR_VISIT_IN = TR_ACT_VISIT_IN,
  TR_VISIT_OUT = TR_ACT_VISIT_OUT,
  TR_POP_TO = TR_ACT_POP_SCOPE_TO,
  TR_END = TR_ACT_END,
  TR_LINK_SIG = TR_ACT_LINK_SIG,
} traverse_action;

typedef union {
  traverse_action action;
  traverse_action_internal action_internal;
} traverse_action_union;

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
  TR_VEC(traverse_action) actions;
  TR_VEC(node_ind) environment_len_stack;
  TR_VEC(node_ind) node_stack;
  node_ind_t environment_amt;
  traverse_mode mode;
  #ifndef PRECALC_MODE
    traversal_wanted_actions wanted_actions;
  #endif
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
  traverse_action action;
  pt_trav_elem_data data;
} pt_traverse_elem;

typedef union {
  node_ind_t node_index;
  traversal_link_sig_data link_sig_data;
  node_ind_t new_environment_amount;
} pt_minimal_trav_elem_data;

typedef struct {
  traverse_action action;
  pt_minimal_trav_elem_data data;
} pt_minimal_traverse_elem;

VEC_DECL(pt_minimal_traverse_elem);

typedef struct {
  pt_minimal_traverse_elem *elems;
  uint64_t traverse_action_amt;
} pt_precalculated_traversal;
