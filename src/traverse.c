// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdint.h>

#include "builtins.h"
#include "parse_tree.h"
#include "traverse.h"

// TODO rename push_scope(function) to declare_function

/*

Notes on PUSH_SCOPE:

* For letrec, functions are pushed visited on the way in
* For let, we push the value on the way out.

typedef enum {

  // - [x] traverse patterns on the way in
  // - [x] traverse patterns on the way out
  // - [x] traverse types/expressions on the way in
  // - [x] traverse types/expressions on the way out
  // - [ ] push scope
  // - [ ] pop scope
  // - [ ] link signatures
  // - [ ] block operations
  TRAVERSE_PRINT_TREE = 0,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse types/expressions on the way in
  // - [ ] traverse types/expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [ ] link signatures
  // - [ ] block operations
  TRAVERSE_RESOLVE_BINDINGS = 1,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse types/expressions on the way in
  // - [ ] traverse types/expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [x] link signatures
  // - [ ] block operations
  TRAVERSE_TYPECHECK = 2,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [ ] traverse types/expressions on the way in
  // - [x] traverse types/expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [ ] link signatures
  // - [x] block operations
  TRAVERSE_CODEGEN = 3,

} traverse_mode;
*/

static uint8_t traverse_patterns_in =
  (1 << TRAVERSE_PRINT_TREE) | (1 << TRAVERSE_RESOLVE_BINDINGS) |
  (1 << TRAVERSE_TYPECHECK) | (1 << TRAVERSE_CODEGEN);

static uint8_t traverse_patterns_out = (1 << TRAVERSE_PRINT_TREE);

static uint8_t traverse_expressions_in = (1 << TRAVERSE_PRINT_TREE) |
                                         (1 << TRAVERSE_RESOLVE_BINDINGS) |
                                         (1 << TRAVERSE_TYPECHECK);

static uint8_t traverse_expressions_out =
  (1 << TRAVERSE_PRINT_TREE) | (1 << TRAVERSE_CODEGEN);

static uint8_t should_edit_environment = (1 << TRAVERSE_RESOLVE_BINDINGS) |
                                         (1 << TRAVERSE_TYPECHECK) |
                                         (1 << TRAVERSE_CODEGEN);

static uint8_t should_link_sigs = (1 << TRAVERSE_TYPECHECK);

static uint8_t should_add_blocks = (1 << TRAVERSE_CODEGEN);

static void tr_push_action(pt_traversal *traversal,
                           traverse_action_internal action,
                           node_ind_t node_index) {
  VEC_PUSH(&traversal->actions, action);
  VEC_PUSH(&traversal->node_stack, node_index);
}

static void tr_push_initial(pt_traversal *traversal, node_ind_t node_index) {
  tr_push_action(traversal, TR_ACT_INITIAL, node_index);
}

static void tr_maybe_add_block(pt_traversal *traversal) {
  if (traversal->wanted_actions.add_blocks) {
    traverse_action_internal action = TR_ACT_NEW_BLOCK;
    VEC_PUSH(&traversal->actions, action);
  }
}

static void tr_push_in(pt_traversal *traversal, node_ind_t node_index) {
  tr_push_action(traversal, TR_ACT_VISIT_IN, node_index);
}

static void tr_push_out(pt_traversal *traversal, node_ind_t node_index) {
  tr_push_action(traversal, TR_ACT_VISIT_OUT, node_index);
}

static bool test_should(uint8_t t, traverse_mode mode) {
  return t & (1 << mode);
}

static void traverse_push_block(pt_traversal *traversal, node_ind_t start,
                                node_ind_t amount) {
  for (node_ind_t i = 0; i < amount; i++) {
    const node_ind_t node_index = traversal->inds[start + amount - 1 - i];
    tr_push_initial(traversal, node_index);
  }
}

static traversal_node_data traverse_get_parse_node(pt_traversal *traversal) {
  traversal_node_data res;
  VEC_POP(&traversal->node_stack, &res.node_index);
  res.node = traversal->nodes[res.node_index];
  return res;
}

static void tr_push_subs_external(pt_traversal *traversal, parse_node node) {
  VEC_APPEND_REVERSE(&traversal->node_stack,
                     node.data.more_subs.amt,
                     &traversal->inds[node.data.more_subs.start]);
  const traverse_action_internal act = TR_ACT_INITIAL;
  VEC_REPLICATE(&traversal->actions, node.data.more_subs.amt, act);
}

static void tr_maybe_link_sig(pt_traversal *traversal, node_ind_t node_index) {
  if (traversal->wanted_actions.link_sigs) {
    const traverse_action act = TR_LINK_SIG;
    VEC_PUSH(&traversal->actions, act);
    const vec_node_ind stack = traversal->node_stack;
    node_ind_t target = VEC_GET(stack, stack.len - 1);
    VEC_PUSH(&traversal->node_stack, target);
    VEC_PUSH(&traversal->node_stack, node_index);
  }
}

