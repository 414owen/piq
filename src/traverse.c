#include <stdint.h>

#include "builtins.h"
#include "parse_tree.h"
#include "traverse.h"

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

static uint8_t traverse_patterns_in
  = (1 << TRAVERSE_PRINT_TREE)
  | (1 << TRAVERSE_RESOLVE_BINDINGS)
  | (1 << TRAVERSE_TYPECHECK)
  | (1 << TRAVERSE_CODEGEN);

static uint8_t traverse_patterns_out
  = (1 << TRAVERSE_PRINT_TREE);

static uint8_t traverse_expressions_in
  = (1 << TRAVERSE_PRINT_TREE)
  | (1 << TRAVERSE_RESOLVE_BINDINGS)
  | (1 << TRAVERSE_TYPECHECK);

static uint8_t traverse_expressions_out
  = (1 << TRAVERSE_PRINT_TREE)
  | (1 << TRAVERSE_CODEGEN);

static uint8_t should_edit_environment
  = (1 << TRAVERSE_RESOLVE_BINDINGS)
  | (1 << TRAVERSE_TYPECHECK)
  | (1 << TRAVERSE_CODEGEN);

static uint8_t should_link_sigs
  = (1 << TRAVERSE_TYPECHECK);

static uint8_t should_add_blocks
  = (1 << TRAVERSE_CODEGEN);

static void push_scoped_traverse_action(pt_traversal *traversal,
                                        scoped_traverse_action action,
                                        node_ind_t node_ind) {
  VEC_PUSH(&traversal->actions, action);
  VEC_PUSH(&traversal->node_stack, node_ind);
}

static bool test_should(uint8_t t, traverse_mode mode) {
  return t & (1 << mode);
}

static void traverse_push_block(pt_traversal *traversal, node_ind_t start, node_ind_t amount) {
  for (node_ind_t i = 0; i < amount; i++) {
    const node_ind_t node_ind = traversal->inds[start + amount - 1 - i];
    VEC_PUSH(&traversal->node_stack, node_ind);
    const scoped_traverse_action act = TR_VISIT_IN;
    VEC_PUSH(&traversal->actions, act);
  }
}

// if in a letrec, push scope has to be called before `in` for obvious reasons
// traverse in scope order meaning that in letrecs, we touch the roots first
// then the children
static void pt_scoped_traverse_push_letrec(pt_traversal *traversal,
                                           node_ind_t start,
                                           node_ind_t amount) {
  for (node_ind_t i = 0; i < amount; i++) {
    const node_ind_t node_ind = traversal->inds[start + i];
    const parse_node node = traversal->nodes[node_ind];
    switch (node.type.statement) {
      case PT_STATEMENT_SIG:
        continue;
      case PT_STATEMENT_FUN: {
        const scoped_traverse_action act = TR_PUSH_SCOPE_VAR;
        VEC_PUSH(&traversal->actions, act);
        VEC_PUSH(&traversal->node_stack, node_ind);
        break;
      }
    }
  }
}

