#include <stdbool.h>
#include <stdlib.h>

#include "binding.h"
#include "bitset.h"
#include "builtins.h"
#include "parse_tree.h"
#include "traverse.h"
#include "util.h"

// For a known-length, and a null-terminated string
static bool strn1eq(const char *a, const char *b, size_t alen) {
  size_t i;
  for (i = 0; i < alen; i++) {
    // handles b '\0'
    if (a[i] != b[i])
      return false;
  }
  if (b[i] != '\0')
    return false;
  return true;
}

// This extremely simple string comparison improved compile time by about 3-4%
static bool streq(const char *restrict a, const char *restrict b,
                  buf_ind_t len) {
  while (len--) {
    if (*a != *b) {
      return false;
    }
    a++;
    b++;
  }
  return true;
}

typedef struct {
  bitset is_builtin;
  vec_str_ref bindings;
} scope;

// TODO consider separating builtins into separate array somehow
// Find index of binding, return bindings.len if not found
static node_ind_t lookup_str_ref(const char *source_file, scope scope,
                                 binding bnd) {
  const char *bndp = source_file + bnd.start;
  for (size_t i = 0; i < scope.bindings.len; i++) {
    size_t ind = scope.bindings.len - 1 - i;
    str_ref a = VEC_GET(scope.bindings, ind);
    if (bs_get(scope.is_builtin, ind)) {
      if (strn1eq(bndp, a.builtin, bnd.len)) {
        return ind;
      }
    } else {
      if (a.binding.len != bnd.len)
        continue;
      if (streq(bndp, &source_file[a.binding.start], bnd.len))
        return ind;
    }
  }
  return scope.bindings.len;
}

static scope scope_new(void) {
  scope res = {
    .bindings = VEC_NEW,
    .is_builtin = bs_new(),
  };
  return res;
}

static void scope_push(scope *s, binding b) {
  str_ref str = {
    .binding = b,
  };
  bs_push_false(&s->is_builtin);
  VEC_PUSH(&s->bindings, str);
}

static void scope_free(scope s) {
  bs_free(&s.is_builtin);
  VEC_FREE(&s.bindings);
}

typedef struct {
  vec_binding not_found;
  parse_tree tree;
  const char *restrict input;
  pt_traverse_elem elem;
  scope environment;
  scope type_environment;
} scope_calculator_state;

// Setting the binding's bariable_index to the thing we're about to push
// is theoretically unnecessary work, but it's nit to have a concreate
// index to look things up with in eg. the llvm stage.
static void precalculate_scope_push(scope_calculator_state *state) {
  switch (state->elem.data.node_data.node.type.binding) {
    case PT_BIND_FUN: {
      node_ind_t binding_ind =
        PT_FUN_BINDING_IND(state->tree.inds, state->elem.data.node_data.node);
      parse_node *binding_node = &state->tree.nodes[binding_ind];
      binding b = binding_node->span;
      binding_node->variable_index = state->environment.bindings.len;
      scope_push(&state->environment, b);
      break;
    }
    case PT_BIND_WILDCARD: {
      parse_node *node =
        &state->tree.nodes[state->elem.data.node_data.node_index];
      node->variable_index = state->environment.bindings.len;
      binding b = node->span;
      scope_push(&state->environment, b);
      break;
    }
    case PT_BIND_LET: {
      node_ind_t binding_ind = PT_LET_BND_IND(state->elem.data.node_data.node);
      parse_node *node = &state->tree.nodes[binding_ind];
      node->variable_index = state->environment.bindings.len;
      binding b = node->span;
      scope_push(&state->environment, b);
      break;
    }
  }
}

static void precalculate_scope_visit(scope_calculator_state *state) {
  parse_node node = state->elem.data.node_data.node;
  scope scope;
  switch (node.type.all) {
    case PT_ALL_TY_PARAM_NAME:
    case PT_ALL_TY_CONSTRUCTOR_NAME: {
      scope = state->type_environment;
      break;
    }
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_EX_UPPER_NAME: {
      scope = state->environment;
      break;
    }
    default:
      return;
  }
  node_ind_t index = lookup_str_ref(state->input, scope, node.span);
  if (index == scope.bindings.len) {
    VEC_PUSH(&state->not_found, node.span);
  }
  state->tree.nodes[state->elem.data.node_data.node_index].variable_index =
    index;
}

resolution_errors resolve_bindings(parse_tree tree,
                                   const char *restrict input) {
  pt_traversal traversal = pt_traverse(tree, TRAVERSE_RESOLVE_BINDINGS);
  scope_calculator_state state = {
    .not_found = VEC_NEW,
    .tree = tree,
    .input = input,
    .environment = scope_new(),
    .type_environment = scope_new(),
  };
  bs_push_true_n(&state.type_environment.is_builtin, named_builtin_type_amount);
  bs_push_true_n(&state.environment.is_builtin, builtin_term_amount);
  for (node_ind_t i = 0; i < named_builtin_type_amount; i++) {
    str_ref s = {.builtin = builtin_type_names[i]};
    VEC_PUSH(&state.type_environment.bindings, s);
  }
  for (node_ind_t i = 0; i < builtin_term_amount; i++) {
    str_ref s = {.builtin = builtin_term_names[i]};
    VEC_PUSH(&state.environment.bindings, s);
  }
  while (true) {
    state.elem = pt_traverse_next(&traversal);
    switch (state.elem.action) {
      case TR_NEW_BLOCK:
      case TR_LINK_SIG:
        break;
      case TR_POP_TO:
        state.environment.bindings.len = state.elem.data.new_environment_amount;
        break;
      case TR_END: {
        const VEC_LEN_T len = state.not_found.len;
        resolution_errors res = {
          .binding_amt = len,
          .bindings = VEC_FINALIZE(&state.not_found),
        };
        scope_free(state.environment);
        scope_free(state.type_environment);
        return res;
      }
      case TR_PREDECLARE_FN:
      case TR_PUSH_SCOPE_VAR:
        precalculate_scope_push(&state);
        break;
      case TR_VISIT_IN:
        precalculate_scope_visit(&state);
        break;
      case TR_VISIT_OUT:
        break;
    }
  }
}
