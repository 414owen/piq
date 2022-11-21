#include <stdio.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

#include "llvm_shim.h"

/*

This module doesn't use the reverse actions trick used in other modules,
so please make sure to push actions onto the stack in the reverse of
the order you want them executed.

TODO: benchmark:
1. Stack of actions, where an action is a tagged union (saves lookups?)
2. Stack of action tags, separate stacks or parameters (saves space?)
3. Stack of action tags, and a stack of combined parameters for each variant
(packed?)

*/

static const char *ENTRY_STR = "entry";
static const char *THEN_STR = "then";
static const char *ELSE_STR = "else";

#ifdef DEBUG_CG
#define debug_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug_print(...)                                                       \
  do {                                                                         \
  } while (false);
#endif

#include "binding.h"
#include "bitset.h"
#include "consts.h"
#include "llvm.h"
#include "parse_tree.h"
#include "scope.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

// TODO remove these and use VEC_DECL_CUSTOM instead.
typedef LLVMValueRef llvm_value;
VEC_DECL(llvm_value);

typedef LLVMTypeRef llvm_type;
VEC_DECL(llvm_type);

typedef LLVMBasicBlockRef llvm_block;
VEC_DECL(llvm_block);

typedef LLVMValueRef LLVMFunctionRef;
typedef LLVMFunctionRef llvm_function;
VEC_DECL(llvm_function);

typedef enum {

  // INPUTS: node
  // OUTPUTS: val
  CG_EXPR,

  // INPUTS: node
  // OUTPUTS: val
  CG_STATEMENT,

  // INPUTS: node, val
  // OUTPUTS: edits the environment
  CG_PATTERN,

  // INPUTS: node, str
  // OUTPUTS: block, val
  CG_GEN_BASIC_BLOCK,

  // INPUTS: size
  // OUTPUTS: edited environment
  CG_POP_ENV_TO,

  // INPUTS: none
  // OUTPUTS: edited environment
  CG_IGNORE_VALUE,

  // TODO rename to generate_function_body?
  // INPUTS: node_ind, fun
  // OUTPUTS: none
  CG_FUN_STAGE_TWO,

  // TODO rename to something descriptive?
  // INPUTS: node_ind, fun
  // OUTPUTS: pops val, builds ret, pops fn, pops block
  CG_FUN_STAGE_THREE,
} cg_action_tag;

// TODO remove
typedef enum stage { STAGE_ONE, STAGE_TWO } stage;

typedef struct {
  stage stage;
  node_ind_t node_ind;
} cg_expr_params;

typedef struct {
  stage stage;
  node_ind_t node_ind;
} cg_statement_params;

typedef struct {
  node_ind_t node_ind;
  llvm_value val;
} cg_pattern_params;

typedef struct {
  const char *restrict name;
  llvm_function fn;
} cg_block_params;

typedef struct {
  node_ind_t amt;
} cg_pop_env_to_params;

typedef struct {
  node_ind_t node_ind;
  llvm_function fn;
} cg_fun_stage_two_params;

// We used to have multiple vectors for actions (one per parameter type), but it
// was an absolute nightmare. This way wastes some space, but because its size
// is proportional to the depth of the program construct, I think it's okay.
typedef struct {
  cg_action_tag tag;
  union {
    cg_expr_params expr_params;
    cg_statement_params statement_params;
    cg_pattern_params pattern_params;
    cg_block_params block_params;
    cg_pop_env_to_params pop_env_to_params;
    cg_fun_stage_two_params fun_stage_two_params;
  };
} cg_action;

VEC_DECL(cg_action);

typedef struct {
  LLVMContextRef context;
  LLVMBuilderRef builder;
  LLVMModuleRef module;

  vec_cg_action actions;

  // returning vals
  vec_llvm_value val_stack;
  vec_llvm_block block_stack;
  vec_llvm_function function_stack;

  // corresponds to in.types
  LLVMTypeRef *llvm_types;

  vec_str_ref env_bnds;
  vec_llvm_value env_vals;
  bitset env_is_builtin;

  source_file source;
  parse_tree parse_tree;
  type_info types;
} cg_state;

