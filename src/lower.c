#include "ir.h"
#include "parse_tree.h"

typedef struct {
  enum { LOWER_NODE } tag;
  node_ind ind;
} action;

VEC_DECL(action);

typedef struct {
  vec_action actions;
} state;

ir_module lower(parse_tree tree) { ir_module res; }