// To be used with PUSH_SCOPE or PREDECLARE_FN
static void tr_maybe_push_environment(pt_traversal *traversal,
                                      node_ind_t node_index,
                                      traverse_action_internal act) {
  if (traversal->wanted_actions.edit_environment) {
    VEC_PUSH(&traversal->actions, act);
    VEC_PUSH(&traversal->node_stack, node_index);
  }
}

static void tr_push_subs_two(pt_traversal *traversal, node_ind_t a,
                             node_ind_t b) {
  tr_push_initial(traversal, b);
  tr_push_initial(traversal, a);
}

static void tr_push_subs(pt_traversal *traversal, parse_node node) {
  const tree_node_repr repr = pt_subs_type[node.type.all];

  switch (repr) {
    case SUBS_EXTERNAL:
      tr_push_subs_external(traversal, node);
      break;
    case SUBS_NONE:
      break;
    case SUBS_TWO:
      tr_push_subs_two(traversal, node.data.two_subs.a, node.data.two_subs.b);
      break;
    case SUBS_ONE:
      tr_push_initial(traversal, node.data.two_subs.a);
      break;
  }
}

/*
static void tr_push_subs_one(pt_traversal *traversal, node_ind_t a) {
  tr_push_action(traversal, TR_ACT_INITIAL, a);
}
*/

static void tr_maybe_push_pattern_in(pt_traversal *traversal,
                                     node_ind_t node_index) {
  if (traversal->wanted_actions.traverse_patterns_in) {
    tr_push_in(traversal, node_index);
  }
}

static void tr_maybe_push_pattern_out(pt_traversal *traversal,
                                      node_ind_t node_index) {
  if (traversal->wanted_actions.traverse_patterns_out) {
    tr_push_out(traversal, node_index);
  }
}

static void tr_maybe_push_expression_in(pt_traversal *traversal,
                                        node_ind_t node_index) {
  if (traversal->wanted_actions.traverse_expressions_in) {
    tr_push_in(traversal, node_index);
  }
}

static void tr_maybe_push_expression_out(pt_traversal *traversal,
                                         node_ind_t node_index) {
  if (traversal->wanted_actions.traverse_expressions_out) {
    tr_push_out(traversal, node_index);
  }
}

static void tr_initial_pattern(pt_traversal *traversal,
                               traversal_node_data data) {
  tr_maybe_push_pattern_out(traversal, data.node_index);
  tr_push_subs(traversal, data.node);
  tr_maybe_push_pattern_in(traversal, data.node_index);
}

/*
static void tr_initial_two_sub_expression(pt_traversal *traversal,
traversal_node_data data) { tr_maybe_push_expression_out(traversal,
data.node_index); tr_push_subs_two(traversal, data.node.sub_a, data.node.sub_b);
  tr_maybe_push_expression_in(traversal, data.node_index);
}
*/

static void tr_initial_external_sub_expression(pt_traversal *traversal,
                                               traversal_node_data data) {
  tr_maybe_push_expression_out(traversal, data.node_index);
  tr_push_subs_external(traversal, data.node);
  tr_maybe_push_expression_in(traversal, data.node_index);
}

static void tr_initial_expression(pt_traversal *traversal,
                                  traversal_node_data data) {
  tr_maybe_push_expression_out(traversal, data.node_index);
  tr_push_subs(traversal, data.node);
  tr_maybe_push_expression_in(traversal, data.node_index);
}

static void tr_initial_generic(pt_traversal *traversal,
                               traversal_node_data data) {
  tr_push_out(traversal, data.node_index);
  tr_push_subs(traversal, data.node);
  tr_push_in(traversal, data.node_index);
}

static void tr_initial_two_sub_statement(pt_traversal *traversal,
                                         traversal_node_data data) {
  tr_push_out(traversal, data.node_index);
  tr_push_subs_two(
    traversal, data.node.data.two_subs.a, data.node.data.two_subs.b);
  tr_push_in(traversal, data.node_index);
}

static void tr_initial_external_sub_statement(pt_traversal *traversal,
                                              traversal_node_data data) {
  tr_initial_generic(traversal, data);
}

/*
static void tr_push_inout(pt_traversal *traversal, node_ind_t node_index) {
  tr_push_out(traversal, node_index);
  tr_push_in(traversal, node_index);
}
*/

static void tr_maybe_restore_scope(pt_traversal *traversal) {
  if (traversal->wanted_actions.edit_environment) {
    const traverse_action_internal act = TR_ACT_POP_TO;
    VEC_PUSH(&traversal->actions, act);
  }
}

