// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

#include "binding.h"
#include "bitset.h"
#include "builtins.h"
#include "builtins.h"
#include "consts.h"
#include "llvm.h"
#include "llvm_shim.h"
#include "llvm_shim.h"
#include "parse_tree.h"
#include "timing.h"
#include "traverse.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

static const char *TUPLE_EXPRESSION_STR = "tup-expr";
static const char *TUPLE_STR = "tuple";
static const char *LAMBDA_STR = "lambda";
static const char *ENTRY_STR = "fn-entry";
// static const char *THEN_STR = "if-then";
// static const char *ELSE_STR = "if-else";

// TODO deduplicate {i,u}<bitwidth>{add,sub,mul} exogenous functions

typedef LLVMValueRef LLVMFunctionRef;

VEC_DECL_CUSTOM(LLVMValueRef, vec_llvm_value);
VEC_DECL_CUSTOM(LLVMTypeRef, vec_llvm_type);
VEC_DECL_CUSTOM(LLVMBasicBlockRef, vec_llvm_block);
VEC_DECL_CUSTOM(LLVMFunctionRef, vec_llvm_function);

typedef union {
  builtin_term builtin;
  LLVMValueRef exogenous;
} llvm_lang_value_union;

typedef struct {
  bool is_builtin;
  llvm_lang_value_union data;
} llvm_lang_value;

VEC_DECL(llvm_lang_value_union);

typedef struct {
  bitset is_builtin;
  vec_llvm_lang_value_union values;
} llvm_lang_values;

typedef struct {
  LLVMContextRef context;
  LLVMBuilderRef builder;
  LLVMModuleRef module;

  LLVMTypeRef *llvm_type_refs;
  llvm_lang_values return_values;
  llvm_lang_values environment_values;
  LLVMValueRef builtin_values[builtin_term_amount];

  vec_llvm_block block_stack;
  vec_llvm_function function_stack;

  source_file source;
  parse_tree_without_aggregates parse_tree;
  type_info types;

  const struct {
    const LLVMTypeRef ll_bool;
    const LLVMValueRef ll_true;
    const LLVMValueRef ll_false;
  } llvm_cache;
} llvm_cg_state;

static void llvm_gen_basic_block(llvm_cg_state *state, const char *name,
                                 LLVMFunctionRef fn) {
  LLVMBasicBlockRef block =
    LLVMAppendBasicBlockInContext(state->context, fn, name);
  LLVMPositionBuilderAtEnd(state->builder, block);
  VEC_PUSH(&state->block_stack, block);
}

static LLVMIntPredicate llvm_builtin_predicates[] = {
  [i8_eq_builtin] = LLVMIntEQ,    [i16_eq_builtin] = LLVMIntEQ,
  [i32_eq_builtin] = LLVMIntEQ,   [i64_eq_builtin] = LLVMIntEQ,

  [u8_eq_builtin] = LLVMIntEQ,    [u16_eq_builtin] = LLVMIntEQ,
  [u32_eq_builtin] = LLVMIntEQ,   [u64_eq_builtin] = LLVMIntEQ,

  [i8_lt_builtin] = LLVMIntSLT,   [i16_lt_builtin] = LLVMIntSLT,
  [i32_lt_builtin] = LLVMIntSLT,  [i64_lt_builtin] = LLVMIntSLT,

  [u8_lt_builtin] = LLVMIntULT,   [u16_lt_builtin] = LLVMIntULT,
  [u32_lt_builtin] = LLVMIntULT,  [u64_lt_builtin] = LLVMIntULT,

  [i8_lte_builtin] = LLVMIntSLE,  [i16_lte_builtin] = LLVMIntSLE,
  [i32_lte_builtin] = LLVMIntSLE, [i64_lte_builtin] = LLVMIntSLE,

  [u8_lte_builtin] = LLVMIntULE,  [u16_lte_builtin] = LLVMIntULE,
  [u32_lte_builtin] = LLVMIntULE, [u64_lte_builtin] = LLVMIntULE,

  [i8_gt_builtin] = LLVMIntSGT,   [i16_gt_builtin] = LLVMIntSGT,
  [i32_gt_builtin] = LLVMIntSGT,  [i64_gt_builtin] = LLVMIntSGT,

  [u8_gt_builtin] = LLVMIntUGT,   [u16_gt_builtin] = LLVMIntUGT,
  [u32_gt_builtin] = LLVMIntUGT,  [u64_gt_builtin] = LLVMIntUGT,

  [i8_gte_builtin] = LLVMIntSGE,  [i16_gte_builtin] = LLVMIntSGE,
  [i32_gte_builtin] = LLVMIntSGE, [i64_gte_builtin] = LLVMIntSGE,

  [u8_gte_builtin] = LLVMIntUGE,  [u16_gte_builtin] = LLVMIntUGE,
  [u32_gte_builtin] = LLVMIntUGE, [u64_gte_builtin] = LLVMIntUGE,
};

