#include <llvm-c/Core.h>

#include "../consts.h"
#include "../parse_tree.h"
#include "../types.h"
#include "../vec.h"

typedef enum {
  GEN_TYPE,
  // TODO speialize this, to avoid switch-in-switch?
  COMBINE_TYPE,
} llvm_gen_type_action;

VEC_DECL(llvm_gen_type_action);

static void push_gen_type_action(vec_llvm_gen_type_action *actions, llvm_gen_type_action action) {
  VEC_PUSH(actions, action);
}

// TODO this lazy construction is unnecessary, we should just construct all the types up-front

LLVMTypeRef llvm_construct_type_internal(LLVMTypeRef *llvm_type_refs, type_tree types, LLVMContextRef context, node_ind_t root_type_ind) {
  // print_type(stdout, types.nodes, types.inds, root_type_ind);
  // putc('\n', stdout);
  vec_llvm_gen_type_action actions = VEC_NEW;
  push_gen_type_action(&actions, GEN_TYPE);
  vec_node_ind ind_stack = VEC_NEW;
  VEC_PUSH(&ind_stack, root_type_ind);
  node_ind_t *type_inds = types.inds;
  while (ind_stack.len > 0) {
    llvm_gen_type_action action;
    VEC_POP(&actions, &action);
    node_ind_t type_ind;
    VEC_POP(&ind_stack, &type_ind);
    type t = types.nodes[type_ind];
    switch (action) {
      case GEN_TYPE: {
        if (llvm_type_refs[type_ind] != NULL)
          break;
        switch (t.tag.normal) {
          case T_BOOL: {
            llvm_type_refs[type_ind] = LLVMInt1TypeInContext(context);
            break;
          }
          case T_LIST: {
            push_gen_type_action(&actions, COMBINE_TYPE);
            VEC_PUSH(&ind_stack, type_ind);
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&ind_stack, T_LIST_SUB_IND(t));
            break;
          }
          case T_I8:
          case T_U8: {
            llvm_type_refs[type_ind] = LLVMInt8TypeInContext(context);
            break;
          }
          case T_I16:
          case T_U16: {
            llvm_type_refs[type_ind] = LLVMInt16TypeInContext(context);
            break;
          }
          case T_I32:
          case T_U32: {
            llvm_type_refs[type_ind] = LLVMInt32TypeInContext(context);
            break;
          }
          case T_I64:
          case T_U64: {
            llvm_type_refs[type_ind] = LLVMInt64TypeInContext(context);
            break;
          }
          case T_TUP: {
            push_gen_type_action(&actions, COMBINE_TYPE);
            VEC_PUSH(&ind_stack, type_ind);
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&ind_stack, T_TUP_SUB_A(t));
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&ind_stack, T_TUP_SUB_B(t));
            break;
          }
          case T_FN: {
            push_gen_type_action(&actions, COMBINE_TYPE);
            VEC_PUSH(&ind_stack, type_ind);
            node_ind_t param_amt = T_FN_PARAM_AMT(t);
            for (node_ind_t i = 0; i < param_amt; i++) {
              push_gen_type_action(&actions, GEN_TYPE);
              VEC_PUSH(&ind_stack, T_FN_PARAM_IND(type_inds, t, i));
            }
            push_gen_type_action(&actions, GEN_TYPE);
            VEC_PUSH(&ind_stack, T_FN_RET_IND(type_inds, t));
            break;
          }
          case T_UNIT: {
            LLVMTypeRef res = LLVMStructCreateNamed(context, "unit");
            LLVMStructSetBody(res, NULL, 0, false);
            llvm_type_refs[type_ind] = res;
            break;
          }
          case T_CALL: {
            give_up("Type parameter saturation isn't supported by the llvm "
                    "backend yet");
            break;
          }
        }
        break;
      }

      case COMBINE_TYPE: {
        switch (t.tag.normal) {
          case T_LIST: {
            node_ind_t sub_ind = t.data.one_sub.ind;
            llvm_type_refs[type_ind] =
              (LLVMTypeRef)LLVMArrayType(llvm_type_refs[sub_ind], 0);
            break;
          }
          case T_TUP: {
            node_ind_t sub_ind_a = T_TUP_SUB_A(t);
            node_ind_t sub_ind_b = T_TUP_SUB_B(t);
            LLVMTypeRef subs[2] = {llvm_type_refs[sub_ind_a],
                                   llvm_type_refs[sub_ind_b]};
            LLVMTypeRef res = LLVMStructCreateNamed(context, "tuple");
            LLVMStructSetBody(res, subs, 2, false);
            llvm_type_refs[type_ind] = res;
            break;
          }
          case T_FN: {
            node_ind_t param_amt = T_FN_PARAM_AMT(t);
            size_t n_param_bytes = sizeof(LLVMTypeRef) * param_amt;
            LLVMTypeRef *llvm_params = stalloc(n_param_bytes);
            for (node_ind_t i = 0; i < param_amt; i++) {
              node_ind_t param_ind = T_FN_PARAM_IND(type_inds, t, i);
              llvm_params[i] = llvm_type_refs[param_ind];
            }
            node_ind_t ret_ind = T_FN_RET_IND(type_inds, t);
            LLVMTypeRef ret_type = llvm_type_refs[ret_ind];
            llvm_type_refs[type_ind] =
              LLVMFunctionType(ret_type, llvm_params, param_amt, false);
            stfree(llvm_params, n_param_bytes);
            break;
          }
          // TODO: There should really be a separate module-local tag for
          // combinable tags to avoid this non-totality.
          case T_CALL:
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
            // TODO remove this with more specific actions
            give_up("Can't combine non-compound llvm type");
            break;
          }
        }
        break;
      }
    }
  }
  VEC_FREE(&actions);
  VEC_FREE(&ind_stack);
  return llvm_type_refs[root_type_ind];
}

