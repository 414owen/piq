// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <hedley.h>
#include "parse_tree.h"

VEC_DECL_CUSTOM(node_ind_t *, vec_ind_ptr);

typedef struct {
  // input node indices to process
  // upper bound on length: amount of nodes
  node_ind_t *input_stack;

  // output tree data
  parse_node *out_nodes;
  node_ind_t *out_inds;

  // where to store the node's index
  node_ind_t **out_stack;

  // also tracks out_stack
  node_ind_t input_stack_len;
  node_ind_t out_node_ind;
  node_ind_t out_ind_ind;
} reorder_state;

static void push_external_subs(parse_tree *in, reorder_state *state,
                               node_ind_t start, node_ind_t amt) {
  node_ind_t *inds = in->inds;
  for (node_ind_t i = 0; i < amt; i++) {
    node_ind_t inds_ind_offset = amt - 1 - i;
    node_ind_t inds_ind = start + inds_ind_offset;

    state->input_stack[state->input_stack_len] = inds[inds_ind];
    state->out_stack[state->input_stack_len] =
      &state->out_inds[state->out_ind_ind + inds_ind_offset];
    state->input_stack_len++;
  }
  state->out_ind_ind += amt;
}

// consumes previous tree.
parse_tree reorder_tree(parse_tree in) {

  reorder_state state = {
    .input_stack = malloc(in.node_amt * sizeof(node_ind_t)),
    .input_stack_len = 0,

    // this is calloced so that nodes with uninitialized data
    // (eg zero sub-els) can be easily compared
    .out_nodes = calloc(in.node_amt, sizeof(parse_node)),
    .out_node_ind = 0,
    .out_ind_ind = 0,
    .out_inds = malloc(in.ind_amt * sizeof(node_ind_t)),

    .out_stack = malloc(in.node_amt * sizeof(node_ind_t *)),
  };

  // seed the stack
  push_external_subs(&in, &state, in.root_subs_start, in.root_subs_amt);

  while (state.input_stack_len > 0) {
    state.input_stack_len--;
    node_ind_t ind = state.input_stack[state.input_stack_len];
    {
      // "return" the current node index
      node_ind_t *out_ind_p = state.out_stack[state.input_stack_len];
      *out_ind_p = state.out_node_ind;
    }
    parse_node node = in.nodes[ind];

    parse_node *out_node_p = &state.out_nodes[state.out_node_ind++];
    *out_node_p = node;

    switch (pt_subs_type[node.type.all]) {
      case SUBS_NONE:
        break;
      case SUBS_TWO:
        state.input_stack[state.input_stack_len] = node.data.two_subs.b;
        state.out_stack[state.input_stack_len] = &out_node_p->data.two_subs.b;
        state.input_stack_len++;
        state.input_stack[state.input_stack_len] = node.data.two_subs.a;
        state.out_stack[state.input_stack_len] = &out_node_p->data.two_subs.a;
        state.input_stack_len++;
        break;
      case SUBS_ONE:
        state.input_stack[state.input_stack_len] = node.data.one_sub.ind;
        state.out_stack[state.input_stack_len] = &out_node_p->data.one_sub.ind;
        state.input_stack_len++;
        break;
      case SUBS_EXTERNAL:
        out_node_p->data.more_subs.start = state.out_ind_ind;
        push_external_subs(
          &in, &state, node.data.more_subs.start, node.data.more_subs.amt);
        break;
    }
  }

  free(in.nodes);
  free(in.inds);
  free(state.out_stack);
  free(state.input_stack);

  parse_tree res = in;

  assert(state.out_ind_ind == in.ind_amt);
  assert(state.out_node_ind == in.node_amt);

  res.root_subs_start = 0;
  res.nodes = state.out_nodes;
  res.inds = state.out_inds;
  return res;
}