static void cg_block(cg_state *state, node_ind_t start, node_ind_t amt);

static void push_env(cg_state *state, binding bnd, llvm_value val) {
  debug_assert(val != NULL);
  str_ref str = {.binding = bnd};
  VEC_PUSH(&state->env_bnds, str);
  VEC_PUSH(&state->env_vals, val);
  bs_push(&state->env_is_builtin, false);
}

static cg_state new_cg_state(LLVMContextRef ctx, LLVMModuleRef mod,
                             source_file source, parse_tree tree,
                             type_info types) {
  cg_state state = {
    .context = ctx,
    .module = mod,
    .builder = LLVMCreateBuilderInContext(ctx),
    .actions = VEC_NEW,

    .val_stack = VEC_NEW,
    .block_stack = VEC_NEW,
    .function_stack = VEC_NEW,

    .llvm_types = (LLVMTypeRef *)calloc(types.type_amt, sizeof(LLVMTypeRef)),
    .env_bnds = VEC_NEW,
    .env_vals = VEC_NEW,
    .env_is_builtin = bs_new(),

    .source = source,
    .parse_tree = tree,
    .types = types,
  };
  // Sometimes we want to be nowhere
  VEC_PUSH(&state.block_stack, NULL);
  return state;
}

static void destroy_cg_state(cg_state *state) {
  bs_free(&state->env_is_builtin);
  free(state->llvm_types);
  LLVMDisposeBuilder(state->builder);
  VEC_FREE(&state->actions);
  VEC_FREE(&state->block_stack);
  VEC_FREE(&state->env_bnds);
  VEC_FREE(&state->env_vals);
  VEC_FREE(&state->function_stack);
  VEC_FREE(&state->val_stack);
}

static void push_action(cg_state *state, cg_action action) {
  VEC_PUSH(&state->actions, action);
}

static void push_act_ignore_value(cg_state *state) {
  cg_action act = {
    .tag = CG_IGNORE_VALUE,
  };
  push_action(state, act);
}

static void push_act_fn_two(cg_state *state, llvm_value fn, node_ind_t ind) {
  debug_assert(ind < state->parse_tree.node_amt);
  cg_fun_stage_two_params params = {
    .fn = fn,
    .node_ind = ind,
  };
  cg_action act = {
    .tag = CG_FUN_STAGE_TWO,
    .fun_stage_two_params = params,
  };
  push_action(state, act);
}

static void push_act_fn_three(cg_state *state) {
  cg_action act = {
    .tag = CG_FUN_STAGE_THREE,
  };
  push_action(state, act);
}

static void push_act_pop_env_to(cg_state *state, node_ind_t amt) {
  cg_pop_env_to_params params = {
    .amt = amt,
  };
  cg_action act = {
    .tag = CG_POP_ENV_TO,
    .pop_env_to_params = params,
  };
  push_action(state, act);
}

static void push_expression_act(cg_state *state, node_ind_t node_ind,
                                stage stage) {

  debug_assert(node_ind < state->parse_tree.node_amt);
  cg_expr_params params = {
    .stage = stage,
    .node_ind = node_ind,
  };
  cg_action act = {
    .tag = CG_EXPR,
    .expr_params = params,
  };
  push_action(state, act);
}

static void push_statement_act(cg_state *state, node_ind_t node_ind,
                               stage stage) {
  debug_assert(node_ind < state->parse_tree.node_amt);
  cg_statement_params params = {
    .stage = stage,
    .node_ind = node_ind,
  };
  cg_action act = {
    .tag = CG_STATEMENT,
    .statement_params = params,
  };
  push_action(state, act);
}

static void push_pattern_act(cg_state *state, node_ind_t node_ind,
                             llvm_value val) {
  debug_assert(node_ind < state->parse_tree.node_amt);
  cg_pattern_params params = {
    .node_ind = node_ind,
    .val = val,
  };
  cg_action act = {
    .tag = CG_PATTERN,
    .pattern_params = params,
  };
  push_action(state, act);
}

static void push_gen_basic_block(cg_state *state, llvm_function fn, const char *restrict str) {
  cg_block_params params = {
    .name = str,
    .fn = fn,
  };
  cg_action act = {
    .tag = CG_GEN_BASIC_BLOCK,
    .block_params = params,
  };
  push_action(state, act);
}