static LLVMValueRef llvm_cg_mod_floor(llvm_cg_state *state,
                                      LLVMValueRef dividend,
                                      LLVMValueRef divisor) {
  LLVMValueRef a = LLVMBuildSRem(state->builder, dividend, divisor, "mod a");
  LLVMValueRef b = LLVMBuildAdd(state->builder, a, divisor, "mod b");
  return LLVMBuildSRem(state->builder, b, divisor, "mod res");
}

static LLVMValueRef llvm_cg_binop(llvm_cg_state *state, builtin_term term,
                                  LLVMValueRef left, LLVMValueRef right) {
  LLVMValueRef res = NULL;
  switch (term) {
    case BUILTIN_COMPARISON_CASES: {
      LLVMIntPredicate predicate = llvm_builtin_predicates[term];
      res = LLVMBuildICmp(state->builder, predicate, left, right, "eq?");
      break;
    }
    case BUILTIN_ADD_CASES:
      res = LLVMBuildAdd(state->builder, left, right, "+");
      break;
    case BUILTIN_SUB_CASES:
      res = LLVMBuildSub(state->builder, left, right, "-");
      break;
    case BUILTIN_MUL_CASES:
      res = LLVMBuildMul(state->builder, left, right, "*");
      break;
    case BUILTIN_MOD_CASES:
      res = llvm_cg_mod_floor(state, left, right);
      break;
    case BUILTIN_IDIV_CASES:
      res = LLVMBuildSDiv(state->builder, left, right, "/");
      break;
    case BUILTIN_UDIV_CASES:
      res = LLVMBuildUDiv(state->builder, left, right, "/");
      break;
    case BUILTIN_IREM_CASES:
      res = LLVMBuildSRem(state->builder, left, right, "rem");
      break;
    case BUILTIN_UREM_CASES:
      res = LLVMBuildURem(state->builder, left, right, "rem");
      break;
    case BUILTIN_NON_FUNCTION_CASES:
      // TODO mark unreachable
      break;
  }
  return res;
}

static LLVMTypeRef llvm_construct_type(llvm_cg_state *state,
                                       node_ind_t root_type_ind) {
  {
    LLVMTypeRef prev = state->llvm_type_refs[root_type_ind];
    if (prev != NULL)
      return prev;
  }
  return llvm_construct_type_internal(
    state->llvm_type_refs, state->types.tree, state->context, root_type_ind);
}

static LLVMTypeRef llvm_construct_passable_type(llvm_cg_state *state,
                                                node_ind_t root_type_ind) {
  LLVMTypeRef res = llvm_construct_type(state, root_type_ind);
  type t = state->types.tree.nodes[root_type_ind];
  switch (t.tag.normal) {
    case T_FN:
      return LLVMPointerType(res, 0);
    default:
      return res;
  }
}

