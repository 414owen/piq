#include <stdbool.h>
#include <hedley.h>

#include "binding.h"
#include "ir.h"
#include "resolve.h"
#include "scope.h"
#include "util.h"
#include "vec.h"
#include "parse_tree.h"

typedef struct {
  enum { RESOLVE_NODE, RESOLVE_TYPE } tag;
  node_ind_t ind;
} action;

VEC_DECL(action);

typedef struct {
  vec_node_ind node_inds;
  vec_str_ref bindings;
  vec_ir_expr_type expr_types;
  // TODO add data types
} scope;

scope scope_new(void) {
  scope res = {
    .node_inds = VEC_NEW,
    .bindings = VEC_NEW,
    .expr_types = VEC_NEW,
  };
  return res;
}

typedef struct {
  vec_action actions;
  char *source;
  parse_tree tree;
  ir_module module;
  vec_resolve_error errors;

  node_ind_t ir_root_ind;
  node_ind_t ir_call_ind;
  node_ind_t ir_construction_ind;
  node_ind_t ir_fn_ind;
  node_ind_t ir_fn_type_ind;
  node_ind_t ir_fun_body_ind;
  node_ind_t ir_if_ind;
  node_ind_t ir_int_ind;
  node_ind_t ir_list_ind;
  node_ind_t ir_list_type_ind;
  node_ind_t ir_string_ind;
  node_ind_t ir_tup_ind;
  node_ind_t ir_as_ind;
  node_ind_t ir_unit_ind;
  node_ind_t ir_fun_ind;
  node_ind_t ir_sig_ind;
  node_ind_t ir_let_group_ind;
  node_ind_t it_fun_group_ind;

  // TODO rename to term_scope?
  scope scope;
  // vec of amount of items in scope at different layers;
  vec_node_ind scope_layers;
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

static ir_module module_new(parse_tree tree) {
  ir_module module = {
    .ir_root = {0},
    .ir_types = NULL,
    .ir_node_inds = NULL,
  };

  node_ind_t PT_ROOT_AMT = 0;
  node_ind_t PT_CALL_AMT = 0;
  node_ind_t PT_CONSTRUCTION_AMT = 0;
  node_ind_t PT_FN_AMT = 0;
  node_ind_t PT_FN_TYPE_AMT = 0;
  node_ind_t PT_FUN_BODY_AMT = 0;
  node_ind_t PT_IF_AMT = 0;
  node_ind_t PT_INT_AMT = 0;
  node_ind_t PT_LIST_AMT = 0;
  node_ind_t PT_LIST_TYPE_AMT = 0;
  node_ind_t PT_STRING_AMT = 0;
  node_ind_t PT_TUP_AMT = 0;
  node_ind_t PT_AS_AMT = 0;
  node_ind_t PT_UNIT_AMT = 0;
  node_ind_t PT_FUN_AMT = 0;
  node_ind_t PT_SIG_AMT = 0;
  node_ind_t PT_LET_AMT = 0;

  for (size_t i = 0; i < tree.node_amt; i++) {
    parse_node node = tree.nodes[i];
    switch (node.type) {
      case PT_ROOT:
        PT_ROOT_AMT += 1;
        break;
      case PT_CALL:
        PT_CALL_AMT += 1;
        break;
      case PT_CONSTRUCTION:
        PT_CONSTRUCTION_AMT += 1;
        break;
      case PT_FN:
        PT_FN_AMT += 1;
        break;
      case PT_FN_TYPE:
        PT_FN_TYPE_AMT += 1;
        break;
      case PT_FUN_BODY:
        PT_FUN_BODY_AMT += 1;
        break;
      case PT_IF:
        PT_IF_AMT += 1;
        break;
      case PT_INT:
        PT_INT_AMT += 1;
        break;
      case PT_LIST:
        PT_LIST_AMT += 1;
        break;
      case PT_LIST_TYPE:
        PT_LIST_TYPE_AMT += 1;
        break;
      case PT_STRING:
        PT_STRING_AMT += 1;
        break;
      case PT_TUP:
        PT_TUP_AMT += 1;
        break;
      case PT_AS:
        PT_AS_AMT += 1;
        break;
      case PT_UNIT:
        PT_UNIT_AMT += 1;
        break;
      case PT_FUN:
        PT_FUN_AMT += 1;
        break;
      case PT_SIG:
        PT_SIG_AMT += 1;
        break;
      case PT_LET:
        PT_LET_AMT += 1;
        break;
      case PT_LOWER_NAME:
      case PT_UPPER_NAME:
        break;
    }
  }

  char *ptr =
    malloc((sizeof(module.ir_calls[0]) * PT_CALL_AMT) +
           (sizeof(module.ir_constructions[0]) * PT_CONSTRUCTION_AMT) +
           (sizeof(module.ir_fns[0]) * PT_FN_AMT) +
           (sizeof(module.ir_fn_types[0]) * PT_FN_TYPE_AMT) +
           (sizeof(module.ir_ifs[0]) * PT_IF_AMT) +
           (sizeof(module.ir_ints[0]) * PT_INT_AMT) +
           (sizeof(module.ir_lists[0]) * PT_LIST_AMT) +
           (sizeof(module.ir_list_types[0]) * PT_LIST_TYPE_AMT) +
           (sizeof(module.ir_strings[0]) * PT_STRING_AMT) +
           (sizeof(module.ir_tups[0]) * PT_TUP_AMT) +
           (sizeof(module.ir_ass[0]) * PT_AS_AMT) +
           (sizeof(module.ir_units[0]) * PT_UNIT_AMT) +
           (sizeof(module.ir_fun_groups[0]) * PT_FUN_AMT) +
           (sizeof(module.ir_let_groups[0]) * PT_LET_AMT));

  // Leave this here. Must match IR_MODULE_ALLOC_PTR
  module.ir_fun_groups = (typeof(module.ir_fun_groups[0]) *)ptr;
  ptr += sizeof(module.ir_fun_groups[0]) * PT_FUN_AMT;

  module.ir_let_groups = (typeof(module.ir_let_groups[0]) *)ptr;
  ptr += sizeof(module.ir_let_groups[0]) * PT_LET_AMT;

  module.ir_calls = (typeof(module.ir_calls[0]) *)ptr;
  ptr += sizeof(module.ir_calls[0]) * PT_CALL_AMT;

  module.ir_constructions = (typeof(module.ir_constructions[0]) *)ptr;
  ptr += sizeof(module.ir_constructions[0]) * PT_CONSTRUCTION_AMT;

  module.ir_fns = (typeof(module.ir_fns[0]) *)ptr;
  ptr += sizeof(module.ir_fns[0]) * PT_FN_AMT;

  module.ir_fn_types = (typeof(module.ir_fn_types[0]) *)ptr;
  ptr += sizeof(module.ir_fn_types[0]) * PT_FN_TYPE_AMT;

  module.ir_ifs = (typeof(module.ir_ifs[0]) *)ptr;
  ptr += sizeof(module.ir_ifs[0]) * PT_IF_AMT;

  module.ir_ints = (typeof(module.ir_ints[0]) *)ptr;
  ptr += sizeof(module.ir_ints[0]) * PT_INT_AMT;

  module.ir_lists = (typeof(module.ir_lists[0]) *)ptr;
  ptr += sizeof(module.ir_lists[0]) * PT_LIST_AMT;

  module.ir_list_types = (typeof(module.ir_list_types[0]) *)ptr;
  ptr += sizeof(module.ir_list_types[0]) * PT_LIST_TYPE_AMT;

  module.ir_strings = (typeof(module.ir_strings[0]) *)ptr;
  ptr += sizeof(module.ir_strings[0]) * PT_STRING_AMT;

  module.ir_tups = (typeof(module.ir_tups[0]) *)ptr;
  ptr += sizeof(module.ir_tups[0]) * PT_TUP_AMT;

  module.ir_ass = (typeof(module.ir_ass[0]) *)ptr;
  ptr += sizeof(module.ir_ass[0]) * PT_AS_AMT;

  module.ir_units = (typeof(module.ir_units[0]) *)ptr;
  ptr += sizeof(module.ir_units[0]) * PT_UNIT_AMT;

  return module;
}

static bool bindings_eq(char *source, binding a, binding b) {
  if (a.end - a.start != b.end - b.start) {
    return false;
  }
  return strncmp(source + a.start, source + b.start, a.end - a.start + 1) == 0;
}

static void resolve_root(state *state, parse_node node) {

#define NOT_ROOT                                                               \
  case PT_ROOT:                                                                \
  case PT_CALL:                                                                \
  case PT_CONSTRUCTION:                                                        \
  case PT_FN:                                                                  \
  case PT_FN_TYPE:                                                             \
  case PT_FUN_BODY:                                                            \
  case PT_IF:                                                                  \
  case PT_INT:                                                                 \
  case PT_LIST:                                                                \
  case PT_LIST_TYPE:                                                           \
  case PT_LOWER_NAME:                                                          \
  case PT_STRING:                                                              \
  case PT_TUP:                                                                 \
  case PT_AS:                                                                  \
  case PT_UNIT:                                                                \
  case PT_UPPER_NAME:                                                          \
  case PT_LET

  bool last_was_sig = false;
  binding last_sig_bnd = {0};
  node_ind_t last_sig_ind = 0;

  for (node_ind_t i = 0; i < node.sub_amt; i++) {
    node_ind_t sub_ind = state->tree.inds[node.subs_start + i];
    parse_node sub = state->tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG: {
        node_ind_t bnd_ind = PT_SIG_BINDING_IND(sub);
        binding bnd = state->tree.nodes[bnd_ind].span;
        last_was_sig = true;
        last_sig_bnd = bnd;
        push_resolve_type(state, PT_SIG_TYPE_IND(sub));
        break;
      }
      case PT_FUN: {
        last_was_sig = false;
        node_ind_t bnd_ind = PT_FUN_BINDING_IND(state->tree.inds, sub);
        parse_node bnd = state->tree.nodes[bnd_ind];
        ir_fun_group group = {
          .sig_ind = -1,
          .fun_ind = -1,
        };
        if (last_was_sig) {
          if (bindings_eq(state->source, last_sig_bnd, bnd.span)) {
            group.sig_ind = last_sig_ind;
          } else {
            resolve_error err = {
              .tag = SIG_MISMATCH,
              .node_type = FUN_NODE,
              .node_ind = sub_ind,
            };
            VEC_PUSH(&state->errors, err);
          }
        }
        VEC_PUSH(&state->module.ir_fun_groups, group);
        push_resolve_node(state, sub_ind);
        break;
      }
      NOT_ROOT:
        give_up("Non-root node detected");
        break;
    }
  }
#undef NOT_ROOT
}

static void resolve_node(state *state, node_ind_t node_ind) {
  parse_node node = state->tree.nodes[node_ind];
  switch (node.type) {
    case PT_ROOT:
      resolve_root(state, node);
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
      break;
  }
}

resolve_res resolve(char *source, parse_tree tree) {
  state state = {
    .actions = VEC_NEW,
    .source = source,
    .tree = tree,
    .module = module_new(tree),
    .scope = scope_new(),
    // I hope this does what I think it does...
    // Someone read the C spec and tell me?
    {0},
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
  resolve_res res = {
    .module = state.module,
    .error_amt = state.errors.len,
    .errors = VEC_FINALIZE(&state.errors),
  };
  return res;
}
