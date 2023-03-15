#include <stdbool.h>
#include <stdlib.h>

#include "binding.h"
#include "bitset.h"
#include "builtins.h"
#include "hashmap.h"
#include "hashers.h"
#include "parse_tree.h"
#include "traverse.h"
#include "util.h"
#include "vec.h"

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
  for (size_t i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

typedef struct {
  bitset is_builtin;
  vec_str_ref bindings;
  vec_environment_ind shadows;
  a_hashmap map;
} scope;

typedef struct {
  scope *scope;
  const char *source_file;
} resolve_map_ctx;

// TODO consider separating builtins into separate array somehow
// Find index of binding, return bindings.len if not found
static node_ind_t lookup_str_ref(const char *source_file, scope scope,
                                 binding bnd) {
  resolve_map_ctx ctx = {
    .scope = &scope,
    .source_file = source_file,
  };
  u32 bucket_ind = ahm_lookup(&scope.map, &bnd, &ctx);
  return bs_get(scope.map.occupied, bucket_ind)
           ? ((environment_ind_t *)scope.map.keys)[bucket_ind]
           : scope.bindings.len;
}

static bool cmp_bnd(const void *bnd_p, const void *ind_p, const void *ctx_p) {
  const binding bnd = *((binding *)bnd_p);
  const environment_ind_t bnd_ind = *((environment_ind_t *)ind_p);
  const resolve_map_ctx ctx = *((resolve_map_ctx *)ctx_p);
  const char *bndp = ctx.source_file + bnd.start;
  const str_ref a = VEC_GET(ctx.scope->bindings, bnd_ind);
  bool res = false;
  if (bs_get(ctx.scope->is_builtin, bnd_ind)) {
    if (*bndp == *a.builtin && strn1eq(bndp, a.builtin, bnd.len)) {
      res = true;
    }
  } else {
    const char *b = &ctx.source_file[a.binding.start];
    if (a.binding.len == bnd.len && *bndp == *b && streq(bndp, b, bnd.len)) {
      res = true;
    }
  }
  return res;
}

static scope scope_new(void) {
  scope res = {
    .bindings = VEC_NEW,
    .is_builtin = bs_new(),
    .shadows = VEC_NEW,
    .map = hashset_new(
      environment_ind_t, cmp_bnd, hash_binding, hash_stored_binding),
  };
  return res;
}

static void scope_push(const char *source_file, scope *s, binding b) {
  str_ref str = {
    .binding = b,
  };
  resolve_map_ctx ctx = {
    .scope = s,
    .source_file = source_file,
  };
  ahm_maybe_rehash(&s->map, &ctx);
  u32 bucket_ind = ahm_lookup(&s->map, &b, &ctx);
  u32 prev = bs_get(s->map.occupied, bucket_ind)
               ? ((environment_ind_t *)s->map.keys)[bucket_ind]
               : s->bindings.len;
  bs_push_false(&s->is_builtin);
  VEC_PUSH(&s->bindings, str);
  VEC_PUSH(&s->shadows, prev);
  const environment_ind_t key_stored = s->bindings.len - 1;
  ahm_insert_at(&s->map, bucket_ind, &key_stored, NULL);
}

static void scope_free(scope s) {
  bs_free(&s.is_builtin);
  VEC_FREE(&s.bindings);
  VEC_FREE(&s.shadows);
  ahm_free(&s.map);
}

typedef struct {
  vec_binding not_found;
  parse_tree tree;
  const char *restrict input;
  pt_traverse_elem elem;
  scope environment;
  scope type_environment;
  u32 num_names_looked_up;
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
      scope_push(state->input, &state->environment, b);
      break;
    }
    case PT_BIND_WILDCARD: {
      parse_node *node =
        &state->tree.nodes[state->elem.data.node_data.node_index];
      node->variable_index = state->environment.bindings.len;
      binding b = node->span;
      scope_push(state->input, &state->environment, b);
      break;
    }
    case PT_BIND_LET: {
      node_ind_t binding_ind = PT_LET_BND_IND(state->elem.data.node_data.node);
      parse_node *node = &state->tree.nodes[binding_ind];
      node->variable_index = state->environment.bindings.len;
      binding b = node->span;
      scope_push(state->input, &state->environment, b);
      break;
    }
  }
}

