#include <stdio.h>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
// #include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Support/TargetSelect.h"

/*

This module doesn't use the reverse actions trick used in other modules,
so please make sure to push actions onto the stack in the reverse of
the order you want them executed.

TODO: benchmark:
1. Stack of actions, where an action is a tagged union (saves lookups?)
2. Stack of action tags, separate stacks or parameters (saves space?)
3. Stack of action tags, and a stack of combined parameters for each variant (packed?)

*/

#ifdef DEBUG_CG
#define debug_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug_print(...) do {} while (false);
#endif

extern "C" {
  #include "binding.h"
  #include "bitset.h"
  #include "consts.h"
  #include "llvm.h"
  #include "parse_tree.h"
  #include "scope.h"
  #include "typecheck.h"
  #include "util.h"
  #include "vec.h"
}

typedef enum : uint8_t {
  // INPUTS: none
  // OUTPUTS: val_stack
  CG_NODE,
  // INPUTS: none
  // OUTPUTS: val_stack, block_stack
  CG_GEN_FUNCTION_BODY,
  CG_GEN_BLOCK,
  CG_POP_VAL,
  CG_POP_ENV,
  CG_POP_BLOCK,
  CG_RET,
} cg_action;

VEC_DECL(cg_action);

typedef llvm::Value* llvm_value;
VEC_DECL(llvm_value);

typedef llvm::Type* llvm_type;
VEC_DECL(llvm_type);

typedef llvm::BasicBlock *llvm_block;
VEC_DECL(llvm_block);

typedef llvm::Function* llvm_function;
VEC_DECL(llvm_function);

enum stage { STAGE_ONE, STAGE_TWO };

struct cg_state {
  llvm::LLVMContext &context;
  llvm::Module &module;
  llvm::IRBuilder<> builder;

  vec_cg_action actions;
  bitset act_stage;
  vec_node_ind act_nodes;

  vec_llvm_function act_fns;
  vec_string strs;

  vec_llvm_value val_stack;
  vec_llvm_block block_stack;
  vec_llvm_function function_stack;

  tc_res in;

  // corresponds to in.types
  llvm::Type **llvm_types;

  vec_str_ref env_bnds;
  vec_llvm_value env_vals;
  bitset env_is_builtin;
  // TODO do we need this?
  vec_node_ind env_nodes;

  cg_state(llvm::LLVMContext &ctx, llvm::Module &mod, tc_res in_p) :
    context(ctx),
    module(mod),
    builder(this->context)
  {
    actions = VEC_NEW;
    act_stage = bs_new();
    act_nodes = VEC_NEW;

    act_fns = VEC_NEW;
    strs = VEC_NEW;

    val_stack = VEC_NEW;
    block_stack = VEC_NEW;
    function_stack = VEC_NEW;

    in = in_p;

    llvm_types = (llvm::Type **) calloc(in.types.len, sizeof(llvm::Type*)),
    env_bnds = VEC_NEW;
    env_vals = VEC_NEW;
    env_is_builtin = bs_new();
    env_nodes = VEC_NEW;
  }

  ~cg_state() {
    VEC_FREE(&actions);
    VEC_FREE(&strs);
    VEC_FREE(&val_stack);
    VEC_FREE(&block_stack);
    VEC_FREE(&function_stack);
    free(llvm_types);
    VEC_FREE(&env_bnds);
    VEC_FREE(&env_vals);
    bs_free(&env_is_builtin);
    VEC_FREE(&env_nodes);
  }
};

typedef enum {
  GEN_TYPE,
  COMBINE_TYPE,
} gen_type_action;

VEC_DECL(gen_type_action);

