#include <stdbool.h>
#include <hedley.h>

#include "binding.h"
#include "ir.h"
#include "parse_tree.h"
#include "resolve.h"
#include "semantics.h"
#include "scope.h"
#include "util.h"
#include "vec.h"

typedef struct {
  node_ind_t pt_ind;
  node_ind_t ir_ind;
  enum { RESOLVE_NODE, RESOLVE_TYPE } tag;
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
  node_ind_t ir_fun_group_ind;

  // TODO rename to term_scope?
  scope scope;
  // vec of amount of items in scope at different layers;
  vec_node_ind scope_layers;
} state;

typedef struct {
  node_ind_t pt_root;
  node_ind_t pt_call;
  node_ind_t pt_construction;
  node_ind_t pt_fn;
  node_ind_t pt_fn_type;
  node_ind_t pt_fun_body;
  node_ind_t pt_if;
  node_ind_t pt_int;
  node_ind_t pt_list;
  node_ind_t pt_list_type;
  node_ind_t pt_string;
  node_ind_t pt_tup;
  node_ind_t pt_as;
  node_ind_t pt_unit;
  node_ind_t pt_fun;
  node_ind_t pt_sig;
  node_ind_t pt_let;
} pt_node_amounts;

static pt_node_amounts count_pt_node_amounts(parse_tree tree) {
  pt_node_amounts res = {0};

  for (size_t i = 0; i < tree.node_amt; i++) {
    parse_node node = tree.nodes[i];
    switch (node.type) {
      case PT_ROOT:
        res.pt_root += 1;
        break;
      case PT_CALL:
        res.pt_call += 1;
        break;
      case PT_CONSTRUCTION:
        res.pt_construction += 1;
        break;
      case PT_FN:
        res.pt_fn += 1;
        break;
      case PT_FN_TYPE:
        res.pt_fn_type += 1;
        break;
      case PT_FUN_BODY:
        res.pt_fun_body += 1;
        break;
      case PT_IF:
        res.pt_if += 1;
        break;
      case PT_INT:
        res.pt_int += 1;
        break;
      case PT_LIST:
        res.pt_list += 1;
        break;
      case PT_LIST_TYPE:
        res.pt_list_type += 1;
        break;
      case PT_STRING:
        res.pt_string += 1;
        break;
      case PT_TUP:
        res.pt_tup += 1;
        break;
      case PT_AS:
        res.pt_as += 1;
        break;
      case PT_UNIT:
        res.pt_unit += 1;
        break;
      case PT_FUN:
        res.pt_fun += 1;
        break;
      case PT_SIG:
        res.pt_sig += 1;
        break;
      case PT_LET:
        res.pt_let += 1;
        break;
      case PT_LOWER_NAME:
      case PT_UPPER_NAME:
        break;
    }
  }

  return res;
}

static size_t ir_count_bytes_required(pt_node_amounts *amounts) {
  return (sizeof(ir_call) * amounts->pt_call) +
         (sizeof(ir_construction) * amounts->pt_construction) +
         (sizeof(ir_fn) * amounts->pt_fn) +
         (sizeof(ir_fn_type) * amounts->pt_fn_type) +
         (sizeof(ir_if) * amounts->pt_if) + (sizeof(ir_int) * amounts->pt_int) +
         (sizeof(ir_list) * amounts->pt_list) +
         (sizeof(ir_list_type) * amounts->pt_list_type) +
         (sizeof(ir_string) * amounts->pt_string) +
         (sizeof(ir_tup) * amounts->pt_tup) + (sizeof(ir_as) * amounts->pt_as) +
         (sizeof(ir_unit) * amounts->pt_unit) +
         (sizeof(ir_fun_group) * amounts->pt_fun) +
         (sizeof(ir_let_group) * amounts->pt_let);
}