static LLVMValueRef llvm_builtin_to_value(llvm_cg_state *state,
                                          builtin_term term) {
  LLVMValueRef res = state->builtin_values[term];
  if (res != NULL)
    goto ret;

  switch (term) {
    case true_builtin:
      res = state->llvm_cache.ll_true;
      goto cache_and_ret;
    case false_builtin:
      res = state->llvm_cache.ll_false;
      goto cache_and_ret;
    case BUILTIN_FUNCTION_CASES:
    case builtin_term_amount:
      break;
  }

  LLVMTypeRef type_ref = llvm_construct_type(state, builtin_type_inds[term]);
  LLVMValueRef fn =
    LLVMAddFunction(state->module, builtin_term_names[term], type_ref);
  VEC_PUSH(&state->function_stack, fn);

  llvm_gen_basic_block(state, ENTRY_STR, fn);

  switch (term) {
    case BUILTIN_BINOP_CASES: {
      LLVMValueRef left = LLVMGetParam(fn, 0);
      LLVMValueRef right = LLVMGetParam(fn, 1);
      res = llvm_cg_binop(state, term, left, right);
      break;
    }
    // TODO add unary operations
    case BUILTIN_NON_FUNCTION_CASES:
      // TODO mark unreachable
      break;
  }

  LLVMBuildRet(state->builder, res);

  VEC_POP_(&state->block_stack);
  LLVMPositionBuilderAtEnd(state->builder, VEC_PEEK(state->block_stack));
  VEC_POP_(&state->function_stack);
  // res = LLVMConstBitCast(fn, LLVMPointerType(LLVMInt8Type(), 0));
  res = fn;
cache_and_ret:
  state->builtin_values[term] = res;
ret:
  return res;
}

static void llvm_push_exogenous_value(llvm_lang_values *values,
                                      LLVMValueRef val) {
  bs_push(&values->is_builtin, false);
  llvm_lang_value_union l = {
    .exogenous = val,
  };
  VEC_PUSH(&values->values, l);
}

static void llvm_push_builtin(llvm_lang_values *values, builtin_term builtin) {
  llvm_lang_value_union value_union = {
    .builtin = builtin,
  };
  VEC_PUSH(&values->values, value_union);
  bs_push_true(&values->is_builtin);
}

static void llvm_push_value(llvm_lang_values *values, llvm_lang_value value) {
  bs_push(&values->is_builtin, value.is_builtin);
  VEC_PUSH(&values->values, value.data);
}

static llvm_lang_value llvm_pop_value(llvm_lang_values *values) {
  debug_assert(values->values.len > 0);
  debug_assert(values->is_builtin.len > 0);
  llvm_lang_value res = {
    .is_builtin = bs_pop(&values->is_builtin),
  };
  VEC_POP(&values->values, &res.data);
  return res;
}

static LLVMValueRef llvm_to_exogenous_value(llvm_cg_state *state,
                                            llvm_lang_value value) {
  return value.is_builtin ? llvm_builtin_to_value(state, value.data.builtin)
                          : value.data.exogenous;
}

static LLVMValueRef llvm_pop_exogenous_value(llvm_cg_state *state,
                                             llvm_lang_values *values) {
  llvm_lang_value value = llvm_pop_value(values);
  return llvm_to_exogenous_value(state, value);
}

static llvm_lang_value llvm_get_value(llvm_lang_values values,
                                      environment_ind_t ind) {
  debug_assert(values.values.len > ind);
  debug_assert(values.is_builtin.len > ind);
  llvm_lang_value res = {
    .is_builtin = bs_get(values.is_builtin, ind),
    .data = VEC_GET(values.values, ind),
  };
  return res;
}

static llvm_lang_value llvm_get_environment(llvm_cg_state *state,
                                            environment_ind_t ind) {
  return llvm_get_value(state->environment_values, ind);
}

/*
static void llvm_set_value(llvm_lang_values values, environment_ind_t ind,
llvm_lang_value value) { bs_set(values.is_builtin, value.is_builtin);
  VEC_SET(values.values, ind, value.data);
}

static void llvm_set_environment(llvm_cg_state *state, environment_ind_t ind,
llvm_lang_value value) { debug_assert(values->values.length > ind);
  debug_assert(values->is_builtin.length > ind);
  llvm_set_value(state->environment_values, ind, value);
}
*/

static void free_lang_values(llvm_lang_values *values) {
  VEC_FREE(&values->values);
  bs_free(&values->is_builtin);
}

