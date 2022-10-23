#include <stdlib.h>

#include "ir.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

typedef struct {
  ir_module module;
} state;

typedef struct {
  enum {
    BUILD_PATTERN,
    BUILD_TOP_LEVEL,
    BUILD_NODE,
    BUILD_CALL_2,
    BUILD_FUN_2,
  } tag;
  node_ind_t node_ind;
} action;

VEC_DECL(action);

typedef struct {
  ir_expr a;
  ir_expr b;
} two_exprs;

typedef union {
  ir_fun_group ir_fun_groups;
  ir_let_group ir_let_groups;
  ir_call ir_calls;
  ir_data_construction ir_constructions;
  ir_fn ir_fns;
  ir_fn_type ir_fn_types;
  ir_if ir_ifs;
  ir_list ir_lists;
  ir_list_type ir_list_types;
  ir_string ir_strings;
  ir_tup ir_tups;
  ir_as ir_ass;
} ret;

static ir_module new_module(void) {
  ir_root root = {
    .ir_root_function_decls_start = {0},
    .ir_root_function_decl_amt = {0},
    .ir_root_data_decls_start = {0},
    .ir_root_data_decl_amt = {0},
  };

  ir_module res = {
    .ir_root = root,
    .ir_let_groups = VEC_NEW,
    .ir_fun_groups = VEC_NEW,
    .ir_fn_types = VEC_NEW,
    .ir_strings = VEC_NEW,
    .ir_list_types = VEC_NEW,
    .types = VEC_NEW,
    .node_inds = VEC_NEW,
  };

  return res;
}

ir_module build_module(parse_tree tree, type_info types) {
  ir_module module = new_module();
  vec_action actions;

  union {
    ir_expr expr;
    ir_pattern pattern;
  } build_res;

  {
    parse_node root = tree.nodes[tree.root_ind];
    for (node_ind_t i = 0; i < root.sub_amt; i++) {
      action action = {
        .tag = BUILD_TOP_LEVEL,
        .node_ind = PT_ROOT_SUB_IND(tree.inds, root, i),
      };
      VEC_PUSH(&actions, action);
    }
  }

  while (actions.len > 0) {
    action act = VEC_POP(&actions);
    parse_node node = tree.nodes[act.node_ind];
    node_ind_t type_ind = types.node_types[act.node_ind];
    ir_type_ind ti = {
      .n = type_ind,
    };
    switch (act.tag) {
      case BUILD_PATTERN: {
        build_res.pattern.type = ti;
        switch (node.type) {
          case PT_UNIT: {
            // assume the type is unit, I guess
            break;
          }
          // wildcard binding
          case PT_LOWER_NAME:
            // TODO introduce binding somehow
            build_res.pattern.ir_pattern_binding_span = node.span;
            break;
          case PT_LIST: {
            // TODO
            action todo[] = {
              {
                .tag = BUILD_LIST_PAT_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_PATTERN,
                .node_ind = PT_LIST_SUB_IND(tree.inds, node),
              },
            };
            VEC_APPEND_STATIC(&actions, todo);
            break;
          }
          case PT_TUP:
            break;
          case PT_INT:
            build_res.pattern.ir_pattern_int_span = node.span;
            break;
          case PT_STRING:
            build_res.pattern.ir_pattern_str_span = node.span;
            break;
        }
        break;
      }
      case BUILD_TOP_LEVEL: {
        switch (node.type) {
          case PT_FUN: {
            action todo[] = {
              {
                .tag = BUILD_FUN_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_PATTERN,
                .node_ind = PT_FUN_PARAM_IND(tree.inds, node),
              },
            };
            VEC_APPEND_STATIC(&actions, todo);
            break;
          }
          default:
            give_up("Unexpected non-top-level parse node");
            break;
        }
        break;
      }
      // TODO get rid of this, and make BUILD_EXPR and similar groups
      case BUILD_NODE:
        switch (node.type) {
          case PT_CALL: {
            action todo[] = {
              {
                .tag = BUILD_CALL_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_NODE,
                .node_ind = node.sub_a,
              },
            };
            VEC_APPEND_STATIC(&actions, todo);
            break;
          }
          case PT_CONSTRUCTION:
            UNIMPLEMENTED("build construction");
            break;
          case PT_FN:
            break;
          case PT_FN_TYPE:
            break;
          case PT_FUN:
            break;
          case PT_FUN_BODY:
            break;
          case PT_IF:
            break;
          case PT_INT:
            break;
          case PT_LIST:
            break;
          case PT_LIST_TYPE:
            break;
          case PT_LOWER_NAME:
            break;
          case PT_STRING:
            VEC_PUSH(&module.ir_strings, node.span);
            // I guess just assume the type is string
            ir_type_ind ti = {
              .n = type_ind,
            };
            build_res.expr.type = ti;
            break;
          case PT_TUP:
            break;
          case PT_AS:
            break;
          case PT_UNIT:
            break;
          case PT_UPPER_NAME:
            break;
          case PT_SIG:
            break;
          case PT_LET:
            break;
          case PT_ROOT:
            give_up("unexpected node in IR translation");
            break;
        }
        break;
      case BUILD_CALL_2:
        UNIMPLEMENTED("build call 2");
        break;
      case BUILD_FUN_2:
        UNIMPLEMENTED("build fun 2");
        break;
    }
  }
  return module;
}
