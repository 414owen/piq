#include <stdio.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

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

static const char *EMPTY_STR = "entry";
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

typedef enum {

  // INPUTS: node
  // OUTPUTS: val
  CG_NODE,

  // INPUTS: node, val
  // OUTPUTS: edits the environment
  CG_PATTERN,

  // INPUTS: node, str
  // OUTPUTS: block, val
  CG_GEN_BLOCK,

  // INPUTS: none
  // OUTPUTS: none
  CG_POP_VAL,

  // INPUTS: size
  // OUTPUTS: none
  CG_POP_ENV_TO,
} cg_action;

VEC_DECL(cg_action);

typedef LLVMValueRef llvm_value;
VEC_DECL(llvm_value);

typedef LLVMTypeRef llvm_type;
VEC_DECL(llvm_type);

typedef LLVMBasicBlockRef llvm_block;
VEC_DECL(llvm_block);

typedef LLVMValueRef LLVMFunctionRef;
typedef LLVMFunctionRef llvm_function;
VEC_DECL(llvm_function);

typedef enum stage { STAGE_ONE, STAGE_TWO } stage;

typedef struct {
  LLVMContextRef context;
  LLVMBuilderRef builder;
  LLVMModuleRef module;

  vec_cg_action actions;
  bitset act_stage;
  vec_node_ind act_nodes;
  vec_u32 act_sizes;
  vec_llvm_value act_vals;

  vec_llvm_function act_fns;
  vec_string strs;

  vec_llvm_value val_stack;
  vec_llvm_block block_stack;
  vec_llvm_function function_stack;

  // corresponds to in.types
  LLVMTypeRef *llvm_types;

  vec_str_ref env_bnds;
  vec_llvm_value env_vals;
  bitset env_is_builtin;
  // TODO do we need this?
  vec_node_ind env_nodes;

  source_file source;
  parse_tree parse_tree;
  tc_res tc_res;
} cg_state;

static cg_state new_cg_state(LLVMContextRef ctx, LLVMModuleRef mod,
                             source_file source, parse_tree tree,
                             tc_res tc_res) {
  cg_state state = {
    .context = ctx,
    .module = mod,
    .builder = LLVMCreateBuilderInContext(ctx),
    .actions = VEC_NEW,
    .act_stage = bs_new(),
    .act_nodes = VEC_NEW,
    .act_vals = VEC_NEW,
    .act_sizes = VEC_NEW,

    .act_fns = VEC_NEW,
    .strs = VEC_NEW,

    .val_stack = VEC_NEW,
    .block_stack = VEC_NEW,
    .function_stack = VEC_NEW,

    .llvm_types = (LLVMTypeRef *)calloc(tc_res.types.len, sizeof(LLVMTypeRef)),
    .env_bnds = VEC_NEW,
    .env_vals = VEC_NEW,
    .env_is_builtin = bs_new(),
    .env_nodes = VEC_NEW,

    .source = source,
    .parse_tree = tree,
    .tc_res = tc_res,
  };
  // Sometimes we want to be nowhere
  VEC_PUSH(&state.block_stack, NULL);
  return state;
}

static void destroy_cg_state(cg_state *state) {
  VEC_FREE(&state->actions);
  VEC_FREE(&state->strs);
  VEC_FREE(&state->val_stack);
  VEC_FREE(&state->block_stack);
  VEC_FREE(&state->function_stack);
  free(state->llvm_types);
  VEC_FREE(&state->env_bnds);
  VEC_FREE(&state->env_vals);
  bs_free(&state->env_is_builtin);
  VEC_FREE(&state->env_nodes);
}

static stage pop_stage(bitset *bs) {
  debug_assert(bs->len > 0);
  return bs_pop(bs) ? STAGE_TWO : STAGE_ONE;
}

static void push_stage(bitset *bs, stage s) { bs_push(bs, s == STAGE_TWO); }

static void push_node_act(cg_state *state, node_ind_t node_ind, stage stage) {
  VEC_PUSH(&state->actions, CG_NODE);
  push_stage(&state->act_stage, stage);
  VEC_PUSH(&state->act_nodes, node_ind);
}