static void destroy_cg_state(llvm_cg_state *state) {
  LLVMDisposeBuilder(state->builder);
  free(state->llvm_type_refs);
  free_lang_values(&state->return_values);
  free_lang_values(&state->environment_values);
  VEC_FREE(&state->block_stack);
  VEC_FREE(&state->function_stack);
}

static llvm_lang_values llvm_empty_lang_values(void) {
  llvm_lang_values res = {
    .is_builtin = bs_new(),
    .values = VEC_NEW,
  };
  return res;
}

static llvm_cg_state llvm_new_cg_state(LLVMContextRef ctx, LLVMModuleRef mod,
                                       source_file source, parse_tree tree,
                                       type_info types) {
  LLVMTypeRef bool_type = LLVMInt1TypeInContext(ctx);
  llvm_cg_state state = {
    .context = ctx,
    .builder = LLVMCreateBuilderInContext(ctx),
    .module = mod,

    .llvm_type_refs =
      (LLVMTypeRef *)calloc(types.type_amt, sizeof(LLVMTypeRef)),
    .return_values = llvm_empty_lang_values(),
    .environment_values = llvm_empty_lang_values(),
    .builtin_values = {NULL},

    .block_stack = VEC_NEW,
    .function_stack = VEC_NEW,

    .source = source,
    .parse_tree = tree.data,
    .types = types,

    .llvm_cache =
      {
        .ll_bool = bool_type,
        .ll_true = LLVMConstInt(bool_type, 1, false),
        .ll_false = LLVMConstInt(bool_type, 0, false),
      },
  };

  // Sometimes we want to be nowhere
  VEC_PUSH(&state.block_stack, NULL);
  return state;
}

static void llvm_init_builtins(llvm_cg_state *state) {
  llvm_lang_values *values = &state->environment_values;
  for (builtin_term builtin_term = 0; builtin_term < builtin_term_amount;
       builtin_term++) {
    llvm_push_builtin(values, builtin_term);
  }
}

typedef struct {
  LLVMValueRef left;
  LLVMValueRef right;
} tuple_parts;

static tuple_parts llvm_eliminate_tuple(llvm_cg_state *state,
                                        LLVMValueRef tuple) {
  tuple_parts res = {
    .left = LLVMBuildExtractValue(state->builder, tuple, 0, "tuple-left"),
    .right = LLVMBuildExtractValue(state->builder, tuple, 1, "tuple-right"),
  };
  return res;
}