static void precalculate_scope_visit(scope_calculator_state *state) {
  parse_node node = state->elem.data.node_data.node;
  scope *scope_p;
  switch (node.type.all) {
    case PT_ALL_TY_PARAM_NAME:
    case PT_ALL_TY_CONSTRUCTOR_NAME: {
      scope_p = &state->type_environment;
      break;
    }
    case PT_ALL_EX_TERM_NAME:
    case PT_ALL_EX_UPPER_NAME: {
      scope_p = &state->environment;
      break;
    }
    default:
      return;
  }
  scope scope = *scope_p;
  state->num_names_looked_up++;
  VEC_LEN_T index = lookup_str_ref(state->input, scope, node.span);
  if (index == scope.bindings.len) {
    VEC_PUSH(&state->not_found, node.span);
  }
  state->tree.nodes[state->elem.data.node_data.node_index].variable_index =
    index;
}

static void resolve_pop_env(scope_calculator_state *state) {
  scope *env = &state->environment;
  resolve_map_ctx ctx = {
    .scope = env,
    .source_file = state->input,
  };
  VEC_LEN_T vec_ind = env->bindings.len - 1;
  const u32 bucket_ind = ahm_remove_stored(&env->map, &vec_ind, &ctx);
  bs_pop(&env->is_builtin);
  str_ref ref;
  VEC_POP(&env->bindings, &ref);
  environment_ind_t prev;
  VEC_POP(&env->shadows, &prev);
  if (prev < env->bindings.len) {
    ahm_insert_at(&env->map, bucket_ind, &prev, NULL);
  }
}

resolution_res resolve_bindings(parse_tree tree, const char *restrict input) {
  pt_traversal traversal = pt_traverse(tree, TRAVERSE_RESOLVE_BINDINGS);
  scope_calculator_state state = {
    .not_found = VEC_NEW,
    .tree = tree,
    .input = input,
    .environment = scope_new(),
    .type_environment = scope_new(),
    .num_names_looked_up = 0,
  };

#ifdef TIME_NAME_RESOLUTION
  struct timespec start = get_monotonic_time();
#endif

  bs_push_true_n(&state.type_environment.is_builtin, named_builtin_type_amount);
  bs_push_true_n(&state.environment.is_builtin, builtin_term_amount);

  {
    resolve_map_ctx ctx = {
      .scope = &state.type_environment,
      .source_file = state.input,
    };

    for (environment_ind_t i = 0; i < named_builtin_type_amount; i++) {
      str_ref s = {.builtin = builtin_type_names[i]};
      VEC_PUSH(&state.type_environment.shadows, i);
      VEC_PUSH(&state.type_environment.bindings, s);
      ahm_insert_stored(&state.type_environment.map, &i, NULL, &ctx);
    }
  }

  {
    resolve_map_ctx ctx = {
      .scope = &state.environment,
      .source_file = state.input,
    };

    for (node_ind_t i = 0; i < builtin_term_amount; i++) {
      str_ref s = {.builtin = builtin_term_names[i]};
      VEC_PUSH(&state.environment.shadows, i);
      VEC_PUSH(&state.environment.bindings, s);
      ahm_insert_stored(&state.environment.map, &i, NULL, &ctx);
    }
  }

  while (true) {
    state.elem = pt_traverse_next(&traversal);
    switch (state.elem.action) {
      case TR_NEW_BLOCK:
      case TR_LINK_SIG:
        break;
      case TR_POP_TO: {
        const u32 to_pop = state.environment.bindings.len -
                           state.elem.data.new_environment_amount;
        for (u32 i = 0; i < to_pop; i++) {
          resolve_pop_env(&state);
        }
        break;
      }
      case TR_END: {
        const VEC_LEN_T len = state.not_found.len;
        resolution_res res = {
          .not_found =
            {
              .binding_amt = len,
              .bindings = VEC_FINALIZE(&state.not_found),
            },
#ifdef TIME_NAME_RESOLUTION
          .time_taken = time_since_monotonic(start),
          .num_names_looked_up = state.num_names_looked_up,
#endif
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