typedef enum {
  GEN_TYPE,
  // TODO speialize this, to avoid switch-in-switch?
  COMBINE_TYPE,
} gen_type_action;

VEC_DECL(gen_type_action);

static void push_gen_type_action(vec_gen_type_action *actions,
                                 gen_type_action action) {
  VEC_PUSH(actions, action);
}

static LLVMTypeRef construct_type(cg_state *state, node_ind_t root_type_ind) {

  LLVMTypeRef *llvm_types = state->llvm_types;
  {
    LLVMTypeRef prev = llvm_types[root_type_ind];
    if (prev != NULL)
      return prev;
  }

  vec_gen_type_action actions = VEC_NEW;
  push_gen_type_action(&actions, GEN_TYPE);
  vec_node_ind inds = VEC_NEW;
  VEC_PUSH(&inds, root_type_ind);
  while (inds.len > 0) {
    gen_type_action action = VEC_POP(&actions);
    node_ind_t type_ind = VEC_POP(&inds);
    type t = state->types.types[type_ind];
    switch (action) {
      case GEN_TYPE: {
        if (llvm_types[type_ind] != NULL)
          break;
        switch (t.tag) {
          case T_BOOL: {
            llvm_types[type_ind] = LLVMInt1TypeInContext(state->context);
            break;
          }
          case T_LIST: {
            push_gen_type_action(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_LIST_SUB_IND(t));
            break;
          }
          case T_I8:
          case T_U8: {
            llvm_types[type_ind] = LLVMInt8TypeInContext(state->context);
            break;
          }
          case T_I16:
          case T_U16: {
            llvm_types[type_ind] = LLVMInt16TypeInContext(state->context);
            break;
          }
          case T_I32:
          case T_U32: {
            llvm_types[type_ind] = LLVMInt32TypeInContext(state->context);
            break;
          }
          case T_I64:
          case T_U64: {
            llvm_types[type_ind] = LLVMInt64TypeInContext(state->context);
            break;
          }
          case T_TUP: {
            push_gen_type_action(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_TUP_SUB_A(t));
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_TUP_SUB_B(t));
            break;
          }
          case T_FN: {
            push_gen_type_action(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_FN_PARAM_IND(t));
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_FN_RET_IND(t));
            break;
          }
          case T_UNIT: {
            llvm_types[type_ind] = LLVMVoidTypeInContext(state->context);
            break;
          }
          case T_CALL: {
            give_up("Type parameter saturation isn't supported by the llvm "
                    "backend yet");
            break;
          }
          case T_UNKNOWN: {
            give_up("Type not filled in by typechecker!");
            break;
          }
        }
        break;
      }

      case COMBINE_TYPE: {
        switch (t.tag) {
          case T_LIST: {
            node_ind_t sub_ind = t.sub_a;
            llvm_types[type_ind] =
              (LLVMTypeRef)LLVMArrayType(llvm_types[sub_ind], 0);
            break;
          }
          case T_TUP: {
            node_ind_t sub_ind_a = T_TUP_SUB_A(t);
            node_ind_t sub_ind_b = T_TUP_SUB_B(t);
            LLVMTypeRef subs[2] = {llvm_types[sub_ind_a],
                                   llvm_types[sub_ind_b]};
            llvm_types[type_ind] =
              LLVMStructTypeInContext(state->context, subs, 2, false);
            break;
          }
          case T_FN: {
            node_ind_t param_ind = T_FN_PARAM_IND(t);
            node_ind_t ret_ind = T_FN_RET_IND(t);
            LLVMTypeRef param_type = llvm_types[param_ind];
            LLVMTypeRef ret_type = llvm_types[ret_ind];
            llvm_types[type_ind] =
              LLVMFunctionType(ret_type, &param_type, 1, false);
            break;
          }
          // TODO: There should really be a separate module-local tag for
          // combinable tags to avoid this non-totality.
          case T_CALL:
          case T_UNKNOWN:
          case T_UNIT:
          case T_I8:
          case T_U8:
          case T_I16:
          case T_U16:
          case T_I32:
          case T_U32:
          case T_I64:
          case T_U64:
          case T_BOOL: {
            give_up("Can't combine non-compound llvm type");
            break;
          }
        }
        break;
      }
    }
  }
  VEC_FREE(&actions);
  VEC_FREE(&inds);
  return llvm_types[root_type_ind];
}