static void llvm_cg_visit_in(llvm_cg_state *state, traversal_node_data data) {
  switch (data.node.type.all) {
    case PT_ALL_EX_CALL:
      break;
    case PT_ALL_EX_FN: {
      LLVMTypeRef fn_type =
        llvm_construct_type(state, state->types.node_types[data.node_index]);
      // We should add this back at some point, I guess
      // LLVMLinkage linkage = LLVMAvailableExternallyLinkage;
      LLVMValueRef fn = LLVMAddFunction(state->module, LAMBDA_STR, fn_type);
      VEC_PUSH(&state->function_stack, fn);
      llvm_gen_basic_block(state, ENTRY_STR, fn);
      break;
    }
    case PT_ALL_EX_FUN_BODY:
      break;
    case PT_ALL_EX_IF:
      break;
    case PT_ALL_EX_INT:
      break;
    case PT_ALL_EX_AS:
      break;
    case PT_ALL_EX_TERM_NAME:
      break;
    case PT_ALL_EX_UPPER_NAME:
      break;
    case PT_ALL_EX_LIST:
      break;
    case PT_ALL_EX_STRING:
      break;
    case PT_ALL_EX_TUP:
      break;
    case PT_ALL_EX_UNIT:
      break;

    case PT_ALL_MULTI_TERM_NAME:
      break;
    case PT_ALL_MULTI_TYPE_PARAMS: {
      break;
    }
    case PT_ALL_MULTI_TYPE_PARAM_NAME:
      break;
    case PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME:
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME:
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
      break;
    case PT_ALL_MULTI_DATA_CONSTRUCTORS:
      break;

    case PT_ALL_PAT_WILDCARD:
      // the thing on the stack will be pushed to the environment
      break;
    case PT_ALL_PAT_TUP: {
      LLVMValueRef val = llvm_pop_exogenous_value(state, &state->return_values);
      tuple_parts parts = llvm_eliminate_tuple(state, val);
      llvm_push_exogenous_value(&state->return_values, parts.right);
      llvm_push_exogenous_value(&state->return_values, parts.left);
      break;
    }
    case PT_ALL_PAT_UNIT:
      llvm_pop_value(&state->return_values);
      break;
    case PT_ALL_PAT_DATA_CONSTRUCTOR_NAME:
      // TODO
      give_up("Destructureing user-defined data constructors is not "
              "implemented yet.");
      break;
    case PT_ALL_PAT_CONSTRUCTION:
      // TODO
      give_up("Destructureing user-defined data constructors is not "
              "implemented yet.");
      break;
    case PT_ALL_PAT_STRING:
      break;
    case PT_ALL_PAT_INT:
      break;
    case PT_ALL_PAT_LIST: {
      LLVMValueRef val = llvm_pop_exogenous_value(state, &state->return_values);
      for (node_ind_t i = 0; i < PT_LIST_SUB_AMT(data.node); i++) {
        LLVMValueRef elem =
          LLVMBuildExtractValue(state->builder, val, i, "list-pat-elem");
        llvm_push_exogenous_value(&state->return_values, elem);
      }
      break;
    }

    case PT_ALL_STATEMENT_SIG:
      break;
    case PT_ALL_STATEMENT_FUN: {
      const node_ind_t binding_ind =
        PT_FUN_BINDING_IND(state->parse_tree.inds, data.node);
      const parse_node binding = state->parse_tree.nodes[binding_ind];
      // we've already predeclared this
      LLVMFunctionRef fn = VEC_GET(state->environment_values.values,
                                   binding.data.var_data.variable_index)
                             .exogenous;
      VEC_PUSH(&state->function_stack, fn);
      llvm_gen_basic_block(state, ENTRY_STR, fn);
      for (node_ind_t i = 0; i < PT_LIST_SUB_AMT(data.node); i++) {
        node_ind_t j = PT_LIST_SUB_AMT(data.node) - 1 - i;
        LLVMValueRef param = LLVMGetParam(fn, j);
        llvm_push_exogenous_value(&state->return_values, param);
      }
      break;
    }
    case PT_ALL_STATEMENT_LET:
      break;
    case PT_ALL_STATEMENT_DATA_DECLARATION:
      break;

    case PT_ALL_TY_CONSTRUCTION:
      break;
    case PT_ALL_TY_LIST:
      break;
    case PT_ALL_TY_FN:
      break;
    case PT_ALL_TY_TUP:
      break;
    case PT_ALL_TY_UNIT:
      break;
    case PT_ALL_TY_PARAM_NAME:
      break;
    case PT_ALL_TY_CONSTRUCTOR_NAME:
      break;

    default:
      break;
  }
}

static void LLVMPositionBuilderAtStart(LLVMBuilderRef builder,
                                       LLVMBasicBlockRef block) {
  LLVMValueRef first_instruction = LLVMGetFirstInstruction(block);
  if (HEDLEY_UNLIKELY(first_instruction == NULL)) {
    LLVMPositionBuilderAtEnd(builder, block);
  } else {
    LLVMPositionBuilderBefore(builder, first_instruction);
  }
}

static LLVMValueRef llvm_gen_alloca_at_function_start(llvm_cg_state *state,
                                                      const char *restrict name,
                                                      LLVMTypeRef type) {
  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(state->builder);
  LLVMBuilderRef builder = state->builder;
  LLVMFunctionRef current_function = VEC_PEEK(state->function_stack);
  LLVMBasicBlockRef entry_block = LLVMGetEntryBasicBlock(current_function);
  LLVMPositionBuilderAtStart(builder, entry_block);
  LLVMValueRef res = LLVMBuildAlloca(builder, type, name);
  LLVMPositionBuilderAtEnd(builder, current_block);
  return res;
}

