#include <stdlib.h>

#include "llvm-c/Analysis.h"
#include "llvm-c/BitWriter.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"

#include "binding.h"
#include "bitset.h"
#include "consts.h"
#include "parse_tree.h"
#include "scope.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

typedef enum {
  CG_NODE,
  CG_GEN_FUNCTION_BODY,
} cg_action;

VEC_DECL(cg_action);

typedef LLVMValueRef llvm_value;
VEC_DECL(llvm_value);

typedef LLVMTypeRef llvm_type;
VEC_DECL(llvm_type);

typedef struct {
  LLVMModuleRef mod;
  LLVMContextRef context;

  vec_cg_action actions;
  vec_llvm_value act_fns;
  vec_node_ind act_nodes;

  vec_llvm_value val_stack;

  tc_res in;

  // corresponds to in.types
  LLVMTypeRef *llvm_types;
  bitset llvm_type_generated;

  vec_str_ref env_bnds;
  vec_llvm_value env_vals;
  bitset env_is_builtin;
  // TODO do we need this?
  vec_node_ind env_nodes;
} cg_state;

static void cg_combine_node(cg_state *state, node_ind ind) {
  parse_node node = state->in.tree.nodes.data[ind];
  switch (node.type) {
    case PT_CALL: {
      for (size_t i = 0; i < node.sub_amt; i++) {
        parse_node sub = state->in.tree.nodes
                           .data[state->in.tree.inds.data[node.subs_start + i]];
      }
      // TODO
      break;
    }
    default:
      fputs("Unimplemented\n", stderr);
      abort();
      break;
  }
}

typedef enum {
  GEN_TYPE,
  COMBINE_TYPE,
} gen_type_action;

VEC_DECL(gen_type_action);

static LLVMTypeRef construct_type(cg_state *state, NODE_IND_T root_type_ind) {
  vec_gen_type_action actions = VEC_NEW;
  VEC_PUSH(&actions, GEN_TYPE);
  vec_node_ind inds = VEC_NEW;
  VEC_PUSH(&inds, root_type_ind);
  while (inds.len > 0) {
    gen_type_action action = VEC_POP(&actions);
    NODE_IND_T type_ind = VEC_POP(&inds);
    type t = state->in.types.data[type_ind];
    switch (action) {
      case GEN_TYPE: {
        if (bs_get(state->llvm_type_generated, type_ind))
          break;
        switch (t.tag) {
          case T_BOOL: {
            state->llvm_types[type_ind] =
              LLVMIntTypeInContext(state->context, 1);
            bs_set(&state->llvm_type_generated, type_ind, true);
            break;
          }
          case T_LIST: {
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            VEC_PUSH(&actions, GEN_TYPE);
            VEC_PUSH(&inds, state->in.type_inds.data[t.sub_start]);
            break;
          }
          case T_I8:
          case T_U8: {
            state->llvm_types[type_ind] = LLVMInt8TypeInContext(state->context);
            bs_set(&state->llvm_type_generated, type_ind, true);
            break;
          }
          case T_I16:
          case T_U16: {
            state->llvm_types[type_ind] =
              LLVMInt16TypeInContext(state->context);
            bs_set(&state->llvm_type_generated, type_ind, true);
            break;
          }
          case T_I32:
          case T_U32: {
            state->llvm_types[type_ind] =
              LLVMInt32TypeInContext(state->context);
            bs_set(&state->llvm_type_generated, type_ind, true);
            break;
          }
          case T_I64:
          case T_U64: {
            state->llvm_types[type_ind] =
              LLVMInt64TypeInContext(state->context);
            bs_set(&state->llvm_type_generated, type_ind, true);
            break;
          }
          case T_TUP: {
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            for (NODE_IND_T i = 0; i < t.sub_amt; i++) {
              VEC_PUSH(&actions, GEN_TYPE);
              VEC_PUSH(&inds, state->in.type_inds.data[t.sub_start + i]);
            }
            break;
          }
          case T_FN: {
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            for (NODE_IND_T i = 0; i < t.sub_amt; i++) {
              VEC_PUSH(&actions, GEN_TYPE);
              VEC_PUSH(&inds, state->in.type_inds.data[t.sub_start + i]);
            }
            break;
          }
          default: {
            give_up("Type unsupported by LLVM backend");
            break;
          }
        }
      }

      case COMBINE_TYPE: {
        size_t sub_bytes = sizeof(LLVMTypeRef) * t.sub_amt;
        LLVMTypeRef *subs = stalloc(sub_bytes);
        bs_set(&state->llvm_type_generated, type_ind, true);
        switch (t.tag) {
          case T_LIST: {
            NODE_IND_T sub_ind = state->in.type_inds.data[t.sub_start];
            state->llvm_types[type_ind] =
              LLVMArrayType(state->llvm_types[sub_ind], 0);
            break;
          }
          case T_TUP: {
            for (size_t i = 0; i < t.sub_amt; i++) {
              subs[i] =
                state->llvm_types[state->in.type_inds.data[t.sub_start + 1]];
            }
            state->llvm_types[type_ind] =
              LLVMStructTypeInContext(state->context, subs, t.sub_amt, false);
            break;
          }
          case T_FN: {
            NODE_IND_T param_amt = t.sub_amt - 1;
            for (NODE_IND_T i = 0; i < t.sub_amt; i++) {
              subs[i] =
                state->llvm_types[state->in.type_inds.data[t.sub_start + 1]];
            }
            state->llvm_types[type_ind] =
              LLVMFunctionType(subs[param_amt], subs, param_amt, false);
            break;
          }
          default:
            give_up("Can't combine non-compound llvm type");
            break;
        }
        stfree(subs, sub_bytes);
      }
    }
  }
  return state->llvm_types[root_type_ind];
}