static void cg_expression(cg_state *state, cg_expr_params params) {
  node_ind_t ind = params.node_ind;
  parse_node node = state->parse_tree.nodes[ind];
  stage stage = params.stage;
  switch (node.type.expression) {
    case PT_EX_CALL: {
      node_ind_t callee_ind = PT_CALL_CALLEE_IND(node);
      switch (stage) {
        case STAGE_ONE: {
          push_expression_act(state, ind, STAGE_TWO);
          // These will be in the same order on the value (out) stack
          push_expression_act(state, PT_CALL_CALLEE_IND(node), STAGE_ONE);
          push_expression_act(state, PT_CALL_PARAM_IND(node), STAGE_ONE);
          break;
        }
        case STAGE_TWO: {
          LLVMValueRef callee = VEC_POP(&state->val_stack);
          LLVMValueRef param = VEC_POP(&state->val_stack);
          LLVMTypeRef fn_type =
            construct_type(state, state->types.node_types[callee_ind]);
          LLVMValueRef res = LLVMBuildCall2(state->builder, fn_type, callee, &param, 1, "call-res");
          VEC_PUSH(&state->val_stack, res);
          break;
        }
      }
      break;
    }
    case PT_EX_LOWER_NAME:
    case PT_EX_UPPER_NAME: {
      binding b = node.span;
      node_ind_t ind = lookup_str_ref(
        state->source.data, state->env_bnds, state->env_is_builtin, b);
      // missing refs are caught in typecheck phase
      debug_assert(ind != state->env_bnds.len);
      llvm_value val = VEC_GET(state->env_vals, ind);
      VEC_PUSH(&state->val_stack, val);
      break;
    }
    case PT_EX_INT: {
      node_ind_t type_ind = state->types.node_types[ind];
      LLVMTypeRef type = construct_type(state, type_ind);
      const char *str = VEC_GET_PTR(state->source, node.span.start);
      size_t len = node.span.end - node.span.start + 1;
      VEC_PUSH(&state->val_stack,
               LLVMConstIntOfStringAndSize(type, str, len, 10));
      break;
    }
    case PT_EX_IF: {
      switch (stage) {
        case STAGE_ONE: {
          push_expression_act(state, ind, STAGE_TWO);

          // cond
          node_ind_t cond_ind = PT_IF_COND_IND(state->parse_tree.inds, node);
          push_expression_act(state, cond_ind, STAGE_ONE);

          // then
          node_ind_t a_node = PT_IF_A_IND(state->parse_tree.inds, node);
          push_expression_act(state, a_node, STAGE_TWO);
          push_gen_basic_block(state, VEC_PEEK(state->function_stack), THEN_STR);

          // else
          node_ind_t b_node = PT_IF_B_IND(state->parse_tree.inds, node);
          push_expression_act(state, b_node, STAGE_TWO);
          push_gen_basic_block(state, VEC_PEEK(state->function_stack), ELSE_STR);
          break;
        }
        case STAGE_TWO: {
          LLVMValueRef cond = VEC_POP(&state->val_stack);
          LLVMValueRef then_val = VEC_POP(&state->val_stack);
          LLVMValueRef else_val = VEC_POP(&state->val_stack);
          LLVMTypeRef res_type =
            construct_type(state, state->types.node_types[ind]);
          LLVMBasicBlockRef then_block = VEC_POP(&state->block_stack);
          LLVMBasicBlockRef else_block = VEC_POP(&state->block_stack);
          LLVMBuildCondBr(state->builder, cond, then_block, else_block);

          LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(
            state->context, VEC_PEEK(state->function_stack), "if-end");
          LLVMPositionBuilderAtEnd(state->builder, end_block);
          LLVMValueRef phi = LLVMBuildPhi(state->builder, res_type, "end-if");
          LLVMValueRef incoming_vals[2] = {then_val, else_val};
          LLVMBasicBlockRef incoming_blocks[2] = {then_block, else_block};
          LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);

          LLVMPositionBuilderAtEnd(state->builder, then_block);
          LLVMBuildBr(state->builder, end_block);

          LLVMPositionBuilderAtEnd(state->builder, else_block);
          LLVMBuildBr(state->builder, end_block);
          VEC_PUSH(&state->val_stack, phi);
          break;
        }
      }
      break;
    }
    case PT_EX_TUP: {
      switch (stage) {
        case STAGE_ONE:
          push_expression_act(state, ind, STAGE_TWO);
          push_expression_act(state, PT_TUP_SUB_A(node), STAGE_ONE);
          // processed first = pushed onto val_stack first = appears last in STAGE_TWO
          push_expression_act(state, PT_TUP_SUB_B(node), STAGE_ONE);
          break;
        case STAGE_TWO: {
          LLVMTypeRef tup_type =
            construct_type(state, state->types.node_types[ind]);
          const char *name = "tuple";
          LLVMValueRef allocated = LLVMBuildAlloca(state->builder, tup_type, name);

          LLVMValueRef left_val = VEC_POP(&state->val_stack);
          LLVMValueRef right_val = VEC_POP(&state->val_stack);

          LLVMValueRef left_ptr = LLVMBuildStructGEP2(state->builder, tup_type, allocated, 0, name);
          LLVMValueRef right_ptr = LLVMBuildStructGEP2(state->builder, tup_type, allocated, 1, name);

          LLVMBuildStore(state->builder, left_val, left_ptr);
          LLVMBuildStore(state->builder, right_val, right_ptr);

          LLVMValueRef res = LLVMBuildLoad2(state->builder, tup_type, allocated, "tup-ex");
          VEC_PUSH(&state->val_stack, res);
          break;
        }
      }
      break;
    }
    case PT_EX_FUN_BODY: {
      // TODO return value
      cg_block(state, PT_FUN_BODY_SUBS_START(node), PT_FUN_BODY_SUB_AMT(node));
      break;
    }
    case PT_EX_AS: {
      push_expression_act(state, PT_AS_VAL_IND(node), STAGE_ONE);
      break;
    }
    case PT_EX_UNIT: {
      LLVMTypeRef void_type =
        construct_type(state, state->types.node_types[ind]);
      LLVMValueRef void_val = LLVMGetUndef(void_type);
      VEC_PUSH(&state->val_stack, void_val);
      break;
    }
    case PT_EX_FN:
    case PT_EX_LIST:
    case PT_EX_STRING:
      printf("Typechecking %s nodes hasn't been implemented yet.\n",
             parse_node_string(node.type));
      exit(1);
      break;
  }
}