static void llvm_cg_visit_out(llvm_cg_state *state, traversal_node_data data) {
  switch (data.node.type.all) {
    case PT_ALL_EX_CALL: {
      node_ind_t callee_ind =
        PT_CALL_CALLEE_IND(state->parse_tree.inds, data.node);
      node_ind_t callee_type_ind = state->types.node_types[callee_ind];
      type callee_type = state->types.tree.nodes[callee_type_ind];
      LLVMTypeRef fn_type =
        llvm_construct_type(state, state->types.node_types[callee_ind]);
      node_ind_t param_amt = T_FN_PARAM_AMT(callee_type);
      size_t n_param_ref_bytes = sizeof(LLVMValueRef) * param_amt;
      LLVMValueRef *llvm_params = stalloc(n_param_ref_bytes);
      for (node_ind_t i = 0; i < param_amt; i++) {
        llvm_params[param_amt - 1 - i] =
          llvm_pop_exogenous_value(state, &state->return_values);
      }
      llvm_lang_value callee = llvm_pop_value(&state->return_values);
      LLVMValueRef res;
      if (callee.is_builtin) {
        res = llvm_cg_binop(
          state, callee.data.builtin, llvm_params[0], llvm_params[1]);
      } else {
        res = LLVMBuildCall2(state->builder,
                             fn_type,
                             callee.data.exogenous,
                             llvm_params,
                             param_amt,
                             "call-res");
      }
      llvm_push_exogenous_value(&state->return_values, res);
      break;
    }
    case PT_ALL_EX_FUN_BODY: {
      // Return the last expression's value, which is already on the return
      // stack
      break;
    }
    case PT_ALL_EX_IF: {

      LLVMBasicBlockRef else_block;
      VEC_POP(&state->block_stack, &else_block);
      LLVMBasicBlockRef then_block;
      VEC_POP(&state->block_stack, &then_block);

      llvm_lang_value else_val = llvm_pop_value(&state->return_values);
      llvm_lang_value then_val = llvm_pop_value(&state->return_values);
      llvm_lang_value cond_val = llvm_pop_value(&state->return_values);

      if (HEDLEY_UNLIKELY(cond_val.is_builtin)) {
        LLVMBasicBlockRef block_to_delete;
        LLVMBasicBlockRef block_to_branch_to;
        llvm_lang_value value_to_return;
        // TODO remove condition instructions?
        if (cond_val.data.builtin == true_builtin) {
          block_to_branch_to = then_block;
          block_to_delete = else_block;
          value_to_return = then_val;
        } else {
          block_to_branch_to = else_block;
          block_to_delete = then_block;
          value_to_return = else_val;
        }

        LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(
          state->context, VEC_PEEK(state->function_stack), "const-if-end");

        // back to the original block
        LLVMPositionBuilderAtEnd(state->builder, VEC_PEEK(state->block_stack));
        LLVMBuildBr(state->builder, block_to_branch_to);

        LLVMPositionBuilderAtEnd(state->builder, block_to_branch_to);
        LLVMBuildBr(state->builder, end_block);

        LLVMDeleteBasicBlock(block_to_delete);
        llvm_push_value(&state->return_values, value_to_return);
        LLVMPositionBuilderAtEnd(state->builder, end_block);
        break;
      }

      LLVMValueRef cond_ex_val = llvm_to_exogenous_value(state, cond_val);
      LLVMValueRef then_ex_val = llvm_to_exogenous_value(state, then_val);
      LLVMValueRef else_ex_val = llvm_to_exogenous_value(state, else_val);

      // back to the original block
      LLVMPositionBuilderAtEnd(state->builder, VEC_PEEK(state->block_stack));
      LLVMBuildCondBr(state->builder, cond_ex_val, then_block, else_block);

      LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(
        state->context, VEC_PEEK(state->function_stack), "if-end");

      *VEC_PEEK_PTR(state->block_stack) = end_block;

      LLVMPositionBuilderAtEnd(state->builder, else_block);
      LLVMBuildBr(state->builder, end_block);

      LLVMPositionBuilderAtEnd(state->builder, then_block);
      LLVMBuildBr(state->builder, end_block);

      LLVMTypeRef res_type = llvm_construct_passable_type(
        state, state->types.node_types[data.node_index]);

      LLVMPositionBuilderAtEnd(state->builder, end_block);
      LLVMValueRef phi = LLVMBuildPhi(state->builder, res_type, "if-result");
      LLVMValueRef incoming_vals[2] = {then_ex_val, else_ex_val};
      LLVMBasicBlockRef incoming_blocks[2] = {then_block, else_block};
      LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);
      llvm_push_exogenous_value(&state->return_values, phi);

      break;
    }
    case PT_ALL_EX_INT: {
      node_ind_t type_ind = state->types.node_types[data.node_index];
      LLVMTypeRef type = llvm_construct_type(state, type_ind);
      span span = state->parse_tree.spans[data.node_index];
      const char *str = VEC_GET_PTR(state->source, span.start);
      size_t len = span.len;
      llvm_push_exogenous_value(
        &state->return_values, LLVMConstIntOfStringAndSize(type, str, len, 10));
      break;
    }
    case PT_ALL_STATEMENT_ABI:
    case PT_ALL_EX_AS: {
      break;
    }
    case PT_ALL_EX_UPPER_NAME:
    case PT_ALL_EX_TERM_NAME: {
      node_ind_t ind = data.node.data.var_data.variable_index;
      debug_assert(ind != state->environment_values.is_builtin.len);
      llvm_lang_value val = llvm_get_environment(state, ind);
      llvm_push_value(&state->return_values, val);
      break;
    }
    case PT_ALL_EX_LIST:
    case PT_ALL_EX_STRING:
      // TODO implement
      break;
    case PT_ALL_EX_TUP: {

      node_ind_t ind = data.node_index;
      LLVMTypeRef tup_type =
        llvm_construct_type(state, state->types.node_types[ind]);
      LLVMValueRef allocated =
        llvm_gen_alloca_at_function_start(state, TUPLE_STR, tup_type);

      LLVMValueRef left_val =
        llvm_pop_exogenous_value(state, &state->return_values);
      LLVMValueRef right_val =
        llvm_pop_exogenous_value(state, &state->return_values);

      LLVMValueRef left_ptr =
        LLVMBuildStructGEP2(state->builder, tup_type, allocated, 0, TUPLE_STR);
      LLVMValueRef right_ptr =
        LLVMBuildStructGEP2(state->builder, tup_type, allocated, 1, TUPLE_STR);

      LLVMBuildStore(state->builder, left_val, left_ptr);
      LLVMBuildStore(state->builder, right_val, right_ptr);

      LLVMValueRef res = LLVMBuildLoad2(
        state->builder, tup_type, allocated, TUPLE_EXPRESSION_STR);
      llvm_push_exogenous_value(&state->return_values, res);
      break;
    }

    case PT_ALL_EX_UNIT: {
      LLVMTypeRef void_type =
        llvm_construct_type(state, state->types.node_types[data.node_index]);
      LLVMValueRef void_val = LLVMGetUndef(void_type);
      llvm_push_exogenous_value(&state->return_values, void_val);
      break;
    }

    case PT_MULTI_CASES:
      break;
    case PT_PATTERN_CASES:
      break;

    case PT_ALL_STATEMENT_SIG:
      break;
    case PT_ALL_STATEMENT_FUN: {
      const LLVMValueRef ret =
        llvm_pop_exogenous_value(state, &state->return_values);
      LLVMBuildRet(state->builder, ret);
      VEC_POP_(&state->block_stack);
      VEC_POP_(&state->function_stack);
      LLVMPositionBuilderAtEnd(state->builder, VEC_PEEK(state->block_stack));
      break;
    }

    case PT_ALL_STATEMENT_LET:
      // return_value stays on the stack, to be dealt with in PUSH_VAR
      break;

    case PT_ALL_STATEMENT_DATA_DECLARATION:
      // TODO implement
      break;

    case PT_TYPE_CASES:
      break;

    case PT_ALL_EX_FN: {
      // TODO implement
      break;
    }
  }
}

