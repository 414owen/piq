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
    BUILD_EXPRESSION_CONSTRUCTION_2,
    BUILD_CALL_2,
    BUILD_CALL_3,
    BUILD_FUN_2,
    BUILD_TUP_PAT_2,
    BUILD_TUP_PAT_3,
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
  ir_expression a;
  ir_expression b;
} two_exressions;

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

  vec_ir_pattern ir_statement = VEC_NEW;
  // vec_node_ind ir_statement_amts = VEC_NEW;

  vec_ir_pattern ir_patterns = VEC_NEW;
  // I think we can get this from the current node
  // vec_node_ind ir_pattern_amts = VEC_NEW;

  union {
    ir_expression expression;
    ir_pattern pattern;
  } build_res;

  {
    for (node_ind_t i = 0; i < tree.root_subs_amt; i++) {
      action action = {
        .tag = BUILD_TOP_LEVEL,
        .node_ind = tree.inds[tree.root_subs_start + i],
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
        switch (node.type.pattern) {
          // Done
          case PT_PAT_UNIT: {
            build_res.pattern.ir_pattern_tag = IR_PAT_UNIT;
            break;
          }
          case PT_PAT_UPPER_NAME: {
            build_res.pattern.ir_pattern_tag = IR_PAT_UNIT;
            UNIMPLEMENTED("Lowering constructors in patterns");
            break;
          }
          // Done
          // wildcard binding
          case PT_PAT_WILDCARD:
            build_res.pattern.ir_pattern_tag = IR_PAT_PLACEHOLDER;
            build_res.pattern.ir_pattern_binding_span = node.span;
            break;
          // Done
          case PT_PAT_LIST: {
            for (size_t i = 0; i < PT_LIST_SUB_AMT(node); i++) {
              action todos[] = {
                {
                  .tag = BUILD_LIST_PAT_2,
                  .node_ind = act.node_ind,
                },
                {
                  .tag = BUILD_PATTERN,
                  .node_ind = PT_LIST_SUB_IND(tree.inds, node, i),
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
          case PT_PAT_TUP:
            build_res.pattern.ir_pattern_tag = IR_PAT_TUP;
            // TODO finish this
            action todos[] = {
              {
                .tag = BUILD_TUP_PAT_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_PATTERN,
                .node_ind = PT_TUP_SUB_A(node),
              },
            };
            VEC_APPEND_STATIC(&actions, todos);
            UNIMPLEMENTED("Lowering tuples");
            break;
          case PT_PAT_INT:
            build_res.pattern.ir_pattern_tag = IR_PAT_INT;
            build_res.pattern.ir_pattern_int_span = node.span;
            break;
          case PT_PAT_STRING:
            build_res.pattern.ir_pattern_tag = IR_PAT_STRING;
            build_res.pattern.ir_pattern_str_span = node.span;
            break;
          case PT_ALL_PAT_CONSTRUCTION:
            // TODO construction
            UNIMPLEMENTED("Lowering pattern constructions");
            break;
        }
        break;
      }
      case BUILD_TUP_PAT_2:
      case BUILD_LIST_PAT_2: {
        VEC_PUSH(&ir_patterns, build_res.pattern);
        break;
      }
      // Use the topmost vector of patterns
      case BUILD_LIST_PAT_3: {
        ir_pattern *patterns = VEC_GET_PTR(ir_patterns, node.sub_amt);
        node_ind_t start = module.ir_patterns.len;
        VEC_APPEND(&module.ir_patterns, node.sub_amt, patterns);
        ir_pattern pattern = {
          .ir_pattern_type = ti,
          .ir_pattern_tag = IR_PAT_LIST,
          .subs =
            {
              .start = start,
              .amt = node.sub_amt,
            },
        };
        // TODO is it more efficient to assign the three fields individually?
        build_res.pattern = pattern;
        break;
      }
      case BUILD_TUP_PAT_3: {
        ir_pattern *patterns = VEC_GET_PTR(ir_patterns, node.sub_amt);
        node_ind_t start = module.ir_patterns.len;
        VEC_APPEND(&module.ir_patterns, node.sub_amt, patterns);
        ir_pattern pattern = {
          .ir_pattern_type = ti,
          .subs =
            {
              .start = start,
              .amt = node.sub_amt,
            },
        };
        // TODO is it more efficient to assign the three fields individually?
        build_res.pattern = pattern;
        break;
      }
      case BUILD_TOP_LEVEL: {
        switch (node.type.top_level) {
          case PT_TL_FUN: {
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
          case PT_TL_SIG:
            // I guess do nothing?
            break;
        }
        break;
      }
      // TODO get rid of this, and make BUILD_EXPR and similar groups
      case BUILD_EXPR:
        build_res.expression.ir_expression_type = ti;
        switch (node.type.expression) {
          case PT_EX_CALL: {
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
          case PT_EX_FN:
            build_res.expression.ir_expression_tag = IR_EXPRESSION_FN;
            UNIMPLEMENTED("build clusure fn");
            break;
          case PT_EX_IF: {
            build_res.expression.ir_expression_tag = IR_EXPRESSION_IF;
            // TODO
            UNIMPLEMENTED("Lowering IFs");
            /*
            action todo[] = {
              {
                .tag = BUILD_IF_EXPRESSION_2,
                .node_ind = act.node_ind,
              },
              {
                .tag = BUILD_EXPR,
                .node_ind = node.sub_a,
              },
              {
                .tag = BUILD_IF_EXPRESSION_2,
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
          case PT_EX_INT: {
            type t = types.types[type_ind];
            switch (t.tag) {
              case T_I8:
                // build_res.expression.ir_expression_i8 =
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
            // build_res.expression.
            break;
          }
          case PT_EX_LIST:
            // TODO
            UNIMPLEMENTED("Lowering lists");
            break;
          case PT_EX_LOWER_NAME:
            // TODO
            UNIMPLEMENTED("Lowering variables");
            break;
          case PT_EX_STRING:
            VEC_PUSH(&module.ir_strings, node.span);
            // I guess just assume the type is string
            ir_type_ind ti = {
              .n = type_ind,
            };
            build_res.expression.ir_expression_type = ti;
            break;
          case PT_EX_TUP:
            // TODO
            UNIMPLEMENTED("Lowering tuple");
            break;
          case PT_EX_AS:
            // TODO
            UNIMPLEMENTED("Lowering as");
            break;
          case PT_EX_UNIT:
            // TODO
            UNIMPLEMENTED("Lowering unit");
            break;
          case PT_EX_UPPER_NAME:
            // TODO
            UNIMPLEMENTED("Lowering constructor");
            break;
          case PT_EX_FUN_BODY:
            // TODO
            UNIMPLEMENTED("Lowering fun body");
            break;
        }
        break;
      case BUILD_EXPRESSION_CONSTRUCTION_2: {
        ir_data_construction construction = {
          .ir_data_construction_param = build_res.expression,
        };
        VEC_PUSH(&module.ir_data_constructions, construction);
        ir_expression expression = {
          .ir_expression_tag = IR_EXPRESSION_CONSTRUCTOR,
          .ir_expression_type = ti,
        };
        build_res.expression = expression;
        break;
      }
      case BUILD_CALL_2: {
        ir_expression callee = build_res.expression;
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
        build_res.expression.ir_expression_tag = IR_EXPRESSION_CALL;
        ir_expression param = build_res.expression;
        act.partial_call.ir_call_param = param;
        VEC_PUSH(&module.ir_calls, act.partial_call);
        ir_expression e = {
          .ir_expression_tag = IR_EXPRESSION_CALL,
          .ir_expression_type = ti,
          .ir_expression_ind.n = module.ir_calls.len - 1,
        };
        UNIMPLEMENTED("Lowering call");
        break;
      }
      case BUILD_FUN_2: {
        ir_fun_case fun = {
          .ir_fun_param = build_res.pattern,
          // TODO with a vec_statement like the patterns
          //.ir_fun_body_statements_start =
        };
        break;
      }
    }
  }
  return module;
}
