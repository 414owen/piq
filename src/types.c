#include "builtins.h"
#include "types.h"

type_ref find_primitive_type(type_builder *tb, type_check_tag tag) {
  for (type_ref i = 0; i < tb->types.len; i++) {
    type t = VEC_GET(tb->types, i);
    if (t.check_tag == tag)
      return i;
  }
  return tb->types.len;
}

type_ref find_type(type_builder *tb, type_check_tag tag, const type_ref *subs,
                   type_ref sub_amt) {
  for (type_ref i = 0; i < tb->types.len; i++) {
    type t = VEC_GET(tb->types, i);
    if (t.check_tag != tag || t.sub_amt != sub_amt)
      continue;
    type_ref *p1 = &VEC_DATA_PTR(&tb->inds)[t.subs_start];
    if (!memeq(p1, subs, sub_amt * sizeof(type_ref)))
      continue;
    return i;
  }
  return tb->types.len;
}

type_ref mk_primitive_type(type_builder *tb, type_check_tag tag) {
  debug_assert(type_repr(tag) == SUBS_NONE);
  type_ref ind = find_primitive_type(tb, tag);
  if (ind < tb->types.len)
    return ind;
  type t = {
    .check_tag = tag,
    .sub_amt = 0,
    .subs_start = 0,
  };
  VEC_PUSH(&tb->types, t);
  return tb->types.len - 1;
}

type_ref mk_type_inline(type_builder *tb, type_check_tag tag, type_ref sub_a,
                        type_ref sub_b) {
  debug_assert(type_repr(tag) == SUBS_ONE || type_repr(tag) == SUBS_TWO);
  type t = {
    .check_tag = tag,
    .sub_a = sub_a,
    .sub_b = sub_b,
  };
  for (size_t i = 0; i < tb->types.len; i++) {
    type t2 = VEC_GET(tb->types, i);
    if (inline_types_eq(t, t2)) {
      return i;
    }
  }
  VEC_PUSH(&tb->types, t);
  return tb->types.len - 1;
}

type_ref mk_type(type_builder *tb, type_check_tag tag, const type_ref *subs,
                 type_ref sub_amt) {
  debug_assert(type_repr(tag) == SUBS_EXTERNAL);
  if (subs == NULL) {
    return mk_primitive_type(tb, tag);
  }
  type_ref ind = find_type(tb, tag, subs, sub_amt);
  if (ind < tb->types.len)
    return ind;
  // debug_assert(sizeof(subs[0]) == sizeof(VEC_GET(state->types.type_inds,
  // 0))); size_t find_range(const void *haystack, size_t el_size, size_t
  // el_amt, const void *needle, size_t needle_els);
  suffix_arr_lookup_res suffix_res = find_range_with_suffix_array_u32(
    VEC_DATA_PTR(&tb->inds), VEC_DATA_PTR(&tb->suffix_array), subs, tb->inds.len, sub_amt);
  switch (suffix_res.type) {
    case SUFF_NOTHING:
      VEC_APPEND(&tb->inds, sub_amt, subs);
      break;
  }
  type t = {
    .check_tag = tag,
    .sub_amt = sub_amt,
    .subs_start = ind,
  };
  VEC_PUSH(&tb->types, t);
  return tb->types.len - 1;
}

type_ref mk_type_var(type_builder *tb, typevar value) {
  type t = {
    .check_tag = TC_VAR,
    .type_var = value,
  };
  // Don't check for duplicates, because type variables should be unique
  // and constructed once
  VEC_PUSH(&tb->types, t);
  return tb->types.len - 1;
}

typedef struct {
  u8 early_exit : 1;
  u8 redirect_to_next : 1;
  type_ref next;
} var_step_res;
typedef var_step_res (*typevar_step)(typevar a, const void *data);
typedef bool exited_early;

void push_type_subs(vec_type_ref *restrict stack, const type_ref *restrict inds,
                    type t) {
  switch (type_repr(t.check_tag)) {
    case SUBS_NONE:
      break;
    case SUBS_TWO:
      VEC_PUSH(stack, t.sub_b);
      HEDLEY_FALL_THROUGH;
    case SUBS_ONE:
      VEC_PUSH(stack, t.sub_a);
      break;
    case SUBS_EXTERNAL:
      VEC_APPEND(stack, t.sub_amt, &inds[t.subs_start]);
      break;
  }
}

// TODO this is only used once, we should just monomorphise it over the step.
static exited_early type_contains_typevar_by(const type_builder *types,
                                             type_ref root, typevar_step step,
                                             const void *data) {
  bool exited_early = false;
  vec_type_ref stack = VEC_NEW;
  VEC_PUSH(&stack, root);
  const type_ref *inds = VEC_DATA_PTR(&types->inds);
  while (stack.len > 0) {
    type_ref node_ind;
    VEC_POP(&stack, &node_ind);
    type node = VEC_GET(types->types, node_ind);
    switch (node.check_tag) {
      case TC_VAR: {
        var_step_res step_res = step(node.type_var, data);
        if (step_res.early_exit) {
          exited_early = true;
          goto end;
        }
        if (step_res.redirect_to_next) {
          VEC_PUSH(&stack, step_res.next);
        }
        break;
      }
      case TC_OR:
      case TC_UNIT:
      case TC_I8:
      case TC_U8:
      case TC_I16:
      case TC_U16:
      case TC_I32:
      case TC_U32:
      case TC_I64:
      case TC_U64:
      case TC_FN:
      case TC_BOOL:
      case TC_TUP:
      case TC_LIST:
      case TC_CALL:
        break;
    }
    push_type_subs(&stack, inds, node);
  }

end:
  VEC_FREE(&stack);
  return exited_early;
}