static node_ind_t llvm_get_statement_binding_ind(const node_ind_t *inds,
                                                 parse_node node) {
  switch (node.type.statement) {
    case PT_STATEMENT_FUN:
      return PT_FUN_BINDING_IND(inds, node);
    case PT_STATEMENT_LET:
      return PT_LET_BND_IND(node);
    default:
      return 0;
  }
}

static void llvm_cg_predeclare_fn(llvm_cg_state *state,
                                  traversal_node_data data) {
  const node_ind_t binding_ind =
    llvm_get_statement_binding_ind(state->parse_tree.inds, data.node);
  const type_ref type_ref = state->types.node_types[data.node_index];
  const LLVMTypeRef fn_type = llvm_construct_type(state, type_ref);
  span span = state->parse_tree.spans[binding_ind];

  // We should add this back at some point, I guess
  // LLVMLinkage linkage = LLVMAvailableExternallyLinkage;
  const LLVMFunctionRef fn = LLVMAddFunctionCustom(
    state->module, &state->source.data[span.start], span.len, fn_type);
  LLVMSetLinkage(fn, LLVMExternalLinkage);
  llvm_push_exogenous_value(&state->environment_values, fn);
}

static void llvm_cg_push_scope(llvm_cg_state *state) {
  llvm_lang_value val = llvm_pop_value(&state->return_values);
  llvm_push_value(&state->environment_values, val);
}