static llvm_function cg_emit_empty_fn(cg_state *state, node_ind_t ind, parse_node node) {
  LLVMTypeRef fn_type =
    construct_type(state, state->types.node_types[ind]);

  node_ind_t binding_ind =
    PT_FUN_BINDING_IND(state->parse_tree.inds, node);
  parse_node binding = state->parse_tree.nodes[binding_ind];
  buf_ind_t binding_len = 1 + binding.span.end - binding.span.start;

  // We should add this back at some point, I guess
  // LLVMLinkage linkage = LLVMAvailableExternallyLinkage;
  LLVMValueRef fn =
    LLVMAddFunctionCustom(state->module,
                          &state->source.data[binding.span.start],
                          binding_len,
                          fn_type);

  LLVMSetLinkage(fn, LLVMExternalLinkage);
  return fn;
}

static void cg_gen_basic_block(cg_state *state, cg_block_params params) {
  const char *name = params.name;
  llvm_function fn = params.fn;
  LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(state->context, fn, name);
  LLVMPositionBuilderAtEnd(state->builder, block);
  VEC_PUSH(&state->block_stack, block);
}

// generate the body, and schedule cleanup
static void cg_function_stage_two_internal(cg_state *state, node_ind_t node_ind, llvm_function fn, parse_node node) {
  VEC_PUSH(&state->function_stack, fn);

  u32 env_amt = state->env_bnds.len;
  push_act_pop_env_to(state, env_amt);

  push_act_fn_three(state);

  node_ind_t body_ind = PT_FUN_BODY_IND(state->parse_tree.inds, node);
  push_expression_act(state, body_ind, STAGE_ONE);

  LLVMValueRef arg = LLVMGetParam(fn, 0);
  node_ind_t param_ind = PT_FUN_PARAM_IND(state->parse_tree.inds, node);
  push_pattern_act(state, param_ind, arg);

  push_gen_basic_block(state, fn, ENTRY_STR);
}

