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
    BUILD_EXPR,
    BUILD_EXPR_CONSTRUCTION_2,
    BUILD_CALL_2,
    BUILD_CALL_3,
    BUILD_FUN_2,
    BUILD_LIST_PAT_2,
    BUILD_LIST_PAT_3,
  } tag;
  node_ind_t node_ind;
  union {
    ir_call partial_call;
  };
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
    .ir_ifs = VEC_NEW,
    .ir_let_groups = VEC_NEW,
    .ir_calls = VEC_NEW,
    .ir_fun_groups = VEC_NEW,
    .ir_ass = VEC_NEW,
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

  vec_ir_pattern ir_stmt = VEC_NEW;
  vec_node_ind ir_stmt_amts = VEC_NEW;

  vec_ir_pattern ir_patterns = VEC_NEW;
  vec_node_ind ir_pattern_amts = VEC_NEW;

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
        build_res.pattern.ir_pattern_type = ti;
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
            for (size_t i = 0; i < PT_LIST_SUB_AMT(node); i++) {
              action todos[] = {
                {
                  .tag = BUILD_PATTERN,
                  .node_ind = PT_LIST_SUB_IND(tree.inds, node, i),
                },
                {
                  .tag = BUILD_LIST_PAT_2,
                  .node_ind = act.node_ind,
                },
              };
              VEC_APPEND_STATIC(&actions, todos);
            }
            action todos = {
              .tag = BUILD_LIST_PAT_3,
              .node_ind = act.node_ind,
            };
            VEC_PUSH(&actions, todos);
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
          case PT_LET:
          case PT_AS:
          // TODO construction
          case PT_UPPER_NAME:
          case PT_SIG:
          case PT_IF:
          case PT_LIST_TYPE:
          case PT_ROOT:
          case PT_FUN:
          case PT_FN:
          case PT_FUN_BODY:
          case PT_FN_TYPE:
          case PT_CONSTRUCTION:
          case PT_CALL:
            give_up("Unexpected pattern node");
            break;
        }
        break;
      }
      case BUILD_LIST_PAT_2: {
        VEC_PUSH(&ir_patterns, build_res.pattern);
        *VEC_PEEK_PTR(ir_pattern_amts) += 1;
        break;
      }
      // Use the topmost vector of patterns
      case BUILD_LIST_PAT_3: {
        node_ind_t pattern_amt = VEC_POP(&ir_pattern_amts);
        ir_pattern *patterns =
          VEC_GET_PTR(ir_patterns, ir_patterns.len - pattern_amt);
        node_ind_t start = module.ir_patterns.len;
        VEC_APPEND(&module.ir_patterns, pattern_amt, patterns);
        ir_pattern pattern = {
          .ir_pattern_type = ti,
          .subs =
            {
              .start = start,
              .amt = pattern_amt,
            },
        };
        // TODO is it more efficient to assign the three fields individually?
        build_res.pattern = pattern;
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
          case PT_UNIT:
          case PT_LOWER_NAME:
          case PT_TUP:
          case PT_INT:
          case PT_STRING:
          case PT_LET:
          case PT_LIST:
          case PT_AS:
          case PT_UPPER_NAME:
          case PT_SIG:
          case PT_IF:
          case PT_LIST_TYPE:
          case PT_ROOT:
          case PT_FN:
          case PT_FUN_BODY:
          case PT_FN_TYPE:
          case PT_CONSTRUCTION:
          case PT_CALL:
            give_up("Unexpected non-top-level parse node");
            break;
        }
        break;
      }
      // TODO get rid of this, and make BUILD_EXPR and similar groups
      case BUILD_EXPR:
        build_res.expr.ir_expr_type = ti;
        switch (node.type) {
          case PT_CALL: {
            action todo[] = {
              {
                .tag = BUILD_CALL_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_EXPR,
                .node_ind = PT_CALL_CALLEE_IND(node),
              },
            };
            VEC_APPEND_STATIC(&actions, todo);
            break;
          }
          case PT_CONSTRUCTION: {
            action todo[] = {
              {
                .tag = BUILD_EXPR_CONSTRUCTION_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_EXPR,
                .node_ind = PT_CALL_CALLEE_IND(node),
              },
            };
            VEC_APPEND_STATIC(&actions, todo);
            break;
          }
          case PT_FN:
            UNIMPLEMENTED("build clusure fn");
            break;
          case PT_IF: {
            /*
            action todo[] = {
              {
                .tag = BUILD_IF_EXPR_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_EXPR,
                .node_ind = node.sub_a,
              },
              {
                .tag = BUILD_IF_EXPR_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_EXPR,
                .node_ind = node.sub_a,
              },
            };
            VEC_APPEND_STATIC(&actions, todo);
            */
            break;
          }
          case PT_INT: {
            type t = types.types[type_ind];
            switch (t.tag) {
              case T_I8:
                // build_res.expr.ir_expr_i8 =
                break;
              case T_UNKNOWN:
              case T_UNIT:
              case T_U8:
              case T_I16:
              case T_U16:
              case T_I32:
              case T_U32:
              case T_I64:
              case T_U64:
              case T_FN:
              case T_BOOL:
              case T_TUP:
              case T_LIST:
              case T_CALL:
                break;
            }
            // build_res.expr.
            break;
          }
          case PT_LIST:
            // TODO
            break;
          case PT_LIST_TYPE:
            // TODO
            break;
          case PT_LOWER_NAME:
            // TODO
            break;
          case PT_STRING:
            VEC_PUSH(&module.ir_strings, node.span);
            // I guess just assume the type is string
            ir_type_ind ti = {
              .n = type_ind,
            };
            build_res.expr.ir_expr_type = ti;
            break;
          case PT_TUP:
            // TODO
            break;
          case PT_AS:
            // TODO
            break;
          case PT_UNIT:
            // TODO
            break;
          case PT_UPPER_NAME:
            // TODO
            break;
          case PT_SIG:
          case PT_LET:
          case PT_FUN_BODY:
          case PT_FUN:
          case PT_FN_TYPE:
          case PT_ROOT:
            give_up("unexpected node in IR expr translation");
            break;
        }
        break;
      case BUILD_EXPR_CONSTRUCTION_2: {
        ir_data_construction construction = {
          .ir_data_construction_param = build_res.expr,
        };
        VEC_PUSH(&module.ir_data_constructions, construction);
        ir_expr expr = {
          .ir_expr_tag = IR_EXPR_CONSTRUCTOR,
          .ir_expr_type = ti,
        };
        build_res.expr = expr;
        break;
      }
      case BUILD_CALL_2: {
        ir_expr callee = build_res.expr;
        ir_call call = {
          .ir_call_callee = callee,
        };
        action todo[] = {
          {
            .tag = BUILD_CALL_3,
            .node_ind = act.node_ind,
            .partial_call = call,
          },
          {
            .tag = BUILD_EXPR,
            .node_ind = PT_CALL_PARAM_IND(node),
          },
        };
        VEC_APPEND_STATIC(&actions, todo);
        break;
      }
      case BUILD_CALL_3: {
        ir_expr param = build_res.expr;
        act.partial_call.ir_call_param = param;
        VEC_PUSH(&module.ir_calls, act.partial_call);
        ir_expr e = {
          .ir_expr_tag = IR_EXPR_CALL,
          .ir_expr_type = ti,
          .ir_expr_ind = module.ir_calls.len - 1,
        };
      }
      case BUILD_FUN_2: {
        ir_fun_case fun = {
          .ir_fun_param = build_res.pattern,
          // TODO with a vec_stmt like the patterns
          //.ir_fun_body_stmts_start =
        };
        break;
      }
    }
  }
  return module;
}
