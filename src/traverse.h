#include "parse_tree.h"
#include "vec.h"

// If traversing the AST ever becomes a bottleneck (unlikely)
// we could consider specializing the traversal to these cases
typedef enum {

  // - [x] traverse patterns on the way in
  // - [x] traverse patterns on the way out
  // - [x] traverse expressions on the way in
  // - [x] traverse expressions on the way out
  // - [ ] push scope
  // - [ ] pop scope
  // - [ ] link signatures
  // - [ ] block operations
  TRAVERSE_PRINT_TREE = 0,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse expressions on the way in
  // - [ ] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [ ] link signatures
  // - [ ] block operations
  TRAVERSE_RESOLVE_BINDINGS = 1,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse expressions on the way in
  // - [ ] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [x] link signatures
  // - [ ] block operations
  TRAVERSE_TYPECHECK = 2,

  // - [x] traverse patterns on the way in
  // - [ ] traverse patterns on the way out
  // - [x] traverse expressions on the way in (eg FN)
  // - [x] traverse expressions on the way out
  // - [x] push scope
  // - [x] pop scope
  // - [ ] link signatures
  // - [x] block operations
  TRAVERSE_CODEGEN = 3,

} traverse_mode;

typedef enum {
  // first time we see a node
  TR_ACT_INITIAL = 0,
  TR_ACT_PUSH_SCOPE_VAR = 1,
  TR_ACT_VISIT_IN = 2,
  TR_ACT_VISIT_OUT = 3,
  TR_ACT_POP_TO = 4,
  TR_ACT_END = 5,
  TR_ACT_LINK_SIG = 6,
} scoped_traverse_internal_action;

typedef enum {
  TR_PUSH_SCOPE_VAR = TR_ACT_INITIAL,
  TR_NEW_BLOCK = TR_ACT_PUSH_SCOPE_VAR,
  TR_VISIT_IN = TR_ACT_VISIT_IN,
  TR_VISIT_OUT = TR_ACT_VISIT_OUT,
  TR_POP_TO = TR_ACT_POP_TO,
  TR_END = TR_ACT_END,
  TR_LINK_SIG = TR_ACT_LINK_SIG,
} scoped_traverse_action;

typedef union {
  scoped_traverse_action action;
  scoped_traverse_internal_action action_internal;
} traverse_action_union;

VEC_DECL_CUSTOM(traverse_action_union, vec_traverse_action);

typedef struct {
  const parse_node *nodes;
  const node_ind_t *inds;
  vec_parse_node_type_all path;
  // const node_ind_t node_amt;
  vec_traverse_action actions;
  node_ind_t environment_amt;
  const traverse_mode mode;
  union {
    vec_node_ind node_stack;
    vec_node_ind amt_stack;
  };
} pt_traversal;

typedef struct {
  node_ind_t node_index;
  parse_node node;
} traversal_node_data;

typedef struct {
  node_ind_t sig_index;
  node_ind_t linked_index;
} traversal_link_sig_data;

typedef union {
  traversal_node_data node_data;
  traversal_link_sig_data link_sig_data;
  node_ind_t new_environment_amount;
} pt_trav_elem_data;

typedef struct {
  scoped_traverse_action action;
  pt_trav_elem_data data;
} pt_traverse_elem;

pt_traversal pt_scoped_traverse(parse_tree tree, traverse_mode mode);
pt_traverse_elem pt_scoped_traverse_next(pt_traversal *traversal);
