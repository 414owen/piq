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
#include "scope.h"
#include "traverse.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

static const char *LAMBDA_STR = "lambda";
static const char *ENTRY_STR = "fn-entry";
static const char *THEN_STR = "if-then";
static const char *ELSE_STR = "if-else";

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
  parse_tree parse_tree;
  type_info types;
} llvm_cg_state;

struct llvm_globals {
  LLVMTypeRef bool_type;
  LLVMValueRef true_value;
  LLVMValueRef false_value;
};

struct llvm_globals llvm_globals;

static void llvm_init(void) {
  llvm_globals.bool_type = LLVMInt1Type();
  llvm_globals.true_value = LLVMConstInt(llvm_globals.bool_type, 1, false);
  llvm_globals.false_value = LLVMConstInt(llvm_globals.bool_type, 0, false);
}

static void llvm_gen_basic_block(llvm_cg_state *state, const char *name, LLVMFunctionRef fn) {
  LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(state->context, fn, name);
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

static LLVMValueRef llvm_cg_mod_floor(llvm_cg_state *state, LLVMValueRef dividend,
                                 LLVMValueRef divisor) {
  LLVMValueRef a = LLVMBuildSRem(state->builder, dividend, divisor, "mod a");
  LLVMValueRef b = LLVMBuildAdd(state->builder, a, divisor, "mod b");
  return LLVMBuildSRem(state->builder, b, divisor, "mod res");
}

static LLVMValueRef llvm_cg_binop(llvm_cg_state *state, builtin_term term, LLVMValueRef left, LLVMValueRef right) {
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

static LLVMTypeRef llvm_construct_type(llvm_cg_state *state, node_ind_t root_type_ind) {
  {
    LLVMTypeRef prev = state->llvm_type_refs[root_type_ind];
    if (prev != NULL)
      return prev;
  }
  return llvm_construct_type_internal(state->llvm_type_refs, state->types.tree, state->context, root_type_ind);
}

static LLVMValueRef llvm_builtin_to_value(llvm_cg_state *state, builtin_term term) {
  LLVMValueRef res = state->builtin_values[term];
  if (res != NULL)
    goto ret;

  switch (term) {
    case true_builtin:
      res = llvm_globals.true_value;
      goto cache_and_ret;
    case false_builtin:
      res = llvm_globals.false_value;
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
  res =
    LLVMConstBitCast(fn, LLVMPointerType(LLVMInt8Type(), 0));
cache_and_ret:
  state->builtin_values[term] = res;
ret:
  return res;
}

static void llvm_push_exogenous_value(llvm_lang_values *values, LLVMValueRef val) {
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

static llvm_lang_value llvm_pop_value(llvm_lang_values *values) {
  debug_assert(values->values.length > 0);
  debug_assert(values->is_builtin.length > 0);
  llvm_lang_value res = {
    .is_builtin = bs_pop(&values->is_builtin),
  };
  VEC_POP(&values->values, &res.data);
  return res;
}

static LLVMValueRef llvm_pop_exogenous_value(llvm_cg_state *state, llvm_lang_values *values) {
  llvm_lang_value value = llvm_pop_value(values);
  if (value.is_builtin) {
    return llvm_builtin_to_value(state, value.is_builtin);
  } else {
    return value.data.exogenous;
  }
}

static llvm_lang_value llvm_get_value(llvm_lang_values values, node_ind_t ind) {
  debug_assert(values->values.length > ind);
  debug_assert(values->is_builtin.length > ind);
  llvm_lang_value res = {
    .is_builtin = bs_get(values.is_builtin, ind),
    .data = VEC_GET(values.values, ind),
  };
  return res;
}

static llvm_lang_value llvm_get_environment(llvm_cg_state *state, node_ind_t ind) {
  return llvm_get_value(state->environment_values, ind);
}

static void destroy_cg_state(llvm_cg_state *state) {
  LLVMDisposeBuilder(state->builder);
  VEC_FREE(&state->block_stack);
  VEC_FREE(&state->function_stack);
}

static llvm_cg_state llvm_new_cg_state(LLVMContextRef ctx, LLVMModuleRef mod,
                                       source_file source, parse_tree tree, type_info types) {
  llvm_cg_state state = {
    .context = ctx,
    .builder = LLVMCreateBuilderInContext(ctx),
    .module = mod,

    .llvm_type_refs = (LLVMTypeRef *)calloc(types.type_amt, sizeof(LLVMTypeRef)),
    .return_values = {0},
    .environment_values = {0},
    .builtin_values = {NULL},

    .block_stack = VEC_NEW,
    .function_stack = VEC_NEW,

    .source = source,
    .parse_tree = tree,
    .types = types,
  };

  // Sometimes we want to be nowhere
  VEC_PUSH(&state.block_stack, NULL);
  return state;
}

static void llvm_init_builtins(llvm_cg_state *state) {
  llvm_lang_values *values = &state->environment_values;
  for (builtin_term builtin_term = 0; builtin_term < builtin_term_amount; builtin_term++) {
    llvm_push_builtin(values, builtin_term);
  }
}

static void llvm_cg_visit_in(llvm_cg_state *state, traversal_node_data data) {
  switch (data.node.type.all) {
    case PT_ALL_STATEMENT_FUN: {
      const node_ind_t binding_ind = PT_FUN_BINDING_IND(state->parse_tree.inds, data.node);
      const parse_node binding = state->parse_tree.nodes[binding_ind];
      LLVMFunctionRef fn;
      if (binding.variable_index < state->environment_values.values.len) {
        // we've already predeclared this
        fn = VEC_GET(state->environment_values.values, binding.variable_index).exogenous;
      } else {
        LLVMTypeRef fn_type = llvm_construct_type(state, state->types.node_types[data.node_index]);
        buf_ind_t binding_len = binding.span.len;

        // We should add this back at some point, I guess
        // LLVMLinkage linkage = LLVMAvailableExternallyLinkage;
        fn = LLVMAddFunctionCustom(state->module,
                                &state->source.data[binding.span.start],
                                binding_len,
                                fn_type);

        LLVMSetLinkage(fn, LLVMExternalLinkage);
      }
      VEC_PUSH(&state->function_stack, fn);
      llvm_gen_basic_block(state, ENTRY_STR, fn);
      break;
    case PT_ALL_EX_FN: {
      LLVMTypeRef fn_type = llvm_construct_type(state, state->types.node_types[data.node_index]);
      // We should add this back at some point, I guess
      // LLVMLinkage linkage = LLVMAvailableExternallyLinkage;
      LLVMValueRef fn = LLVMAddFunction(state->module, LAMBDA_STR, fn_type);
      VEC_PUSH(&state->function_stack, fn);
      llvm_gen_basic_block(state, ENTRY_STR, fn);
      break;
    }
  }
}

static void llvm_cg_visit_out(llvm_cg_state *state, traversal_node_data data) {
  switch (data.node.type.all) {
    case PT_ALL_EX_CALL: {
      node_ind_t callee_ind = PT_CALL_CALLEE_IND(state->parse_tree.inds, data.node);
      node_ind_t callee_type_ind = state->types.node_types[callee_ind];
      type callee_type = state->types.tree.nodes[callee_type_ind];
      LLVMTypeRef fn_type = llvm_construct_type(state, state->types.node_types[callee_ind]);
      node_ind_t param_amt = T_FN_PARAM_AMT(callee_type);
      size_t n_param_ref_bytes = sizeof(LLVMValueRef) * param_amt;
      LLVMValueRef *llvm_params = stalloc(n_param_ref_bytes);
      for (node_ind_t i = 0; i < param_amt; i++) {
        llvm_params[param_amt - 1 - i] = llvm_pop_exogenous_value(state, &state->return_values);
      }
      llvm_lang_value callee = llvm_pop_value(&state->return_values);
      LLVMValueRef res;
      if (callee.is_builtin) {
        res = llvm_cg_binop(state, callee.data.builtin, llvm_params[0], llvm_params[1]);
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
      // Return the last expression's value, which is already on the return stack
      break;
    }
    case PT_ALL_EX_IF: {

      break;
    }
    case PT_ALL_EX_INT:
    case PT_ALL_EX_AS:
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_EX_UPPER_NAME:
    case PT_ALL_EX_LIST:
    case PT_ALL_EX_STRING:
    case PT_ALL_EX_TUP:
    case PT_ALL_EX_UNIT:

    case PT_ALL_MULTI_TERM_NAME:
    case PT_ALL_MULTI_TYPE_PARAMS:
    case PT_ALL_MULTI_TYPE_PARAM_NAME:
    case PT_ALL_MULTI_TYPE_CONSTRUCTOR_NAME:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_NAME:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
    case PT_ALL_MULTI_DATA_CONSTRUCTORS:

    case PT_ALL_PAT_WILDCARD:
    case PT_ALL_PAT_TUP:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_PAT_DATA_CONSTRUCTOR_NAME:
    case PT_ALL_PAT_CONSTRUCTION:
    case PT_ALL_PAT_STRING:
    case PT_ALL_PAT_INT:
    case PT_ALL_PAT_LIST:

    case PT_ALL_STATEMENT_SIG:
    case PT_ALL_STATEMENT_FUN:
    case PT_ALL_STATEMENT_LET:
    case PT_ALL_STATEMENT_DATA_DECLARATION:

    case PT_ALL_TY_CONSTRUCTION:
    case PT_ALL_TY_LIST:
    case PT_ALL_TY_FN:
    case PT_ALL_TY_TUP:
    case PT_ALL_TY_UNIT:
    case PT_ALL_TY_PARAM_NAME:
    case PT_ALL_TY_CONSTRUCTOR_NAME:

    case PT_ALL_LEN:
    case PT_ALL_EX_FN: {
      printf("Code generation for %s nodes via LLVM hasn't been implemented yet.\n",
             parse_node_strings[data.node.type.all]);
      exit(1);
      break;
    }
  }
}

static void llvm_cg_module(LLVMContextRef ctx, LLVMModuleRef mod,
                           source_file source, parse_tree tree, type_info types) {

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  llvm_cg_state state = llvm_new_cg_state(ctx, mod, source, tree, types);
  llvm_init_builtins(&state);

  pt_traversal traversal = pt_scoped_traverse(tree, TRAVERSE_CODEGEN);
  pt_traverse_elem elem = pt_scoped_traverse_next(&traversal);

  while (true) {
    switch (elem.action) {
      case TR_END:
        break;
      case TR_PUSH_SCOPE_VAR:
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

llvm_res llvm_gen_module(const char *module_name, source_file source,
                         parse_tree tree, type_info types, LLVMContextRef ctx) {
#ifdef TIME_CODEGEN
  struct timespec start = get_monotonic_time();
#endif
  if (llvm_globals.bool_type == NULL) {
    llvm_init();
  }
  llvm_res res = {
    .module = LLVMModuleCreateWithNameInContext(module_name, ctx),
  };
  char *err;
  bool broken = LLVMVerifyModule(res.module, LLVMPrintMessageAction, &err);
  fputs(err, stderr);
  llvm_cg_module(ctx, res.module, tree, types);
  free(err);
  if (broken) {
    exit(1);
  }
#ifdef TIME_CODEGEN
  res.time_taken = time_since_monotonic(start);
#endif
  return res;
}

void gen_and_print_module(source_file source, parse_tree tree, type_info types,
                          FILE *out_f) {
  LLVMContextRef ctx = LLVMContextCreate();
  llvm_res res = llvm_gen_module("my_module", source, tree, types, ctx);
  fputs(LLVMPrintModuleToString(res.module), out_f);
  LLVMContextDispose(ctx);
}
