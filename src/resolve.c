#include "binding.h"
#include "ir.h"
#include "vec.h"
#include "parse_tree.h"

VEC_DECL(scope);

typedef struct {
  enum { RESOLVE_NODE, RESOLVE_TYPE } tag;
  node_ind_t ind;
} action;

VEC_DECL(action);

typedef struct {
  vec_action actions;
  parse_tree tree;
  scope res;
} state;

static void push_resolve_node(state *state, node_ind_t node_ind) {
  action a = {
    .tag = RESOLVE_NODE,
    .ind = node_ind,
  };
  VEC_PUSH(&state->actions, a);
}

static void push_resolve_type(state *state, node_ind_t node_ind) {
  action a = {
    .tag = RESOLVE_TYPE,
    .ind = node_ind,
  };
  VEC_PUSH(&state->actions, a);
}

static scope scope_new(void) {
  scope module = {
    .data_decls = VEC_NEW,
    .data_constructors = VEC_NEW,
    .type_refs = VEC_NEW,
    .refs = VEC_NEW,
  };
  return module;
}

static void push_ref(state *state, binding b, bnd_type type) {
  str_ref ref = { .span = b, };
  VEC_PUSH(&state->res.refs, ref);
  VEC_PUSH(&state->res.bnd_types, type);
  bs_push(&state->res.ref_is_builtin, false);
}

static void resolve_letrec(state *state, parse_node node) {

#define NOT_LETREC \
  case PT_ROOT: \
  case PT_CALL: \
  case PT_CONSTRUCTION: \
  case PT_FN: \
  case PT_FN_TYPE: \
  case PT_FUN_BODY: \
  case PT_IF: \
  case PT_INT: \
  case PT_LIST: \
  case PT_LIST_TYPE: \
  case PT_LOWER_NAME: \
  case PT_STRING: \
  case PT_TUP: \
  case PT_AS: \
  case PT_UNIT: \
  case PT_UPPER_NAME: \
  case PT_LET

  bool prev_sig = false;
  binding prev_bnd;

  for (node_ind_t i = 0; i < node.sub_amt; i++) {
    node_ind_t sub_ind = state->tree.inds[node.subs_start + i];
    parse_node sub = state->tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG: {
        prev_sig = true;
        node_ind_t bnd_ind = PT_SIG_BINDING_IND(sub);
        prev_bnd = state->tree.nodes[bnd_ind].span;
        push_resolve_type(state, PT_SIG_TYPE_IND(sub));
        break;
      }
      case PT_FUN: {
        node_ind_t bnd_ind = PT_FUN_BINDING_IND(state->tree.inds, sub);
        parse_node bnd = state->tree.nodes[bnd_ind];
        push_ref(state, bnd.span, R_FUN);
        push_resolve_node(state, sub_ind);
        break;
      }
      NOT_LETREC:
        UNIMPLEMENTED("letrec");
        break;
    }
  }
#undef NOT_LETREC
}

static void resolve_node(state *state, node_ind_t node_ind) {
  parse_node node = state->tree.nodes[node_ind];
  switch (node.type) {
    case PT_ROOT:
      resolve_letrec(state, node);
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
      UNIMPLEMENTED("resolveing node");
  }
}

scope resolve(parse_tree tree) {
  scope res;
  state state = {
    .actions = VEC_NEW,
    .tree = tree,
    .res = scope_new(),
  };

  push_resolve_node(&state, tree.root_ind);

  while (state.actions.len > 0) {
    action act = VEC_POP(&state.actions);
    switch (act.tag) {
      case RESOLVE_NODE:
        resolve_node(&state, act.ind);
        break;
      case RESOLVE_TYPE:
        UNIMPLEMENTED("resolving types");
        break;
    }
  }

  push_resolve_node(&state, tree.root_ind);
  return res;
}
