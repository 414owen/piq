#include "ir.h"

void ir_module_free(ir_module *module) { free(module->IR_MODULE_ALLOC_PTR); }

static pt_node_amounts count_pt_node_amounts(parse_tree tree) {
  pt_node_amounts res = {0};

  for (size_t i = 0; i < tree.node_amt; i++) {
    parse_node node = tree.nodes[i];
    switch (node.type) {
      case PT_ROOT:
        res.pt_root++;
        break;
      case PT_CALL:
        res.pt_call++;
        break;
      case PT_CONSTRUCTION:
        res.pt_construction++;
        break;
      case PT_FN:
        res.pt_fn++;
        break;
      case PT_FN_TYPE:
        res.pt_fn_type++;
        break;
      case PT_IF:
        res.pt_if++;
        break;
      case PT_INT:
        res.pt_int++;
        break;
      case PT_LIST:
        res.pt_list++;
        break;
      case PT_LIST_TYPE:
        res.pt_list_type++;
        break;
      case PT_STRING:
        res.pt_string++;
        break;
      case PT_TUP:
        res.pt_tup++;
        break;
      case PT_AS:
        res.pt_as++;
        break;
      case PT_UNIT:
        res.pt_unit++;
        break;
      case PT_FUN:
        res.pt_fun++;
        break;
      case PT_SIG:
        res.pt_sig++;
        break;
      case PT_LET:
        res.pt_let++;
        break;
      case PT_LOWER_NAME:
      case PT_UPPER_NAME:
        res.pt_binding++;
      case PT_FUN_BODY:
        break;
    }
  }

  return res;
}

static size_t ir_count_bytes_required(const pt_node_amounts *amounts) {
  return (sizeof(ir_call) * amounts->pt_call) +
         (sizeof(ir_construction) * amounts->pt_construction) +
         (sizeof(ir_fn) * amounts->pt_fn) +
         (sizeof(ir_fn_type) * amounts->pt_fn_type) +
         (sizeof(ir_if) * amounts->pt_if) + (sizeof(ir_int) * amounts->pt_int) +
         (sizeof(ir_list) * amounts->pt_list) +
         (sizeof(ir_list_type) * amounts->pt_list_type) +
         (sizeof(ir_string) * amounts->pt_string) +
         (sizeof(ir_tup) * amounts->pt_tup) + (sizeof(ir_as) * amounts->pt_as) +
         (sizeof(ir_unit) * amounts->pt_unit) +
         (sizeof(ir_fun_group) * amounts->pt_fun) +
         (sizeof(ir_let_group) * amounts->pt_let);
}

ir_module ir_module_new(parse_tree tree) {
  ir_module module = {
    .ir_root = {0},
    .ir_types = NULL,
    .ir_node_inds = NULL,
  };

  pt_node_amounts amounts = count_pt_node_amounts(tree);
  char *ptr = malloc(ir_count_bytes_required(&amounts));

  // Leave this here. Must match IR_MODULE_ALLOC_PTR
  module.ir_fun_groups = (typeof(module.ir_fun_groups[0]) *)ptr;
  ptr += sizeof(module.ir_fun_groups[0]) * amounts.pt_fun;

  module.ir_let_groups = (typeof(module.ir_let_groups[0]) *)ptr;
  ptr += sizeof(module.ir_let_groups[0]) * amounts.pt_let;

  module.ir_calls = (typeof(module.ir_calls[0]) *)ptr;
  ptr += sizeof(module.ir_calls[0]) * amounts.pt_call;

  module.ir_constructions = (typeof(module.ir_constructions[0]) *)ptr;
  ptr += sizeof(module.ir_constructions[0]) * amounts.pt_construction;

  module.ir_fns = (typeof(module.ir_fns[0]) *)ptr;
  ptr += sizeof(module.ir_fns[0]) * amounts.pt_fn;

  module.ir_fn_types = (typeof(module.ir_fn_types[0]) *)ptr;
  ptr += sizeof(module.ir_fn_types[0]) * amounts.pt_fn_type;

  module.ir_ifs = (typeof(module.ir_ifs[0]) *)ptr;
  ptr += sizeof(module.ir_ifs[0]) * amounts.pt_if;

  module.ir_ints = (typeof(module.ir_ints[0]) *)ptr;
  ptr += sizeof(module.ir_ints[0]) * amounts.pt_int;

  module.ir_lists = (typeof(module.ir_lists[0]) *)ptr;
  ptr += sizeof(module.ir_lists[0]) * amounts.pt_list;

  module.ir_list_types = (typeof(module.ir_list_types[0]) *)ptr;
  ptr += sizeof(module.ir_list_types[0]) * amounts.pt_list_type;

  module.ir_strings = (typeof(module.ir_strings[0]) *)ptr;
  ptr += sizeof(module.ir_strings[0]) * amounts.pt_string;

  module.ir_tups = (typeof(module.ir_tups[0]) *)ptr;
  ptr += sizeof(module.ir_tups[0]) * amounts.pt_tup;

  module.ir_ass = (typeof(module.ir_ass[0]) *)ptr;
  ptr += sizeof(module.ir_ass[0]) * amounts.pt_as;

  module.ir_units = (typeof(module.ir_units[0]) *)ptr;
  ptr += sizeof(module.ir_units[0]) * amounts.pt_unit;

  return module;
}