static void cg_function_stage_two(cg_state *state, cg_fun_stage_two_params params) {
  node_ind_t ind = params.node_ind;
  parse_node node = state->parse_tree.nodes[ind];
  llvm_function fn = params.fn;
  cg_function_stage_two_internal(state, ind, fn, node);
}

static void cg_statement(cg_state *state, cg_statement_params params) {
  node_ind_t ind = params.node_ind;
  parse_node node = state->parse_tree.nodes[ind];
  stage stage = params.stage;
  switch (node.type.statement) {
    case PT_STATEMENT_LET:
      switch (stage) {
        case STAGE_ONE:
          push_statement_act(state, ind, STAGE_TWO);
          push_expression_act(state, PT_LET_VAL_IND(node), STAGE_ONE);
          break;
        case STAGE_TWO: {
          debug_assert(ind != state->env_bnds.len);
          node_ind_t binding_ind = PT_LET_BND_IND(node);
          parse_node binding = state->parse_tree.nodes[binding_ind];
          LLVMValueRef val = VEC_POP(&state->val_stack);
          push_env(state, binding.span, val);
          break;
        }
      }
      break;
    case PT_STATEMENT_SIG:
      break;
    // TODO support recursive?
    case PT_STATEMENT_FUN: {
      llvm_function fn = cg_emit_empty_fn(state, ind, node);
      cg_function_stage_two_internal(state, params.node_ind, fn, node);
      break;
    }
    default:
      // expressions are statements :(
      // TODO this is really inefficient. We should probably extract out the
      // cases into functions, and inline the calls.
      push_act_ignore_value(state);
      push_expression_act(state, ind, STAGE_ONE);
      break;
  }
}

static void cg_function_stage_three(cg_state *state) {
  LLVMValueRef ret = VEC_POP(&state->val_stack);
  VEC_POP(&state->block_stack);
  LLVMBuildRet(state->builder, ret);
  VEC_POP(&state->function_stack);
  LLVMPositionBuilderAtEnd(state->builder,
                           VEC_PEEK(state->block_stack));
}

// expression, last result is returned
static void cg_block(cg_state *state, node_ind_t start, node_ind_t amt) {
  if (amt == 0) {
    give_up("Block expression expected at least one element");
  }
  size_t last = start + amt - 1;
  node_ind_t result_ind = state->parse_tree.inds[last];
  push_expression_act(state, result_ind, STAGE_ONE);
  for (size_t i = last; i > start; i--) {
    node_ind_t sub_ind = state->parse_tree.inds[i];
    parse_node sub = state->parse_tree.nodes[sub_ind];
    if (sub.type.statement == PT_STATEMENT_SIG)
      continue;
    push_statement_act(state, sub_ind, STAGE_ONE);
  }
}

static void cg_block_recursive(cg_state *state, node_ind_t start, node_ind_t amt) {
  size_t last = start + amt - 1;
  // TODO support let bindings
  for (size_t i = 0; i < amt; i++) {
    size_t j = last - i;
    node_ind_t sub_ind = state->parse_tree.inds[j];
    parse_node node = state->parse_tree.nodes[sub_ind];
    switch (node.type.statement) {
      case PT_STATEMENT_SIG:
        break;
      case PT_STATEMENT_FUN: {
        node_ind_t binding_ind =
          PT_FUN_BINDING_IND(state->parse_tree.inds, node);
        parse_node binding = state->parse_tree.nodes[binding_ind];
        llvm_function fn = cg_emit_empty_fn(state, sub_ind, node);
        push_env(state, binding.span, fn);
        push_act_fn_two(state, fn, sub_ind);
        break;
      }
      case PT_STATEMENT_LET: {
        UNIMPLEMENTED("Top level let binding");
        break;
      }
    }
  }
}