pt_traversal pt_scoped_traverse(parse_tree tree, traverse_mode mode) {
  pt_traversal res = {
    .nodes = tree.nodes,
    .inds = tree.inds,
    .mode = mode,
    .wanted_actions = {
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
  const scoped_traverse_action act = TR_END;
  VEC_PUSH(&res.actions, act);
  traverse_push_block(
    &res, tree.root_subs_start, tree.root_subs_amt);
  if (res.wanted_actions.edit_environment) {
    pt_scoped_traverse_push_letrec(
      &res, tree.root_subs_start, tree.root_subs_amt);
  }
  return res;
}

static const uint64_t in_letrec = (((uint64_t) 1) << PT_ALL_LEN);

static traversal_node_data traverse_get_parse_node(pt_traversal *traversal) {
  traversal_node_data res;
  VEC_POP(&traversal->node_stack, &res.node_index);
  res.node = traversal->nodes[res.node_index];
  return res;
}

// these could probably be made faster with lookups...
static bool should_push_out(traversal_wanted_actions wanted_actions, parse_node_type_all type) {
  bool res = true;
  switch (parse_node_categories[type]) {
    case PT_C_TYPE:
    case PT_C_EXPRESSION:
      res = wanted_actions.traverse_expressions_out;
      break;
    case PT_C_PATTERN:
      res = wanted_actions.traverse_patterns_out;
      break;
    default:
      break;
  }
  return res;
}

static bool should_traverse_in(traversal_wanted_actions wanted_actions, parse_node_type_all type) {
  bool res = true;
  switch (parse_node_categories[type]) {
    case PT_C_TYPE:
    case PT_C_EXPRESSION:
      res = wanted_actions.traverse_expressions_in;
      break;
    case PT_C_PATTERN:
      res = wanted_actions.traverse_patterns_in;
      break;
    default:
      break;
  }
  return res;
}

// pre-then-postorder traversal
pt_traverse_elem pt_scoped_traverse_next(pt_traversal *traversal) {
  while (true) {
    scoped_traverse_action action;
    VEC_POP(&traversal->actions, &action);
  
    pt_traverse_elem res = {
      .action = action,
    };
    switch (action) {
      case TR_PUSH_SCOPE_VAR:
        traversal->environment_amt++;
        res.data.node_data = traverse_get_parse_node(traversal);
        return res;
      case TR_VISIT_OUT:
        res.data.node_data = traverse_get_parse_node(traversal);
        return res;
      case TR_VISIT_IN:
        res.data.node_data = traverse_get_parse_node(traversal);
        if (should_push_out(traversal->wanted_actions, res.data.node_data.node.type.all)) {
          scoped_traverse_action act = TR_VISIT_OUT;
          VEC_PUSH(&traversal->actions, act);
          VEC_PUSH(&traversal->node_stack, res.data.node_data.node_index);
        }

        switch (res.data.node_data.node.type.all) {
          case PT_ALL_STATEMENT_SIG: {
            if (traversal->wanted_actions.link_sigs) {
              scoped_traverse_action act = TR_LINK_SIG;
              VEC_PUSH(&traversal->actions, act);
              node_ind_t target = VEC_PEEK(traversal->node_stack);
              VEC_PUSH(&traversal->node_stack, target);
              VEC_PUSH(&traversal->node_stack, res.data.node_data.node_index);
            }
            break;
          }
          case PT_ALL_STATEMENT_LET:
          case PT_ALL_PAT_WILDCARD: {
            if (traversal->wanted_actions.edit_environment) {
              scoped_traverse_action act = TR_PUSH_SCOPE_VAR;
              VEC_PUSH(&traversal->actions, act);
              VEC_PUSH(&traversal->node_stack, res.data.node_data.node_index);
            }
            break;
          }
          case PT_ALL_STATEMENT_FUN:
          case PT_ALL_EX_FN: {
            if (traversal->wanted_actions.edit_environment) {
              scoped_traverse_action act = TR_POP_TO;
              VEC_PUSH(&traversal->actions, act);
              VEC_PUSH(&traversal->amt_stack, traversal->environment_amt);
            }
            break;
          }
          default:
            break;
        }

        const tree_node_repr repr = pt_subs_type[res.data.node_data.node.type.all];

        switch (repr) {
          case SUBS_EXTERNAL:
            VEC_APPEND_REVERSE(&traversal->node_stack,
                               res.data.node_data.node.sub_amt,
                               &traversal->inds[res.data.node_data.node.subs_start]);
            const scoped_traverse_action act = TR_VISIT_IN;
            VEC_REPLICATE(&traversal->actions, res.data.node_data.node.sub_amt, act);
            break;
          case SUBS_NONE:
            break;
          case SUBS_TWO:
            push_scoped_traverse_action(traversal, TR_VISIT_IN, res.data.node_data.node.sub_b);
            HEDLEY_FALL_THROUGH;
          case SUBS_ONE:
            push_scoped_traverse_action(traversal, TR_VISIT_IN, res.data.node_data.node.sub_a);
            break;
        }
        if (should_traverse_in(traversal->wanted_actions, res.data.node_data.node.type.all)) {
          return res;
        }
        break;
      case TR_POP_TO:
        VEC_POP(&traversal->amt_stack, &res.data.new_environment_amount);
        traversal->environment_amt = res.data.new_environment_amount;
        return res;
      case TR_END:
        VEC_FREE(&traversal->actions);
        VEC_FREE(&traversal->node_stack);
        // VEC_FREE(&traversal->path);
        return res;
      case TR_LINK_SIG: {
        node_ind_t node_ind;
        node_ind_t target_ind;
        VEC_POP(&traversal->node_stack, &node_ind);
        VEC_POP(&traversal->node_stack, &target_ind);
        res.data.link_sig_data.sig_index = node_ind;
        res.data.link_sig_data.linked_index = target_ind;
        return res;
      }
    }
  }
}
