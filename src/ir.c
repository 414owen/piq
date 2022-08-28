#include "ir.h"

void ir_module_free(ir_module *module) {
  free(module->IR_MODULE_ALLOC_PTR);
}
