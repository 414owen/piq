#include <assert.h>
#include <stdlib.h>

#include "ir.h"
#include "test.h"

typedef struct {
  const char *name;
  size_t node_size;
  size_t vec_size;
} node_info;

#define DECL_NODE_TYPE(typename)                                               \
  {                                                                            \
    .name = #typename, .node_size = sizeof(typename),                          \
    .vec_size = sizeof(vec_##typename),                                        \
  }

static int cmp_node_info(const void *pa, const void *pb) {
  const node_info *a = (node_info *)pa;
  const node_info *b = (node_info *)pb;
  return b->node_size - a->node_size;
}

void print_union_upto(node_info *nodes, size_t from, size_t to, FILE *out) {
  if (from >= to)
    return;
  {
    node_info info = nodes[from];
    fprintf(out, "  // sizeof: %zu\n", info.node_size);
    if (from + 1 == to) {
      fprintf(out, "  vec_%s %ss;\n\n", info.name, info.name);
      return;
    }
  }
  fputs("  union {\n", out);
  for (; from < to; from++) {
    node_info info = nodes[from];
    fprintf(out, "    vec_%s %ss;\n", info.name, info.name);
  }
  fputs("  };\n\n", out);
}

void test_ir_layout(test_state *state) {
  test_start(state, "layout");
  node_info nodes[] = {
    DECL_NODE_TYPE(ir_fun_group),
    DECL_NODE_TYPE(ir_let_group),
    DECL_NODE_TYPE(ir_call),
    DECL_NODE_TYPE(ir_data_construction),
    DECL_NODE_TYPE(ir_fn),
    DECL_NODE_TYPE(ir_fn_type),
    DECL_NODE_TYPE(ir_if),
    DECL_NODE_TYPE(ir_list),
    DECL_NODE_TYPE(ir_list_type),
    DECL_NODE_TYPE(ir_string),
    DECL_NODE_TYPE(ir_tup),
    DECL_NODE_TYPE(ir_as),
  };

  qsort(nodes, STATIC_LEN(nodes), sizeof(nodes[0]), cmp_node_info);

  size_t min_total_size =
    sizeof(ir_root) + sizeof(vec_type) + sizeof(vec_node_ind);

  {
    size_t prev_size = 0;
    for (size_t i = 0; i < STATIC_LEN(nodes); i++) {
      node_info info = nodes[i];
      if (prev_size != info.node_size) {
        min_total_size += info.vec_size;
        prev_size = info.node_size;
      }
    }
  }

  if (min_total_size != sizeof(ir_module)) {
    char *str;

    // print optimal representation
    {
      stringstream ss;
      ss_init_immovable(&ss);
      size_t prev_size = 0;
      size_t prev_ind = 0;
      fputs("typedef struct {\n"
            "  ir_root root;\n\n",
            ss.stream);
      for (size_t i = 0; i < STATIC_LEN(nodes); i++) {
        node_info info = nodes[i];
        if (prev_size != info.node_size) {
          print_union_upto(nodes, prev_ind, i, ss.stream);
          prev_ind = i;
          prev_size = info.node_size;
        }
      }
      print_union_upto(nodes, prev_ind, STATIC_LEN(nodes), ss.stream);
      fputs("  vec_type types;\n"
            "  vec_node_ind node_inds;\n"
            "} ir_module;",
            ss.stream);
      ss_finalize(&ss);
      str = ss.string;
    }
    failf(state,
          "ir_module could be optimized for construction and access patterns.\n"
          "Try something like this:\n%s",
          str);
  }
  test_end(state);
}

void test_ir(test_state *state) {
  assert(state->path.len == 0);
  test_group_start(state, "IR");
  test_ir_layout(state);
  test_group_end(state);
}