static ir_module module_new(pt_node_amounts *amounts, char *ptr) {
  ir_module module = {
    .ir_root = {0},
    .ir_types = NULL,
    .ir_node_inds = NULL,
  };

  // Leave this here. Must match IR_MODULE_ALLOC_PTR
  module.ir_fun_groups = (typeof(module.ir_fun_groups[0]) *)ptr;
  ptr += sizeof(module.ir_fun_groups[0]) * amounts->pt_fun;

  module.ir_let_groups = (typeof(module.ir_let_groups[0]) *)ptr;
  ptr += sizeof(module.ir_let_groups[0]) * amounts->pt_let;

  module.ir_calls = (typeof(module.ir_calls[0]) *)ptr;
  ptr += sizeof(module.ir_calls[0]) * amounts->pt_call;

  module.ir_constructions = (typeof(module.ir_constructions[0]) *)ptr;
  ptr += sizeof(module.ir_constructions[0]) * amounts->pt_construction;

  module.ir_fns = (typeof(module.ir_fns[0]) *)ptr;
  ptr += sizeof(module.ir_fns[0]) * amounts->pt_fn;

  module.ir_fn_types = (typeof(module.ir_fn_types[0]) *)ptr;
  ptr += sizeof(module.ir_fn_types[0]) * amounts->pt_fn_type;

  module.ir_ifs = (typeof(module.ir_ifs[0]) *)ptr;
  ptr += sizeof(module.ir_ifs[0]) * amounts->pt_if;

  module.ir_ints = (typeof(module.ir_ints[0]) *)ptr;
  ptr += sizeof(module.ir_ints[0]) * amounts->pt_int;

  module.ir_lists = (typeof(module.ir_lists[0]) *)ptr;
  ptr += sizeof(module.ir_lists[0]) * amounts->pt_list;

  module.ir_list_types = (typeof(module.ir_list_types[0]) *)ptr;
  ptr += sizeof(module.ir_list_types[0]) * amounts->pt_list_type;

  module.ir_strings = (typeof(module.ir_strings[0]) *)ptr;
  ptr += sizeof(module.ir_strings[0]) * amounts->pt_string;

  module.ir_tups = (typeof(module.ir_tups[0]) *)ptr;
  ptr += sizeof(module.ir_tups[0]) * amounts->pt_tup;

  module.ir_ass = (typeof(module.ir_ass[0]) *)ptr;
  ptr += sizeof(module.ir_ass[0]) * amounts->pt_as;

  module.ir_units = (typeof(module.ir_units[0]) *)ptr;
  ptr += sizeof(module.ir_units[0]) * amounts->pt_unit;

  return module;
}

static state state_new(char *source, parse_tree tree) {
  state res = {
    .actions = VEC_NEW,
    .source = source,
    .tree = tree,
    .errors = VEC_NEW,

    .ir_root_ind = 0,
    .ir_call_ind = 0,
    .ir_construction_ind = 0,
    .ir_fn_ind = 0,
    .ir_fn_type_ind = 0,
    .ir_fun_body_ind = 0,
    .ir_if_ind = 0,
    .ir_int_ind = 0,
    .ir_list_ind = 0,
    .ir_list_type_ind = 0,
    .ir_string_ind = 0,
    .ir_tup_ind = 0,
    .ir_as_ind = 0,
    .ir_unit_ind = 0,
    .ir_fun_ind = 0,
    .ir_sig_ind = 0,
    .ir_let_group_ind = 0,
    .ir_fun_group_ind = 0,

    // TODO rename to term_scope?
    .scope = scope_new(),
    // amount of items in scope at different layers = 0,
    .scope_layers = VEC_NEW,
  };

  // This is done here in case we want to allocate any state along with
  // the module nodes
  pt_node_amounts node_amounts = count_pt_node_amounts(tree);
  size_t ir_node_bytes = ir_count_bytes_required(&node_amounts);
  char *ptr = malloc(ir_node_bytes);
  res.module = module_new(&node_amounts, ptr);

  return res;
}

static void push_resolve_node(state *state, node_ind_t pt_ind,
                              node_ind_t ir_ind) {
  action a = {
    .tag = RESOLVE_NODE,
    .pt_ind = pt_ind,
    .ir_ind = ir_ind,
  };
  VEC_PUSH(&state->actions, a);
}

static void push_resolve_type(state *state, node_ind_t pt_ind) {
  action a = {
    .tag = RESOLVE_TYPE,
    .pt_ind = pt_ind,
  };
  VEC_PUSH(&state->actions, a);
}

static bool bindings_eq(char *source, binding a, binding b) {
  if (a.end - a.start != b.end - b.start) {
    return false;
  }
  return strncmp(source + a.start, source + b.start, a.end - a.start + 1) == 0;
}

