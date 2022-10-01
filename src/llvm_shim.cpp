/* IMPORTANT
   Try not to import IRBuilder.
   It adds a second or two to the build time.
*/
#include "llvm/IR/Module.h"

using namespace llvm;

static Twine mkTwine(const char *volatile name, size_t name_size) {
  return Twine(StringRef(name, name_size));
}

extern "C" {

#include "llvm_shim.h"

LLVMValueRef LLVMAddFunctionCustom(LLVMModuleRef M, const char *name, size_t name_size, LLVMTypeRef FunctionTy) {
   
  return wrap(Function::Create(unwrap<FunctionType>(FunctionTy),
                               GlobalValue::ExternalLinkage, mkTwine(name, name_size), unwrap(M)));
}

}