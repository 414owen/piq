#include <stdlib.h>

#include "ir.h"
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

ir_module build_module(parse_tree tree) {
  ir_module module;
  vec_action actions;

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
    switch (act.tag) {
      case BUILD_FUN_2: {
        break;
      }
      case BUILD_PATTERN: {
        UNIMPLEMENTED("Building patterns");
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
      }
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
    }
  }
  return module;
}