static void cg_node(cg_state *state, node_ind ind) {
  parse_node node = state->in.tree.nodes.data[ind];
  switch (node.type) {
    case PT_TOP_LEVEL: {
      fputs("Unhandled case: TopLevel\n", stderr);
      abort();
      break;
    }
    case PT_CALL: {
      for (size_t i = 0; i < node.sub_amt; i++) {
        VEC_PUSH(&state->actions, CG_NODE);
      }
      VEC_PUSH(&state->act_nodes, ind);
      break;
    }
    case PT_LOWER_NAME: {
      case PT_UPPER_NAME: {
        binding b = {
          .start = node.start,
          .end = node.end,
        };
        NODE_IND_T ind = lookup_bnd(state->in.source.data, state->env_bnds,
                                    state->env_is_builtin, b);
        // missing refs are caught in typecheck phase
        debug_assert(ind != state->env_bnds.len);
        VEC_PUSH(&state->val_stack, state->env_vals.data[ind]);
        break;
      }
      case PT_INT: {
        NODE_IND_T type_ind = state->in.node_types.data[ind];
        LLVMTypeRef type = construct_type(state, type_ind);
        char *str = &state->in.source.data[node.start];
        size_t len = node.end - node.start + 1;
        VEC_PUSH(&state->val_stack,
                 LLVMConstIntOfStringAndSize(type, str, len, 10));
        break;
      }
      case PT_IF: {
        break;
      }
      case PT_TUP: {
        break;
      }
      case PT_ROOT: {
        for (size_t i = 0; i < node.sub_amt / 2; i++) {
          parse_node sub =
            state->in.tree.nodes
              .data[state->in.tree.inds.data[node.subs_start + i]];

          node_ind bnd_ind = state->in.tree.inds.data[sub.subs_start];
          node_ind val_ind = state->in.tree.inds.data[sub.subs_start + 1];

          parse_node bnd = state->in.tree.nodes.data[bnd_ind];
          parse_node val = state->in.tree.nodes.data[val_ind];

          debug_assert(sub.type == PT_TOP_LEVEL);

          LLVMTypeRef param_types[] = {LLVMInt32Type(), LLVMInt32Type()};
          LLVMTypeRef ret_type =
            LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
          LLVMValueRef fn = LLVMAddFunction(state->mod, "sum", ret_type);

          binding b = {
            .start = bnd.start,
            .end = bnd.end,
          };
          VEC_PUSH(&state->env_bnds, b);
          VEC_PUSH(&state->env_nodes, val_ind);
          VEC_PUSH(&state->env_vals, fn);

          VEC_PUSH(&state->actions, CG_GEN_FUNCTION_BODY);
          VEC_PUSH(&state->act_nodes, val_ind);
          VEC_PUSH(&state->act_fns, fn);
        }
        break;
      }
      case PT_FN: {
        LLVMTypeRef param_types[] = {LLVMInt32Type(), LLVMInt32Type()};
        LLVMTypeRef ret_type =
          LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
        LLVMValueRef fn = LLVMAddFunction(state->mod, "sum", ret_type);
        VEC_PUSH(&state->actions, CG_GEN_FUNCTION_BODY);
        break;
      }
      case PT_TYPED: {
        break;
      }
      case PT_LIST: {
        break;
      }
      case PT_STRING: {
        break;
      }
    }
  }

  static void codegen(tc_res in) {
    cg_state state = {
      .actions = VEC_NEW,
      .in = in,
      .inds = VEC_NEW,
      .env_strs = VEC_NEW,
      .env_vals = VEC_NEW,
      .env_nodes = VEC_NEW,
    };
    VEC_PUSH(&state.actions, CG_NODE);
    VEC_PUSH(&state.inds, in.tree.root_ind);

    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    state.mod = LLVMModuleCreateWithName("my_module");
    state.context = LLVMContextCreate();

    while (state.actions.len > 0) {
      cg_action action = VEC_POP(&state.actions);
      switch (action) {
        case CG_NODE: {
          cg_node(&state, VEC_POP(&state.inds));
          break;
        }
      }
    }
  }

  int main() {

    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    LLVMModuleRef mod = LLVMModuleCreateWithName("my_module");
    LLVMTypeRef param_types[] = {LLVMInt32Type(), LLVMInt32Type()};
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
    LLVMValueRef sum = LLVMAddFunction(mod, "sum", ret_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMValueRef tmp =
      LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
    LLVMBuildRet(builder, tmp);

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    LLVMExecutionEngineRef engine;
    error = NULL;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();

    if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) != 0) {
      fprintf(stderr, "failed to create execution engine\n");
      abort();
    }

    if (error) {
      fprintf(stderr, "error: %s\n", error);
      LLVMDisposeMessage(error);
      exit(EXIT_FAILURE);
    }

    if (LLVMWriteBitcodeToFile(mod, "test.bc") != 0) {
      fprintf(stderr, "error writing bitcode to file, skipping\n");
    }

    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    LLVMTargetRef target;
    LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &error);
    printf("error: %s\n", error);
    LLVMDisposeMessage(error);
    printf("target: %s, [%s], %d, %d\n", LLVMGetTargetName(target),
           LLVMGetTargetDescription(target), LLVMTargetHasJIT(target),
           LLVMTargetHasTargetMachine(target));
    printf("triple: %s\n", LLVMGetDefaultTargetTriple());
    printf("features: %s\n", LLVMGetHostCPUFeatures());
    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, LLVMGetDefaultTargetTriple(), "generic", LLVMGetHostCPUFeatures(),
      LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);

    LLVMSetTarget(mod, LLVMGetDefaultTargetTriple());
    LLVMTargetDataRef datalayout = LLVMCreateTargetDataLayout(machine);
    char *datalayout_str = LLVMCopyStringRepOfTargetData(datalayout);
    printf("datalayout: %s\n", datalayout_str);
    LLVMSetDataLayout(mod, datalayout_str);
    LLVMDisposeMessage(datalayout_str);

    LLVMTargetMachineEmitToFile(machine, mod, "result.o", LLVMObjectFile,
                                &error);
    printf("error: %s\n", error);
    LLVMDisposeMessage(error);
    return 0;
  }