typedef struct {
  typevar target;
  type *types;
  type_ref *substitutions;
} cmp_specific_typevar_ctx;

static var_step_res cmp_typevar(typevar a, const void *data) {
  cmp_specific_typevar_ctx *ctx = (cmp_specific_typevar_ctx *)data;
  if (a == ctx->target) {
    var_step_res res = {
      .early_exit = true,
    };
    return res;
  }
  type_ref type_ind = ctx->substitutions[a];
  type t = ctx->types[type_ind];
  var_step_res res = {
    .early_exit = false,
    .redirect_to_next = t.check_tag != TC_VAR || t.type_var != a,
    .next = type_ind,
  };
  return res;
}

bool type_contains_specific_typevar(const type_builder *types, type_ref root,
                                    typevar a) {
  cmp_specific_typevar_ctx ctx = {
    .target = a,
    .substitutions = VEC_DATA_PTR(&types->substitutions),
    .types = VEC_DATA_PTR(&types->types),
  };
  return type_contains_typevar_by(types, root, cmp_typevar, &ctx);
}

typedef struct {
  type_builder builder;
  node_ind_t root_node;
  node_ind_t parse_node_amount;
} unsubstituted_check_data;

/*
static bool is_unsubstituted_typevar(typevar a, const type_builder *builder) {
  type_ref type_ind = VEC_GET(builder->substitutions, a);
  type t = VEC_GET(builder->types, type_ind);
  return (t.check_tag == TC_VAR && a == t.type_var);
}
*/

static var_step_res is_unsubstituted_typevar_step(typevar a,
                                                  const void *data_p) {
  const unsubstituted_check_data *data = (unsubstituted_check_data *)data_p;
  const type_builder *builder = &data->builder;
  const type_ref type_ind = VEC_GET(builder->substitutions, a);
  const type t = VEC_GET(builder->types, type_ind);
  // If this branch containsunsubstituted vars
  // they'll be reported by their parse_node.
  const bool is_node_var = a < data->parse_node_amount;
  if (t.check_tag == TC_VAR &&
      (a == t.type_var || !is_node_var || a == data->root_node)) {
    const var_step_res res = {
      .early_exit = true,
    };
    return res;
  }
  const var_step_res res = {
    .early_exit = false,
    .redirect_to_next = true,
    .next = type_ind,
  };
  return res;
}

bool type_contains_unsubstituted_typevar(const type_builder *builder,
                                         type_ref root,
                                         node_ind_t parse_node_amount) {
  unsubstituted_check_data data = {
    .builder = *builder,
    .parse_node_amount = parse_node_amount,
    .root_node = root,
  };
  return type_contains_typevar_by(
    builder, root, is_unsubstituted_typevar_step, &data);
}

type_builder new_type_builder(void) {
  type_builder type_builder = {
    .types = VEC_NEW,
    .inds = VEC_NEW,
    .suffix_array = VEC_NEW,
    .substitutions = VEC_NEW,
  };
  return type_builder;
}

type_ref *builtin_suffixes = NULL;

int cmp_builtin_suffixes(const void *a_par, const void *b_par) {
  type_ref a = *((type_ref*) a_par);
  type_ref b = *((type_ref*) b_par);
  type_ref *a_suff = &builtin_type_inds[a];
  type_ref *b_suff = &builtin_type_inds[b];
  type_ref a_suff_len = builtin_type_ind_amount - a;
  type_ref b_suff_len = builtin_type_ind_amount - b;
  int res = memcmp(a_suff, b_suff, MIN(a_suff_len, b_suff_len));
  if (res == 0) {
    return a_suff_len < b_suff_len ? -1 : 1;
  }
  return res;
}

void initialise_types(void) {
  builtin_suffixes = malloc(sizeof(type_ref) * builtin_type_ind_amount);
  for (type_ref i = 0; i < builtin_type_ind_amount; i++) {
    builtin_suffixes[i] = i;
  }
  // It's possible to do this in linear time and constant space.
  // I'm using qsort for simplicity, as I don't expect this to ever show up on a profile.
  qsort(builtin_suffixes, builtin_type_ind_amount, sizeof(type_ref), cmp_builtin_suffixes);
}

type_builder new_type_builder_with_builtins(void) {
  type_builder res = new_type_builder();
  // blit builtin types
  VEC_APPEND(&res.types, builtin_type_amount, builtin_types);
  VEC_APPEND(&res.inds, builtin_type_ind_amount, builtin_type_inds);
  VEC_APPEND(&res.suffix_array, builtin_type_ind_amount, builtin_suffixes);
  return res;
}

void free_type_builder(type_builder tb) {
  VEC_FREE(&tb.inds);
  VEC_FREE(&tb.types);
  VEC_FREE(&tb.substitutions);
}
