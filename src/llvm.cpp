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

static const char *EMPTY_STR = "entry";
static const char *THEN_STR = "then";
static const char *ELSE_STR = "else";

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

  // INPUTS: none
  // OUTPUTS: none
  CG_POP_FN,
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
  vec_u32 act_sizes;
  vec_llvm_value act_vals;

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
    act_vals = VEC_NEW;
    act_sizes = VEC_NEW;

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

static stage pop_stage(bitset *bs) {
  debug_assert(bs->len > 0);
  puts("pop");
  return bs_pop(bs) ? STAGE_TWO : STAGE_ONE;
}

static void push_stage(bitset *bs, stage s) {
  puts("push");
  bs_push(bs, s == STAGE_TWO);
}

static void push_node_act(cg_state *state, NODE_IND_T node_ind, stage stage) {
  printf("%d\n", node_ind);
  if (node_ind == 7 || node_ind == 9) {
    printf("here\n");
  }
  VEC_PUSH(&state->actions, CG_NODE);
  push_stage(&state->act_stage, stage);
  VEC_PUSH(&state->act_nodes, node_ind);
}

static void push_pattern_act(cg_state *state, NODE_IND_T node_ind, stage stage) {
  VEC_PUSH(&state->actions, CG_PATTERN);
  push_stage(&state->act_stage, stage);
  VEC_PUSH(&state->act_nodes, node_ind);
}

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
            NODE_IND_T sub_ind_a = T_TUP_SUB_A(t);
            NODE_IND_T sub_ind_b = T_TUP_SUB_B(t);
            llvm::Type *subs[2] = {llvm_types[sub_ind_a], llvm_types[sub_ind_b]};
            llvm::ArrayRef<llvm::Type*> subs_arr = llvm::ArrayRef<llvm::Type*>(subs, 2);
            llvm_types[type_ind] = llvm::StructType::create(state->context, subs_arr, "tuple", false);
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

static void cg_node(cg_state *state) {
  node_ind ind = VEC_POP(&state->act_nodes);
  parse_node node = state->in.tree.nodes[ind];
  stage stage = pop_stage(&state->act_stage);
  switch (node.type) {
    case PT_CALL: {
      NODE_IND_T callee_ind = PT_CALL_CALLEE_IND(node);
      switch (stage) {
        case STAGE_ONE: {
          NODE_IND_T param_ind = PT_CALL_PARAM_IND(node);
          push_node_act(state, ind, STAGE_TWO);
          // These will be in the same order on the value (out) stack
          push_node_act(state, PT_CALL_CALLEE_IND(node), STAGE_ONE);
          push_node_act(state, PT_CALL_PARAM_IND(node), STAGE_ONE);
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
          push_node_act(state, ind, STAGE_TWO);

          // cond
          push_node_act(state, PT_IF_COND_IND(state->in.tree.inds, node), STAGE_ONE);

          // then
          VEC_PUSH(&state->strs, THEN_STR);
          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          VEC_PUSH(&state->act_nodes, PT_IF_A_IND(state->in.tree.inds, node));

          // else
          VEC_PUSH(&state->strs, ELSE_STR);
          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          VEC_PUSH(&state->act_nodes, PT_IF_B_IND(state->in.tree.inds, node));
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
          push_node_act(state, ind, STAGE_TWO);
          for (size_t i = 0; i < node.sub_amt / 2; i++) {
            NODE_IND_T sub_ind = state->in.tree.inds[node.subs_start + i];
            push_node_act(state, sub_ind, STAGE_ONE);
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
    case PT_FUN_BODY:
    case PT_ROOT: {
      for (size_t i = 0; i < PT_BLOCK_SUB_AMT(node); i++) {
        NODE_IND_T sub_ind = PT_BLOCK_SUB_IND(state->in.tree.inds, node, i);
        parse_node sub = state->in.tree.nodes[sub_ind];
        if (sub.type == PT_SIG) continue;
        push_node_act(state, sub_ind, STAGE_ONE);
      }
      break;
    }
    case PT_FUN:
      switch (stage) {
        case STAGE_ONE: {
          llvm::FunctionType *fn_type =
            (llvm::FunctionType *) construct_type(state, state->in.node_types[ind]);
          llvm::Twine name("fn");
          llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::AvailableExternallyLinkage;
          llvm::Function *fn = llvm::Function::Create(fn_type, linkage, 0, name);
          VEC_PUSH(&state->function_stack, fn);
          fn->hasLazyArguments();

          VEC_PUSH(&state->actions, CG_POP_FN);
          u32 env_amt = state->env_bnds.len;
          VEC_PUSH(&state->actions, CG_POP_ENV_TO);
          VEC_PUSH(&state->act_sizes, env_amt);

          push_node_act(state, ind, STAGE_TWO);

          NODE_IND_T param_ind = PT_FUN_PARAM_IND(state->in.tree.inds, node);
          push_pattern_act(state, param_ind, STAGE_ONE);
          llvm::Type *param_type = construct_type(state, state->in.node_types[param_ind]);
          llvm::Argument *arg = fn->getArg(0);
          VEC_PUSH(&state->act_vals, arg);

          VEC_PUSH(&state->actions, CG_GEN_BLOCK);
          VEC_PUSH(&state->strs, EMPTY_STR);
          VEC_PUSH(&state->act_nodes, PT_FUN_BODY_IND(state->in.tree.inds, node));
          break;
        }
        case STAGE_TWO: {
          llvm::Value *ret = VEC_POP(&state->val_stack);
          VEC_POP(&state->block_stack);
          state->builder.CreateRet(ret);
          break;
        }
      }
    break;
    case PT_AS: {
      push_node_act(state, PT_AS_VAL_IND(node), STAGE_ONE);
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
    case PT_LET:
    case PT_SIG:
    case PT_LIST:
    case PT_STRING:
    case PT_CONSTRUCTION: {
      printf("Typechecking %s nodes hasn't been implemented yet.\n", parse_node_string(node.type));
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
  node_ind ind = VEC_POP(&state->act_nodes);
  parse_node node = state->in.tree.nodes[ind];
  stage stage = pop_stage(&state->act_stage);

  switch (node.type) {
    case PT_TUP: {
      node_ind sub_a = PT_TUP_SUB_A(node);
      node_ind sub_b = PT_TUP_SUB_B(node);

      push_pattern_act(state, sub_a, STAGE_ONE);
      push_pattern_act(state, sub_b, STAGE_ONE);
      break;
    }
    case PT_UNIT: break;
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

static void cg_llvm_module(llvm::LLVMContext &ctx, llvm::Module &mod, tc_res in) {

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  cg_state state(ctx, mod, in);

  push_node_act(&state, in.tree.root_ind, STAGE_ONE);

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
        llvm::Twine name(VEC_POP(&state.strs));
        llvm::BasicBlock *block = llvm::BasicBlock::Create(
          state.context, name, VEC_PEEK(state.function_stack));
        state.builder.SetInsertPoint(block);
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