static void push_pattern_act(cg_state *state, node_ind_t node_ind) {
  VEC_PUSH(&state->actions, CG_PATTERN);
  VEC_PUSH(&state->act_nodes, node_ind);
}

typedef enum {
  GEN_TYPE,
  COMBINE_TYPE,
} gen_type_action;

VEC_DECL(gen_type_action);

static LLVMTypeRef construct_type(cg_state *state, node_ind_t root_type_ind) {

  LLVMTypeRef *llvm_types = state->llvm_types;
  {
    LLVMTypeRef prev = llvm_types[root_type_ind];
    if (prev != NULL)
      return prev;
  }

  vec_gen_type_action actions = VEC_NEW;
  VEC_PUSH(&actions, GEN_TYPE);
  vec_node_ind inds = VEC_NEW;
  VEC_PUSH(&inds, root_type_ind);
  while (inds.len > 0) {
    gen_type_action action = VEC_POP(&actions);
    node_ind_t type_ind = VEC_POP(&inds);
    type t = VEC_GET(state->tc_res.types, type_ind);
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
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            VEC_PUSH(&actions, GEN_TYPE);
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
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            VEC_PUSH(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_TUP_SUB_A(t));
            VEC_PUSH(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_TUP_SUB_B(t));
            break;
          }
          case T_FN: {
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            VEC_PUSH(&actions, GEN_TYPE);
            VEC_PUSH(&inds, T_FN_PARAM_IND(t));
            VEC_PUSH(&actions, GEN_TYPE);
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
            node_ind_t sub_ind = T_LIST_SUB_IND(t);
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
  return llvm_types[root_type_ind];
}

static void cg_node(cg_state *state) {
  node_ind_t ind = VEC_POP(&state->act_nodes);
  parse_node node = state->parse_tree.nodes[ind];
  stage stage = pop_stage(&state->act_stage);
  switch (node.type) {
    case PT_CALL: {
      node_ind_t callee_ind = PT_CALL_CALLEE_IND(node);
      switch (stage) {
        case STAGE_ONE: {
          push_node_act(state, ind, STAGE_TWO);
          // These will be in the same order on the value (out) stack
          push_node_act(state, PT_CALL_CALLEE_IND(node), STAGE_ONE);
          push_node_act(state, PT_CALL_PARAM_IND(node), STAGE_ONE);
          break;
        }
        case STAGE_TWO: {
          LLVMValueRef callee = VEC_POP(&state->val_stack);
          LLVMValueRef param = VEC_POP(&state->val_stack);
          LLVMTypeRef fn_type =
            construct_type(state, state->tc_res.node_types[callee_ind]);
          LLVMBuildCall2(state->builder, fn_type, callee, &param, 1, "call");
          break;
        }
      }
      break;
    }
    case PT_LOWER_NAME:
    case PT_UPPER_NAME: {
      binding b = {
        .start = node.start,
        .end = node.end,
      };
      node_ind_t ind = lookup_bnd(state->source.data, state->env_bnds,
                                  state->env_is_builtin, b);
      // missing refs are caught in typecheck phase
      debug_assert(ind != state->env_bnds.len);
      VEC_PUSH(&state->val_stack, VEC_GET(state->env_vals, ind));
      break;
    }
    case PT_INT: {
      node_ind_t type_ind = state->tc_res.node_types[ind];
      LLVMTypeRef type = construct_type(state, type_ind);
      const char *str = VEC_GET_PTR(state->source, node.start);
      size_t len = node.end - node.start + 1;
      VEC_PUSH(&state->val_stack,
               LLVMConstIntOfStringAndSize(type, str, len, 10));
      break;
    }
    case PT_IF: {
      switch (stage) {
        case STAGE_ONE:
          push_node_act(state, ind, STAGE_TWO);

          // cond
          push_node_act(state, PT_IF_COND_IND(state->parse_tree.inds, node),
                        STAGE_ONE);

          // then
          VEC_PUSH(&state->strs, THEN_STR);
          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          VEC_PUSH(&state->act_nodes,
                   PT_IF_A_IND(state->parse_tree.inds, node));

          // else
          VEC_PUSH(&state->strs, ELSE_STR);
          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          VEC_PUSH(&state->act_nodes,
                   PT_IF_B_IND(state->parse_tree.inds, node));
          break;
        case STAGE_TWO: {
          LLVMValueRef cond = VEC_POP(&state->val_stack);
          LLVMValueRef then_val = VEC_POP(&state->val_stack);
          LLVMValueRef else_val = VEC_POP(&state->val_stack);
          LLVMTypeRef res_type =
            construct_type(state, state->tc_res.node_types[ind]);
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
          break;
        }
      }

      break;
    }
    case PT_TUP: {
      switch (stage) {
        case STAGE_ONE:
          push_node_act(state, ind, STAGE_TWO);
          for (size_t i = 0; i < node.sub_amt / 2; i++) {
            node_ind_t sub_ind = state->parse_tree.inds[node.subs_start + i];
            push_node_act(state, sub_ind, STAGE_ONE);
          }
          break;
        case STAGE_TWO: {
          LLVMTypeRef tup_type =
            construct_type(state, state->tc_res.node_types[ind]);
          const char *name = "tuple";
          LLVMValueRef allocated =
            LLVMBuildAlloca(state->builder, tup_type, name);
          for (size_t i = 0; i < node.sub_amt; i++) {
            size_t j = node.sub_amt - 1 - i;
            // 2^32 cardinality tuples is probably enough
            LLVMValueRef ptr =
              LLVMBuildStructGEP2(state->builder, tup_type, allocated, j, name);
            LLVMValueRef val = VEC_POP(&state->val_stack);
            LLVMBuildStore(state->builder, val, ptr);
          }
          VEC_PUSH(&state->val_stack, allocated);
          break;
        }
      }
      break;
    }
    case PT_FUN_BODY:
    case PT_ROOT: {
      for (size_t i = 0; i < PT_BLOCK_SUB_AMT(node); i++) {
        node_ind_t sub_ind = PT_BLOCK_SUB_IND(state->parse_tree.inds, node, i);
        parse_node sub = state->parse_tree.nodes[sub_ind];
        if (sub.type == PT_SIG)
          continue;
        push_node_act(state, sub_ind, STAGE_ONE);
      }
      break;
    }
    case PT_FUN:
      switch (stage) {
        case STAGE_ONE: {
          LLVMTypeRef fn_type =
            construct_type(state, state->tc_res.node_types[ind]);
          const char *name = "fn";
          // We should add this back at some point, I guess
          // LLVMLinkage linkage = LLVMAvailableExternallyLinkage;
          LLVMValueRef fn = LLVMAddFunction(state->module, name, fn_type);
          VEC_PUSH(&state->function_stack, fn);

          u32 env_amt = state->env_bnds.len;
          VEC_PUSH(&state->actions, CG_POP_ENV_TO);
          VEC_PUSH(&state->act_sizes, env_amt);

          push_node_act(state, ind, STAGE_TWO);

          node_ind_t param_ind = PT_FUN_PARAM_IND(state->parse_tree.inds, node);
          push_pattern_act(state, param_ind);
          LLVMValueRef arg = LLVMGetParam(fn, 0);
          VEC_PUSH(&state->act_vals, arg);

          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          VEC_PUSH(&state->strs, EMPTY_STR);
          VEC_PUSH(&state->act_nodes,
                   PT_FUN_BODY_IND(state->parse_tree.inds, node));
          break;
        }
        case STAGE_TWO: {
          LLVMValueRef ret = VEC_POP(&state->val_stack);
          VEC_POP(&state->block_stack);
          LLVMBuildRet(state->builder, ret);
          VEC_POP(&state->function_stack);
          LLVMPositionBuilderAtEnd(state->builder,
                                   VEC_PEEK(state->block_stack));
          break;
        }
      }
      break;
    case PT_AS: {
      push_node_act(state, PT_AS_VAL_IND(node), STAGE_ONE);
      break;
    }
    case PT_UNIT: {
      LLVMTypeRef void_type =
        construct_type(state, state->tc_res.node_types[ind]);
      LLVMValueRef void_val = LLVMGetUndef(void_type);
      VEC_PUSH(&state->val_stack, void_val);
      break;
    }
    // TODO: all of these
    case PT_LIST_TYPE:
    case PT_FN_TYPE: {
      give_up("Tried to typecheck type-level node as term-level");
      break;
    }
    case PT_FN:
    case PT_LET:
    case PT_SIG:
    case PT_LIST:
    case PT_STRING:
    case PT_CONSTRUCTION: {
      printf("Typechecking %s nodes hasn't been implemented yet.\n",
             parse_node_string(node.type));
      exit(1);
      break;
    }
  }
}

enum pat_actions {
  CG_PAT_NODE,
};

static void cg_pattern(cg_state *state) {
  // TODO remove stage?
  // also, finish writing this
  node_ind_t ind = VEC_POP(&state->act_nodes);
  parse_node node = state->parse_tree.nodes[ind];

  switch (node.type) {
    case PT_TUP: {
      node_ind_t sub_a = PT_TUP_SUB_A(node);
      node_ind_t sub_b = PT_TUP_SUB_B(node);

      push_pattern_act(state, sub_a);
      push_pattern_act(state, sub_b);
      break;
    }
    case PT_UNIT:
      break;
    case PT_CONSTRUCTION:
    case PT_UPPER_NAME:
    case PT_LIST:
    case PT_INT:
    case PT_STRING:

    // pattern calls will be parsed as CONSTRUCTIONs
    case PT_CALL:
    case PT_ROOT:
    case PT_AS:
    case PT_LIST_TYPE:
    case PT_LOWER_NAME:
    case PT_IF:
    case PT_FUN:
    case PT_FN:
    case PT_FN_TYPE:
    case PT_FUN_BODY:
    case PT_LET:
    case PT_SIG:
      give_up("Unknown pattern candidate");
      break;
  }
}

static void cg_llvm_module(LLVMContextRef ctx, LLVMModuleRef mod,
                           source_file source, parse_tree tree, tc_res tc_res) {

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  cg_state state = new_cg_state(ctx, mod, source, tree, tc_res);

  push_node_act(&state, tree.root_ind, STAGE_ONE);

  while (state.actions.len > 0) {

    // printf("Codegen action %d\n", action_num++);
    cg_action action = VEC_POP(&state.actions);
    switch (action) {
      case CG_NODE: {
        debug_assert(state.act_stage.len > 0);
        cg_node(&state);
        break;
      }
      case CG_GEN_BLOCK: {
        const char *name = VEC_POP(&state.strs);
        LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(
          state.context, VEC_PEEK(state.function_stack), name);
        LLVMPositionBuilderAtEnd(state.builder, block);
        VEC_PUSH(&state.block_stack, block);
        VEC_PUSH(&state.actions, CG_NODE);
        push_stage(&state.act_stage, STAGE_ONE);
        break;
      }
      case CG_POP_VAL: {
        VEC_POP(&state.val_stack);
        break;
      }
      case CG_POP_ENV_TO: {
        u32 pop_to = VEC_POP(&state.act_sizes);
        u32 pop_amt = state.env_bnds.len - pop_to;

        VEC_POP_N(&state.env_bnds.len, pop_amt);
        VEC_POP_N(&state.env_nodes.len, pop_amt);
        VEC_POP_N(&state.env_vals.len, pop_amt);
        bs_pop_n(&state.env_is_builtin, pop_amt);
        break;
      }
      case CG_PATTERN: {
        cg_pattern(&state);
        break;
      }
    }
  }
  destroy_cg_state(&state);
}

void gen_and_print_module(source_file source, parse_tree tree, tc_res tc_res,
                          FILE *out_f) {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("my_module", ctx);
  cg_llvm_module(ctx, mod, source, tree, tc_res);
  fputs(LLVMPrintModuleToString(mod), out_f);
}