static void llvm_cg_module(LLVMContextRef ctx, LLVMModuleRef mod,
                           source_file source, parse_tree tree,
                           type_info types) {

  llvm_cg_state state = llvm_new_cg_state(ctx, mod, source, tree, types);
  llvm_init_builtins(&state);

  pt_traversal traversal = pt_walk(tree, TRAVERSE_CODEGEN);

  while (true) {
    pt_traverse_elem elem = pt_walk_next(&traversal);
    switch (elem.action) {
      case TR_NEW_BLOCK:
        llvm_gen_basic_block(&state, "block", VEC_PEEK(state.function_stack));
        continue;
      case TR_END:
        break;
      case TR_PREDECLARE_FN:
        llvm_cg_predeclare_fn(&state, elem.data.node_data);
        continue;
      case TR_PUSH_SCOPE_VAR:
        llvm_cg_push_scope(&state);
        continue;
      case TR_VISIT_IN:
        llvm_cg_visit_in(&state, elem.data.node_data);
        continue;
      case TR_VISIT_OUT:
        llvm_cg_visit_out(&state, elem.data.node_data);
        continue;
      case TR_POP_TO:
        continue;
      case TR_LINK_SIG:
        continue;
    }
    break;
  }

  destroy_cg_state(&state);
}

void llvm_init(void) {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
}

llvm_res llvm_gen_module(source_file source, parse_tree tree, type_info types,
                         LLVMModuleRef module) {
#ifdef TIME_CODEGEN
  perf_state perf_state = perf_start();
#endif
  llvm_res res = {
    .module = module,
  };
  llvm_cg_module(LLVMGetModuleContext(module), res.module, source, tree, types);
  LLVMVerifyModule(res.module, LLVMPrintMessageAction, NULL);
  // char *mod_str = LLVMPrintModuleToString(res.module);
  // puts(mod_str);
  // free(mod_str);
  // fputs(err, stderr);
#ifdef TIME_CODEGEN
  res.perf_values = perf_end(perf_state);
#endif
  return res;
}

void llvm_gen_and_print_module(source_file source, parse_tree tree,
                               type_info types, FILE *out_f) {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("repl", ctx);
  llvm_res res = llvm_gen_module(source, tree, types, module);
  char *mod_str = LLVMPrintModuleToString(res.module);
  fputs(mod_str, out_f);
  LLVMDisposeMessage(mod_str);
  LLVMDisposeModule(module);
  LLVMContextDispose(ctx);
}