static void tr_maybe_backup_scope(pt_traversal *traversal) {
  if (traversal->wanted_actions.edit_environment) {
    const traverse_action_internal act = TR_ACT_BACKUP_SCOPE;
    VEC_PUSH(&traversal->actions, act);
  }
}

// if in a letrec, push scope has to be called before `in` for obvious reasons
// traverse in scope order meaning that in letrecs, we touch the roots first
// then the children
static void pt_traverse_push_letrec(pt_traversal *traversal, node_ind_t start,
                                    node_ind_t amount) {
  for (node_ind_t i = 0; i < amount; i++) {
    const node_ind_t node_index = traversal->inds[start + amount - 1 - i];
    const traversal_node_data data = {
      .node_index = node_index,
      .node = traversal->nodes[node_index],
    };
    switch (data.node.type.statement) {
      case PT_STATEMENT_FUN: {
        tr_maybe_restore_scope(traversal);
        tr_initial_external_sub_statement(traversal, data);
        tr_maybe_backup_scope(traversal);
        break;
      }
      default:
        tr_push_initial(traversal, node_index);
        break;
    }
  }

  for (node_ind_t i = 0; i < amount; i++) {
    const node_ind_t node_index = traversal->inds[start + i];
    const parse_node node = traversal->nodes[node_index];
    if (node.type.statement == PT_STATEMENT_FUN) {
      tr_maybe_push_environment(traversal, node_index, TR_ACT_PREDECLARE_FN);
    }
  }
}

static void tr_handle_initial(pt_traversal *traversal) {
  traversal_node_data data = traverse_get_parse_node(traversal);

  switch (data.node.type.all) {
    case PT_ALL_LEN:
      // impossible
      break;

    case PT_ALL_EX_FN:
      debug_assert(pt_subs_type[PT_ALL_EX_FN] == SUBS_EXTERNAL);
      tr_maybe_restore_scope(traversal);
      tr_initial_external_sub_expression(traversal, data);
      tr_maybe_backup_scope(traversal);
      break;
    case PT_ALL_EX_IF: {
      debug_assert(pt_subs_type[data.node.type.all] == SUBS_EXTERNAL);
      tr_maybe_push_expression_out(traversal, data.node_index);
      const node_ind_t cond_ind = PT_IF_COND_IND(traversal->inds, data.node);
      const node_ind_t then_ind = PT_IF_A_IND(traversal->inds, data.node);
      const node_ind_t else_ind = PT_IF_B_IND(traversal->inds, data.node);
      tr_push_initial(traversal, else_ind);
      tr_maybe_add_block(traversal);
      tr_push_initial(traversal, then_ind);
      tr_maybe_add_block(traversal);
      tr_push_initial(traversal, cond_ind);
      tr_maybe_push_expression_in(traversal, data.node_index);
      break;
    }
    case PT_ALL_EX_CALL:
    case PT_ALL_EX_FUN_BODY:
    case PT_ALL_EX_INT:
    case PT_ALL_EX_AS:
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_EX_UPPER_NAME:
    case PT_ALL_EX_LIST:
    case PT_ALL_EX_STRING:
    case PT_ALL_EX_TUP:
    case PT_ALL_EX_UNIT:
      tr_initial_expression(traversal, data);
      break;

    case PT_ALL_MULTI_TERM_NAME:
    case PT_ALL_MULTI_TYPE_PARAMS:
    case PT_ALL_MULTI_TYPE_PARAM_NAME:
    case PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
    case PT_ALL_MULTI_DATA_CONSTRUCTORS:
      tr_initial_generic(traversal, data);
      break;

    case PT_ALL_PAT_WILDCARD:
      debug_assert(pt_subs_type[PT_ALL_PAT_WILDCARD] == SUBS_NONE);
      tr_maybe_push_environment(
        traversal, data.node_index, TR_ACT_PUSH_SCOPE_VAR);
      HEDLEY_FALL_THROUGH;
    case PT_ALL_PAT_TUP:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_PAT_DATA_CONSTRUCTOR_NAME:
    case PT_ALL_PAT_CONSTRUCTION:
    case PT_ALL_PAT_STRING:
    case PT_ALL_PAT_INT:
    case PT_ALL_PAT_LIST:
      tr_initial_pattern(traversal, data);
      break;

    case PT_ALL_STATEMENT_SIG:
      debug_assert(pt_subs_type[PT_ALL_STATEMENT_SIG] == SUBS_TWO);
      tr_maybe_link_sig(traversal, data.node_index);
      tr_initial_two_sub_statement(traversal, data);
      break;
    case PT_ALL_STATEMENT_LET:
      debug_assert(pt_subs_type[PT_ALL_STATEMENT_LET] == SUBS_TWO);
      // push env after the fact
      tr_maybe_push_environment(
        traversal, data.node_index, TR_ACT_PUSH_SCOPE_VAR);
      tr_initial_two_sub_statement(traversal, data);
      break;
    case PT_ALL_STATEMENT_FUN:
      debug_assert(pt_subs_type[PT_ALL_STATEMENT_FUN] == SUBS_EXTERNAL);
      tr_maybe_restore_scope(traversal);
      tr_initial_external_sub_statement(traversal, data);
      tr_maybe_backup_scope(traversal);
      tr_maybe_push_environment(
        traversal, data.node_index, TR_ACT_PREDECLARE_FN);
      break;
    case PT_ALL_STATEMENT_DATA_DECLARATION:
      tr_initial_generic(traversal, data);
      break;

    case PT_ALL_TY_CONSTRUCTION:
    case PT_ALL_TY_LIST:
    case PT_ALL_TY_FN:
    case PT_ALL_TY_TUP:
    case PT_ALL_TY_UNIT:
    case PT_ALL_TY_PARAM_NAME:
    case PT_ALL_TY_CONSTRUCTOR_NAME:
      // This looks bad, but all dependents require types in/out the same as
      // expressions at the moment.
      tr_maybe_push_expression_out(traversal, data.node_index);
      tr_push_subs(traversal, data.node);
      tr_maybe_push_expression_in(traversal, data.node_index);
      break;
  }
}