enum pat_actions {
  CG_PAT_NODE,
};

static void cg_pattern(cg_state *state, cg_pattern_params params) {
  // TODO remove stage?
  // also, finish writing this
  node_ind_t ind = params.node_ind;
  parse_node node = state->parse_tree.nodes[ind];
  llvm_value val = params.val;

  switch (node.type.pattern) {
    case PT_PAT_TUP: {

      // LLVMTypeRef type = construct_type(state, state->types.node_types[ind]);
      llvm_value left_val = LLVMBuildExtractValue(state->builder, val, 0, "tuple-left");
      llvm_value right_val = LLVMBuildExtractValue(state->builder, val, 1, "tuple-right");
      // TODO does the struct have to be a pointer?

      node_ind_t sub_a = PT_TUP_SUB_A(node);
      node_ind_t sub_b = PT_TUP_SUB_B(node);

      push_pattern_act(state, sub_b, right_val);
      push_pattern_act(state, sub_a, left_val);
      break;
    }
    case PT_PAT_WILDCARD: {
      push_env(state, node.span, val);
      break;
    }
    case PT_PAT_UNIT:
      break;
    case PT_PAT_CONSTRUCTION:
    case PT_PAT_UPPER_NAME:
    case PT_PAT_LIST:
    case PT_PAT_INT:
    case PT_PAT_STRING:
      break;
  }
}

static void cg_pop_env_to(cg_state *state, cg_pop_env_to_params params) {
  u32 pop_to = params.amt;
  u32 pop_amt = state->env_bnds.len - pop_to;

  VEC_POP_N(&state->env_bnds.len, pop_amt);
  VEC_POP_N(&state->env_vals.len, pop_amt);
  bs_pop_n(&state->env_is_builtin, pop_amt);
}

static void cg_llvm_module(LLVMContextRef ctx, LLVMModuleRef mod,
                           source_file source, parse_tree tree,
                           type_info types) {

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  cg_state state = new_cg_state(ctx, mod, source, tree, types);

  cg_block_recursive(&state, tree.root_subs_start, tree.root_subs_amt);

  while (state.actions.len > 0) {

    // printf("Codegen action %d\n", action_num++);
    cg_action action = VEC_POP(&state.actions);
    switch (action.tag) {
      case CG_EXPR:
        cg_expression(&state, action.expr_params);
        break;
      case CG_STATEMENT:
        cg_statement(&state, action.statement_params);
        break;

      // uses:
      // * act_string
      // * node_ind (passes on to expression)
      case CG_GEN_BASIC_BLOCK:
        cg_gen_basic_block(&state, action.block_params);
        break;
      case CG_POP_ENV_TO:
        cg_pop_env_to(&state, action.pop_env_to_params);
        break;
      case CG_IGNORE_VALUE:
        VEC_POP(&state.val_stack);
        break;
      case CG_PATTERN:
        cg_pattern(&state, action.pattern_params);
        break;
      // generate body
      case CG_FUN_STAGE_TWO:
        cg_function_stage_two(&state, action.fun_stage_two_params);
        break;
      // cleanup
      case CG_FUN_STAGE_THREE:
        cg_function_stage_three(&state);
        break;
    }
  }
  destroy_cg_state(&state);
}

llvm_res gen_module(const char *module_name, source_file source, parse_tree tree,
                    type_info types, LLVMContextRef ctx) {
#ifdef TIME_CODEGEN
  struct timespec start = get_monotonic_time();
#endif
  llvm_res res = {
    .module = LLVMModuleCreateWithNameInContext(module_name, ctx),
  };
  cg_llvm_module(ctx, res.module, source, tree, types);
#ifdef TIME_CODEGEN
  res.time_taken = time_since_monotonic(start);
#endif
  return res;
}

void gen_and_print_module(source_file source, parse_tree tree, type_info types,
                          FILE *out_f) {
  LLVMContextRef ctx = LLVMContextCreate();
  llvm_res res = gen_module("my_module", source, tree, types, ctx);
  fputs(LLVMPrintModuleToString(res.module), out_f);
}
