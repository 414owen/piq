#include "binding.h"
#include "ir.h"
#include "vec.h"
#include "parse_tree.h"

typedef struct {
  enum { LOWER_NODE } tag;
  node_ind_t ind;
} action;

VEC_DECL(action);

typedef struct {
  vec_action actions;
  parse_tree tree;
  ir_module res;
} state;

static void push_lower_node(state *state, node_ind_t node_ind) {
  action a = {
    .tag = LOWER_NODE,
    .ind = node_ind,
  };
}

static ir_module ir_module_new(void) {
  ir_module module = {
    .ir_data_decls = VEC_NEW,
    .ir_data_constructors = VEC_NEW,
    .ir_type_refs = VEC_NEW,
    .ir_refs = VEC_NEW,
  };
  return module;
}

static void push_ref(state *state, binding b, bnd_type type) {
  str_ref ref = { .span = b, };
  VEC_PUSH(&state->res.ir_refs, ref);
  VEC_PUSH(&state->res.ir_bnd_types, type);
  bs_push(&state->res.ref_is_builtin, false);
}

static void lower_letrec(state *state, parse_node node) {
  for (node_ind_t i = 0; i < node.sub_amt; i++) {
    node_ind_t sub_ind = state->tree.inds[node.subs_start + i];
    parse_node sub = state->tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG: {
        
      }
      case PT_FUN: {
        node_ind_t bnd_ind = PT_FUN_BINDING_IND(state->tree.inds, sub);
        parse_node bnd = state->tree.nodes[bnd_ind];
        push_ref(state, bnd.span, R_FUN);
        break;
      }
      case PT_CALL:
      case PT_CONSTRUCTION:
      case PT_FN:
      case PT_FN_TYPE:
      case PT_FUN_BODY:
      case PT_IF:
      case PT_INT:
      case PT_LIST:
      case PT_LIST_TYPE:
      case PT_LOWER_NAME:
      case PT_STRING:
      case PT_TUP:
      case PT_AS:
      case PT_UNIT:
      case PT_UPPER_NAME:
      case PT_LET:
        UNIMPLEMENTED("findinf ")
        continue;
    }
  }
  for (node_ind_t i = 0; i < node.sub_amt; i++) {
    node_ind_t sub = state->tree.inds[node.subs_start + i];
  }
}

ir_module lower(parse_tree tree) {
  ir_module res;
  state state = {
    .actions = VEC_NEW,
    .tree = tree,
    .res = ir_module_new(),
  };

  while (state.actions.len > 0) {
    action act = VEC_POP(&state.actions);
    parse_node node = tree.nodes[act.ind];
    switch (node.type) {
      case PT_ROOT:
        lower_letrec(&state, node);
        break;
      case PT_CALL:
      case PT_CONSTRUCTION:
      case PT_FN:
      case PT_FN_TYPE:
      case PT_FUN:
      case PT_FUN_BODY:
      case PT_IF:
      case PT_INT:
      case PT_LIST:
      case PT_LIST_TYPE:
      case PT_LOWER_NAME:
      case PT_STRING:
      case PT_TUP:
      case PT_AS:
      case PT_UNIT:
      case PT_UPPER_NAME:
      case PT_SIG:
      case PT_LET:
        UNIMPLEMENTED("lowering node");
    }
  }

  push_lower_node(&state, tree.root_ind);
  return res;
}