typedef struct {
  const bool enforce_top_level_sigs: 1;
} resolve_block_options;

typedef enum {
  BS_START,
  BS_AFTER_SIG,
  BS_AFTER_FN,
} block_state;

static void resolve_block(const resolve_block_options options, state *state, node_ind_t node_ind) {
  parse_node node = state->tree.nodes[node_ind];
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

  block_state block_state = BS_START;
  binding last_sig_bnd = {0};
  node_ind_t last_sig_ind = 0;
  ir_fun_group fun_group = {0};
  ir_sig sig;

  for (node_ind_t i = 0; i < node.sub_amt; i++) {
    node_ind_t sub_ind = state->tree.inds[node.subs_start + i];
    parse_node sub = state->tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG: {
        node_ind_t bnd_ind = PT_SIG_BINDING_IND(sub);
        binding bnd = state->tree.nodes[bnd_ind].span;
        last_sig_bnd = bnd;
        ir_sig sig_ = {
          .s = sub.span,
          .binding = bnd,
          .type_ind = -1,
        };
        sig = sig_;
        // VEC_PUSH()
        push_resolve_type(state, PT_SIG_TYPE_IND(sub));
        block_state = BS_AFTER_SIG;
        break;
      }
      case PT_FUN: {
        switch (block_state) {
          case BS_START: {
            if (options.enforce_top_level_sigs) {
              resolve_error err = {
                .tag = BINDING_WITHOUT_SIG,
                .node_type = FUN_NODE,
                .node_ind = sub_ind,
              };
              VEC_PUSH(&state->errors, err);
            }
            break;
          }
          case BS_AFTER_FN:
          case BS_AFTER_SIG: {
            node_ind_t bnd_ind = PT_FUN_BINDING_IND(state->tree.inds, sub);
            parse_node bnd = state->tree.nodes[bnd_ind];

            if (block_state == BS_AFTER_SIG) {
              if (bindings_eq(state->source, last_sig_bnd, bnd.span)) {
                fun_group.sig = sig;
                fun_group.fun = {
                  .s = sub.span,
                  .b = bnd,
                  //TODO
                  .param_pattern_ind = VEC_PUSH(&state->ac)
                  node_ind_t body_start;
                  node_ind_t body_amt;
                  
                };
              } else {
                resolve_error err = {
                  .tag = SIG_MISMATCH,
                  .node_type = FUN_NODE,
                  .node_ind = sub_ind,
                };
                VEC_PUSH(&state->errors, err);
              }
            }
            state->module.ir_fun_groups[state->ir_fun_group_ind] = fun_group;
            push_resolve_node(state, sub_ind, state->ir_fun_group_ind++);
            break;
          }
        }
        break;
      }
      NOT_ROOT:
        give_up("Non-root node detected");
        break;
    }
  }
#undef NOT_ROOT
}

static const resolve_block_options root_block_options = {
  .enforce_top_level_sigs = ENFORCE_TOP_LEVEL_SIGS,
};

static void resolve_root(state *state, node_ind_t node_ind) {
  resolve_block(root_block_options, state, node_ind);
}

static void resolve_node(state *state, node_ind_t node_ind) {
  parse_node node = state->tree.nodes[node_ind];
  switch (node.type) {
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
      UNIMPLEMENTED("resolving node");
      break;
    case PT_ROOT:
      give_up("non-root root");
      break;
  }
}

static void resolve_type(state *state, node_ind_t node_ind) {
  parse_node node = state->tree.nodes[node_ind];
  switch (node.type) {
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
      UNIMPLEMENTED("resolving node");
      break;
    case PT_ROOT:
      give_up("non-root root");
      break;
  }
}

resolve_res resolve(char *source, parse_tree tree) {
  state state = state_new(source, tree);

  resolve_root(&state, tree.root_ind);

  while (state.actions.len > 0) {
    action act = VEC_POP(&state.actions);
    switch (act.tag) {
      case RESOLVE_NODE:
        resolve_node(&state, act.pt_ind);
        break;
      case RESOLVE_TYPE:
        UNIMPLEMENTED("resolving types");
        break;
    }
  }

  resolve_res res = {
    .module = state.module,
    .error_amt = state.errors.len,
    .errors = VEC_FINALIZE(&state.errors),
  };
  return res;
}