static llvm::Type *construct_type(cg_state *state, NODE_IND_T root_type_ind) {

  llvm::Type **llvm_types = state->llvm_types;
  {
    llvm::Type *prev = llvm_types[root_type_ind];
    if (prev != NULL) return prev;
  }

  vec_gen_type_action actions = VEC_NEW;
  VEC_PUSH(&actions, GEN_TYPE);
  vec_node_ind inds = VEC_NEW;
  VEC_PUSH(&inds, root_type_ind);
  while (inds.len > 0) {
    gen_type_action action = VEC_POP(&actions);
    NODE_IND_T type_ind = VEC_POP(&inds);
    type t = VEC_GET(state->in.types, type_ind);
    switch (action) {
      case GEN_TYPE: {
        if (llvm_types[type_ind] != NULL) break;
        switch (t.tag) {
          case T_BOOL: {
            llvm_types[type_ind] =
              llvm::Type::getInt1Ty(state->context);
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
            llvm_types[type_ind] = llvm::Type::getInt8Ty(state->context);
            break;
          }
          case T_I16:
          case T_U16: {
            llvm_types[type_ind] = llvm::Type::getInt16Ty(state->context);
            break;
          }
          case T_I32:
          case T_U32: {
            llvm_types[type_ind] = llvm::Type::getInt32Ty(state->context);
            break;
          }
          case T_I64:
          case T_U64: {
            llvm_types[type_ind] = llvm::Type::getInt64Ty(state->context);
            break;
          }
          case T_TUP: {
            VEC_PUSH(&actions, COMBINE_TYPE);
            VEC_PUSH(&inds, type_ind);
            for (NODE_IND_T i = 0; i < T_TUP_SUB_AMT(t); i++) {
              VEC_PUSH(&actions, GEN_TYPE);
              VEC_PUSH(&inds, T_TUP_SUB_IND(state->in.type_inds.data, t, i));
            }
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
            llvm_types[type_ind] = llvm::Type::getVoidTy(state->context);
            break;
          }
          case T_CALL: {
            give_up("Type parameter saturation isn't supported by the llvm backend yet");
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
            NODE_IND_T sub_ind = t.sub_a;
            llvm_types[type_ind] = (llvm::Type*) llvm::ArrayType::get(llvm_types[sub_ind], 0);
            break;
          }
          case T_TUP: {
            size_t sub_bytes = sizeof(LLVMTypeRef) * T_TUP_SUB_AMT(t);
            llvm::Type **subs = (llvm::Type**) stalloc(sub_bytes);
            for (size_t i = 0; i < T_TUP_SUB_AMT(t); i++) {
              NODE_IND_T sub_ind = T_TUP_SUB_IND(state->in.type_inds.data, t, i);
              subs[i] = llvm_types[sub_ind];
            }
            llvm::ArrayRef<llvm::Type*> subs_arr = llvm::ArrayRef<llvm::Type*>(subs, t.sub_amt);
            llvm_types[type_ind] = llvm::StructType::create(state->context, subs_arr, "tuple", false);
            stfree(subs, sub_bytes);
            break;
          }
          case T_FN: {
            NODE_IND_T param_ind = T_FN_PARAM_IND(t);
            NODE_IND_T ret_ind = T_FN_RET_IND(t);
            llvm::Type *param_type = llvm_types[param_ind];
            llvm::Type *ret_type = llvm_types[ret_ind];
            llvm::ArrayRef<llvm::Type*> subs_arr = llvm::ArrayRef<llvm::Type*>(param_type);
            llvm_types[type_ind] = llvm::FunctionType::get(ret_type, subs_arr, false);
            break;
          }
          // TODO: There should really be a separate module-local tag for combinable tags
          // to avoid this non-totality.
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

static stage pop_stage(bitset *bs) {
  debug_assert(bs->len > 0);
  return bs_pop(bs) ? STAGE_TWO : STAGE_ONE;
}

static void push_stage(bitset *bs, stage s) {
  bs_push(bs, s == STAGE_TWO);
}

static void cg_node(cg_state *state, node_ind ind) {
  parse_node node = state->in.tree.nodes[ind];
  stage stage = pop_stage(&state->act_stage);
  switch (node.type) {
    case PT_CALL: {
      NODE_IND_T callee_ind = PT_CALL_CALLEE_IND(node);
      switch (stage) {
        case STAGE_ONE: {
          NODE_IND_T param_ind = PT_CALL_PARAM_IND(node);
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_TWO);
          VEC_PUSH(&state->act_nodes, ind);
          // These will be in the same order on the value (out) stack
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_ONE);
          VEC_PUSH(&state->act_nodes, state->in.tree.nodes[callee_ind]);
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_ONE);
          VEC_PUSH(&state->act_nodes, state->in.tree.nodes[param_ind]);
          break;
        }
        case STAGE_TWO: {
          llvm::Value *callee = VEC_POP(&state->val_stack);
          llvm::Value *param = VEC_POP(&state->val_stack);
          llvm::FunctionType *fn_type = (llvm::FunctionType *) construct_type(state, state->in.node_types[callee_ind]);
          llvm::ArrayRef<llvm::Value*> param_arr = llvm::ArrayRef<llvm::Value*>(param);
          state->builder.CreateCall(fn_type, callee, param_arr);
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
      NODE_IND_T ind = lookup_bnd(state->in.source.data, state->env_bnds,
                                  state->env_is_builtin, b);
      // missing refs are caught in typecheck phase
      debug_assert(ind != state->env_bnds.len);
      VEC_PUSH(&state->val_stack, VEC_GET(state->env_vals, ind));
      break;
    }
    case PT_INT: {
      NODE_IND_T type_ind = state->in.node_types[ind];
      llvm::IntegerType *type = (llvm::IntegerType*) construct_type(state, type_ind);
      const char *str = VEC_GET_PTR(state->in.source, node.start);
      size_t len = node.end - node.start + 1;
      VEC_PUSH(&state->val_stack, llvm::ConstantInt::get(type, llvm::StringRef(str, len), 10));
      break;
    }
    case PT_IF: {
      switch (stage) {
        case STAGE_ONE:
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_TWO);
          VEC_PUSH(&state->act_nodes, ind);

          // cond
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_ONE);
          VEC_PUSH(&state->act_nodes, PT_IF_COND_IND(state->in.tree.inds, node));

          // then
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_ONE);
          VEC_PUSH(&state->act_nodes, PT_IF_A_IND(state->in.tree.inds, node));
          VEC_PUSH(&state->strs, "then");
          VEC_PUSH(&state->actions, CG_GEN_BLOCK);

          // else
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_ONE);
          VEC_PUSH(&state->act_nodes, PT_IF_B_IND(state->in.tree.inds, node));
          VEC_PUSH(&state->strs, "else");
          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          break;
        case STAGE_TWO: {
          llvm::Value *cond = VEC_POP(&state->val_stack);
          llvm::Value *then_val = VEC_POP(&state->val_stack);
          llvm::Value *else_val = VEC_POP(&state->val_stack);
          llvm::Type *res_type = construct_type(state, state->in.node_types[ind]);
          llvm::BasicBlock *then_block = VEC_POP(&state->block_stack);
          llvm::BasicBlock *else_block = VEC_POP(&state->block_stack);

          // TODO do we need to pass the current block in?
          llvm::BranchInst::Create(then_block, else_block, cond, VEC_PEEK(state->block_stack));
      
          llvm::Twine name("if-end");
          llvm::BasicBlock *end_block = llvm::BasicBlock::Create(
            state->context, name, VEC_PEEK(state->function_stack));
          state->builder.SetInsertPoint(end_block);
          llvm::PHINode *phi = state->builder.CreatePHI(res_type, 2, "end-if");
          phi->addIncoming(then_val, then_block);
          phi->addIncoming(else_val, else_block);

          state->builder.SetInsertPoint(then_block);
          state->builder.CreateBr(end_block);

          state->builder.SetInsertPoint(else_block);
          state->builder.CreateBr(end_block);
          break;
        }
        break;
      }
      
      break;
    }
    case PT_TUP: {
      switch (stage) {
        case STAGE_ONE:
          VEC_PUSH(&state->actions, CG_NODE);
          push_stage(&state->act_stage, STAGE_TWO);
          VEC_PUSH(&state->act_nodes, ind);
          for (size_t i = 0; i < node.sub_amt / 2; i++) {
            NODE_IND_T sub_ind = state->in.tree.inds[node.subs_start + i];
            VEC_PUSH(&state->actions, CG_NODE);
            push_stage(&state->act_stage, STAGE_ONE);
            VEC_PUSH(&state->act_nodes, sub_ind);
          }
          break;
        case STAGE_TWO: {
          llvm::Type *tup_type =
            construct_type(state, state->in.node_types[ind]);
          llvm::Twine name("tuple");
          llvm::Value *allocated = state->builder.CreateAlloca(tup_type, 0, nullptr, name);
          for (size_t i = 0; i < node.sub_amt; i++) {
            size_t j = node.sub_amt - 1 - i;
            // 2^32 cardinality tuples is probably enough
            llvm::Value *ptr = state->builder.CreateConstInBoundsGEP1_32(tup_type, allocated, j, name);
            llvm::Value *val = VEC_POP(&state->val_stack);
            state->builder.CreateStore(val, ptr);
          }
          VEC_PUSH(&state->val_stack, allocated);
          break;
        }
      }
      break;
    }
    case PT_ROOT: {
      for (size_t i = 0; i < PT_ROOT_SUB_AMT(node); i++) {
        NODE_IND_T sub_ind = PT_ROOT_SUB_IND(state->in.tree.inds, node, i);
        parse_node node = state->in.tree.nodes[sub_ind];
        if (node.type == PT_SIG) continue;
        VEC_PUSH(&state->actions, CG_NODE);
        push_stage(&state->act_stage, STAGE_ONE);
        VEC_PUSH(&state->act_nodes, sub_ind);

        /*
        node_ind bnd_ind = state->in.tree.inds[sub.subs_start];
        node_ind val_ind = state->in.tree.inds[sub.subs_start + 1];

        parse_node bnd = state->in.tree.nodes[bnd_ind];
        parse_node val = state->in.tree.nodes[val_ind];

        llvm::FunctionType *fn_type = (llvm::FunctionType*) construct_type(state, state->in.node_types[val_ind]);

        size_t bnd_len = bnd.end - bnd.start + 1;
        llvm::StringRef name_ref = llvm::StringRef(VEC_GET_PTR(state->in.source, bnd.start), bnd_len);
        llvm::Twine name(name_ref);

        // TODO don't globally link everything
        llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::ExternalLinkage;
        llvm::Function* fn = llvm::Function::Create(fn_type, linkage, name, state->module);

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
        */
      }
      break;
    }
    case PT_FUN: {
      llvm::FunctionType *fn_type =
        (llvm::FunctionType *) construct_type(state, state->in.node_types[ind]);
      llvm::Twine name("fn");
      llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::AvailableExternallyLinkage;
      llvm::Function *fn = llvm::Function::Create(fn_type, linkage, 0, name);
      VEC_PUSH(&state->actions, CG_GEN_FUNCTION_BODY);
      VEC_PUSH(&state->act_nodes, ind);
      VEC_PUSH(&state->act_fns, fn);
      break;
    }
    case PT_AS: {
      VEC_PUSH(&state->actions, CG_NODE);
      push_stage(&state->act_stage, STAGE_ONE);
      VEC_PUSH(&state->act_nodes,
               state->in.tree.inds[node.subs_start + 1]);
      break;
    }
    case PT_UNIT: {
      llvm::Type *void_type = construct_type(state, state->in.node_types[ind]);
      llvm::Value *void_val = llvm::UndefValue::get(void_type);
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
    case PT_FUN_BODY:
    case PT_LET:
    case PT_SIG:
    case PT_LIST:
    case PT_STRING:
    case PT_CONSTRUCTION: {
      give_up("Unimplemented");
      break;
    }
  }
}

static void cg_llvm_module(llvm::LLVMContext &ctx, llvm::Module &mod, tc_res in) {

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  cg_state state(ctx, mod, in);

  VEC_PUSH(&state.actions, CG_NODE);
  push_stage(&state.act_stage, STAGE_ONE);
  VEC_PUSH(&state.act_nodes, in.tree.root_ind);

  while (state.actions.len > 0) {

    // printf("Codegen action %d\n", action_num++);
    cg_action action = VEC_POP(&state.actions);
    switch (action) {
      case CG_NODE: {
        debug_assert(state.actions.len + 1 == state.act_stage.len);
        cg_node(&state, VEC_POP(&state.act_nodes));
        break;
      }
      case CG_GEN_BLOCK: {
        llvm::Twine name(VEC_POP(&state.strs));
        llvm::BasicBlock *block = llvm::BasicBlock::Create(
          state.context, name, VEC_PEEK(state.function_stack));
        state.builder.SetInsertPoint(block);
        VEC_PUSH(&state.block_stack, block);
        break;
      }
      case CG_POP_VAL: {
        VEC_POP(&state.val_stack);
        break;
      }
      case CG_GEN_FUNCTION_BODY: {
        node_ind ind = VEC_POP(&state.act_nodes);
        llvm::Function *fn = VEC_POP(&state.act_fns);
        llvm::BasicBlock* block = llvm::BasicBlock::Create(state.context, "entry", fn); 
        state.builder.SetInsertPoint(block);
        VEC_PUSH(&state.block_stack, state.builder.GetInsertBlock());
        parse_node node = state.in.tree.nodes[ind];

        for (size_t i = 0; i < node.sub_amt / 2; i++) {
          NODE_IND_T param_name_ind = state.in.tree.inds[node.subs_start + i * 2 + 1];
          parse_node param_name_node = state.in.tree.nodes[param_name_ind];
          debug_assert(param_name_node.type == PT_LOWER_NAME);
          binding b = {
            .start = param_name_node.start,
            .end = param_name_node.end,
          };
          VEC_PUSH(&state.env_bnds, b);
          VEC_PUSH(&state.env_nodes, ind);
          VEC_PUSH(&state.env_vals, &fn->arg_begin()[i]);
          bs_push(&state.env_is_builtin, false);
        }

        for (size_t i = 0; i < node.sub_amt / 2; i++) {
          VEC_PUSH(&state.actions, CG_POP_ENV);
        }

        VEC_PUSH(&state.actions, CG_RET);
        VEC_PUSH(&state.actions, CG_POP_BLOCK);
        VEC_PUSH(&state.actions, CG_NODE);
        push_stage(&state.act_stage, STAGE_ONE);
        VEC_PUSH(&state.act_nodes, state.in.tree.inds[node.subs_start + node.sub_amt - 1]);
        break;
      }
      case CG_RET: {
        llvm::Value *val = VEC_POP(&state.val_stack);
        state.builder.CreateRet(val);
        break;
      }
      case CG_POP_ENV: {
        VEC_POP_(&state.env_bnds);
        VEC_POP_(&state.env_nodes);
        VEC_POP_(&state.env_vals);
        bs_pop(&state.env_is_builtin);
        break;
      }
      case CG_POP_BLOCK: {
        state.builder.SetInsertPoint(VEC_POP(&state.block_stack));
        break;
      }
    }
  }
}

static void optimize(llvm::Module &module, const llvm::PassBuilder::OptimizationLevel opt) {
  // Create the analysis managers.
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  // Create the new pass manager builder.
  // Take a look at the PassBuilder constructor parameters for more
  // customization, e.g. specifying a TargetMachine or various debugging
  // options.
  llvm::PassBuilder PB;

  // Register all the basic analyses with the managers.
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  // Create the pass manager.
  // This one corresponds to a typical -O2 optimization pipeline.
  llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(opt);

  MPM.run(module, MAM);
}

class file_ostream : public llvm::raw_ostream {
  FILE *out;
public:
  void write_impl(const char *ptr, size_t size) override { fwrite(ptr, 1, size, out); }
  uint64_t current_pos() const override { return ftell(out); }
  file_ostream(FILE *o) : out(o) {}
};

static void output_pipeline(llvm::Module &module, FILE *out_f) {
  // Create the analysis managers.
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  // Create the new pass manager builder.
  // Take a look at the PassBuilder constructor parameters for more
  // customization, e.g. specifying a TargetMachine or various debugging
  // options.
  llvm::PassBuilder PB;

  // Register all the basic analyses with the managers.
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  // Create the pass manager.
  // This one corresponds to a typical -O2 optimization pipeline.
  const llvm::PassBuilder::OptimizationLevel opt = llvm::PassBuilder::OptimizationLevel::O0;
  llvm::ModulePassManager MPM = PB.buildO0DefaultPipeline(opt);
  file_ostream out(out_f);
  MPM.addPass(llvm::PrintModulePass(out));

  MPM.run(module, MAM);
}

extern "C" {
  void gen_and_print_module(tc_res in, FILE *out_f) {
    llvm::LLVMContext ctx;
    llvm::Module mod("my_module", ctx);
    cg_llvm_module(ctx, mod, in);

    /*
    file_ostream out(out_f);
  
    llvm::PassBuilder PB;
    llvm::ModuleAnalysisManager MAM;
    // llvm::PrintModulePass print_pass(out);
    const llvm::PassBuilder::OptimizationLevel opt = llvm::PassBuilder::OptimizationLevel::O0;
    llvm::ModulePassManager MPM = PB.buildO0DefaultPipeline(opt);
    */
    // MPM.addPass(llvm::PrintModulePass(out));

    output_pipeline(mod, out_f);
    // MPM.run(mod, MAM);
  }
}