pt_traversal pt_traverse(parse_tree tree, traverse_mode mode) {
  pt_traversal res = {
    .nodes = tree.nodes,
    .inds = tree.inds,
    .mode = mode,
    .wanted_actions =
      {
        .add_blocks = test_should(should_add_blocks, mode),
        .edit_environment = test_should(should_edit_environment, mode),
        .link_sigs = test_should(should_link_sigs, mode),
        .traverse_expressions_in = test_should(traverse_expressions_in, mode),
        .traverse_expressions_out = test_should(traverse_expressions_out, mode),
        .traverse_patterns_in = test_should(traverse_patterns_in, mode),
        .traverse_patterns_out = test_should(traverse_patterns_out, mode),
      },
    // .path = VEC_NEW,
    .node_stack = VEC_NEW,
    .environment_amt = builtin_term_amount,
  };
  // to represent root
  // VEC_PUSH(&res.path, PT_ALL_LEN);
  const traverse_action act = TR_END;
  VEC_PUSH(&res.actions, act);
  if (res.wanted_actions.edit_environment) {
    pt_traverse_push_letrec(&res, tree.root_subs_start, tree.root_subs_amt);
  } else {
    traverse_push_block(&res, tree.root_subs_start, tree.root_subs_amt);
  }
  return res;
}

// pre-then-postorder traversal
pt_traverse_elem pt_traverse_next(pt_traversal *traversal) {
  pt_traverse_elem res;
  while (traversal->actions.len > 0) {
    traverse_action_internal act;
    VEC_POP(&traversal->actions, &act);
    res.action = (traverse_action)act;

    switch (act) {
      case TR_ACT_BACKUP_SCOPE:
        VEC_PUSH(&traversal->environment_len_stack, traversal->environment_amt);
        break;
      case TR_ACT_PREDECLARE_FN:
      case TR_ACT_PUSH_SCOPE_VAR:
        traversal->environment_amt++;
        HEDLEY_FALL_THROUGH;
      case TR_ACT_VISIT_OUT:
      case TR_ACT_VISIT_IN:
        res.data.node_data = traverse_get_parse_node(traversal);
        return res;
      // This is where we schedule everything.
      // The others just regurgitate themselves
      case TR_ACT_INITIAL:
        tr_handle_initial(traversal);
        break;
      case TR_ACT_POP_TO:
        VEC_POP(&traversal->environment_len_stack,
                &res.data.new_environment_amount);
        traversal->environment_amt = res.data.new_environment_amount;
        return res;
      case TR_ACT_END:
        VEC_FREE(&traversal->actions);
        VEC_FREE(&traversal->environment_len_stack);
        VEC_FREE(&traversal->node_stack);
        HEDLEY_FALL_THROUGH;
      case TR_ACT_NEW_BLOCK:
        return res;
      case TR_ACT_LINK_SIG: {
        node_ind_t node_index;
        node_ind_t target_ind;
        VEC_POP(&traversal->node_stack, &node_index);
        VEC_POP(&traversal->node_stack, &target_ind);
        res.data.link_sig_data.sig_index = node_index;
        res.data.link_sig_data.linked_index = target_ind;
        return res;
      }
    }
  }
  res.action = TR_END;
  return res;
}