#include "builtins.h"
#include "hashmap.h"
#include "types.h"
#include "vec.h"

typedef struct {
  type_check_tag tag;
  union {
    struct {
      type_ref sub_amt;
      const type_ref *subs;
    };
    struct {
      type_ref sub_a;
      type_ref sub_b;
    };
  };
} type_key_with_ctx;

type_ref find_inline_type_slow(type_builder *tb, type_check_tag tag, type_ref sub_a, type_ref sub_b) {
  for (type_ref i = 0; i < tb->types.len; i++) {
    type t = VEC_GET(tb->types, i);
    if (t.check_tag == tag && sub_a == t.sub_a && sub_b == t.sub_b)
      return i;
  }
  return tb->types.len;
}

type_ref find_type_slow(type_builder *tb, type_check_tag tag, const type_ref *subs,
                   type_ref sub_amt) {
  for (type_ref i = 0; i < tb->types.len; i++) {
    type t = VEC_GET(tb->types, i);
    if (t.check_tag != tag || t.sub_amt != sub_amt)
      continue;
    type_ref *p1 = &VEC_DATA_PTR(&tb->inds)[t.subs_start];
    if (memcmp(p1, subs, sub_amt * sizeof(type_ref)) != 0)
      continue;
    return i;
  }
  return tb->types.len;
}

NON_NULL_PARAMS
static
type_ref find_inline_type(type_builder *tb, type_check_tag tag, type_ref sub_a, type_ref sub_b) {
  type_key_with_ctx key = {
    .tag = tag,
    .sub_a = sub_a,
    .sub_b = sub_b,
  };
  uint32_t *ind = ahm_lookup(&tb->type_to_index, &key, tb);
  // TODO remove this branch somehow
  return ind == NULL ? tb->types.len : *ind;
}

NON_NULL_PARAMS
static type_ref find_type(type_builder *tb, const type_key_with_ctx *key) {
  uint32_t *ind = ahm_lookup(&tb->type_to_index, key, tb);
  // TODO remove this branch somehow
  return ind == NULL ? tb->types.len : *ind;
}

static
type_ref insert_inline_type_to_hm(type_builder *tb, type_check_tag tag, type_ref sub_a,
                        type_ref sub_b) {
  type t = {
    .check_tag = tag,
    .sub_a = sub_a,
    .sub_b = sub_b,
  };
  type_key_with_ctx key = {
    .tag = tag,
    .sub_a = sub_a,
    .sub_b = sub_b,
  };
  VEC_PUSH(&tb->types, t);
  type_ref res = tb->types.len - 1;
  ahm_insert(&tb->type_to_index, &key, &t, &res, tb);
  return tb->types.len - 1;
}

type_ref __mk_type_inline(type_builder *tb, type_check_tag tag, type_ref sub_a,
                        type_ref sub_b) {
  type_ref ind = find_inline_type(tb, tag, sub_a, sub_b);
  {
    type_ref ind2 = find_inline_type_slow(tb, tag, sub_a, sub_b);
    if (ind != ind2) {
      if (ind >= tb->types.len) {
        printf("erroneous unfound type\n");
      } else {
        printf("bad type index!\n");
      }
      find_inline_type_slow(tb, tag, sub_a, sub_b);
      find_inline_type(tb, tag, sub_a, sub_b);
    }
  }
  if (ind < tb->types.len)
    return ind;
  return insert_inline_type_to_hm(tb, tag, sub_a, sub_b);
}

type_ref mk_type_inline(type_builder *tb, type_check_tag tag, type_ref sub_a,
                        type_ref sub_b) {
  debug_assert(type_repr(tag) == SUBS_ONE || type_repr(tag) == SUBS_TWO);
  return __mk_type_inline(tb, tag, sub_a, sub_b);
}

type_ref mk_primitive_type(type_builder *tb, type_check_tag tag) {
  debug_assert(type_repr(tag) == SUBS_NONE);
  return __mk_type_inline(tb, tag, 0, 0);
}

type_ref mk_type(type_builder *tb, type_check_tag tag, const type_ref *subs,
                 type_ref sub_amt) {
  debug_assert(type_repr(tag) == SUBS_EXTERNAL);
  if (subs == NULL) {
    return mk_primitive_type(tb, tag);
  }
  const type_key_with_ctx key = {
    .tag = tag,
    .sub_amt = sub_amt,
    .subs = subs,
  };
  type_ref ind = find_type(tb, &key);
  {
    type_ref ind2 = find_type_slow(tb, tag, subs, sub_amt);
    if (ind != ind2) {
      if (ind >= tb->types.len) {
        printf("erroneous unfound type\n");
      } else {
        printf("bad type index!\n");
      }
      find_type(tb, &key);
      find_type_slow(tb, tag, subs, sub_amt);
    }
  }
  if (ind < tb->types.len)
    return ind;
  VEC_APPEND(&tb->inds, sub_amt, subs);
  type t = {
    .check_tag = tag,
    .sub_amt = sub_amt,
    .subs_start = tb->inds.len - sub_amt,
  };
  VEC_PUSH(&tb->types, t);
  type_ref res = tb->types.len - 1;
  // won't update
  ahm_insert(&tb->type_to_index, &key, &t, &res, tb);
  return res;
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

static bool cmp_newtype_eq(const void *key_p, const void *stored_key, const void *ctx) {
  type_key_with_ctx *key = (type_key_with_ctx*) key_p;
  type *snd = (type*) stored_key;
  type_builder *builder = (type_builder*) ctx;

  if (key->tag == snd->check_tag) {
    switch (type_repr(key->tag)) {
      case SUBS_NONE:
        return true;
      case SUBS_TWO:
        return key->sub_a == snd->sub_a
          && key->sub_b == snd->sub_b;
      case SUBS_ONE:
        return key->sub_a == snd->sub_a;
      case SUBS_EXTERNAL:
        return key->sub_amt == snd->sub_amt
          && memcmp(key->subs, &VEC_DATA_PTR(&builder->inds)[snd->subs_start], key->sub_amt * sizeof(type_ref));
    }
  }

  return false;
}

static uint32_t hash_newtype(const void *key_p, const void *ctx) {
  type_key_with_ctx *key = (type_key_with_ctx*) key_p;
  switch (type_repr(key->tag)) {
    case SUBS_EXTERNAL: {
      uint32_t hash = hash_eight_bytes(0, key->tag);
      return hash_bytes(hash, (uint8_t*) key->subs, key->sub_amt * sizeof(type_ref));
    }
    default: {
      HASH_TYPE h1 = hash_eight_bytes(0, key->tag);
      HASH_TYPE h2 = hash_eight_bytes(h1, key->sub_a);
      return hash_eight_bytes(h2, key->sub_b);
    }
  }
}

static uint32_t hash_stored_type(const void *key_p, const void *ctx_p) {
  type *key = (type*) key_p;
  type_builder *builder = (type_builder*) ctx_p;
  switch (type_repr(key->check_tag)) {
    case SUBS_EXTERNAL: {
      uint32_t hash = hash_eight_bytes(0, key->tag);
      type_ref *subs = &VEC_DATA_PTR(&builder->inds)[key->subs_start];
      return hash_bytes(hash, (uint8_t*) subs, key->sub_amt * sizeof(type_ref));
    }
    default: {
      HASH_TYPE h1 = hash_eight_bytes(0, key->tag);
      HASH_TYPE h2 = hash_eight_bytes(h1, key->sub_a);
      return hash_eight_bytes(h2, key->sub_b);
    }
  }
}

type_builder new_type_builder(void) {
  type_builder type_builder = {
    .types = VEC_NEW,
    .inds = VEC_NEW,
    .type_to_index = ahm_new(type, VEC_LEN_T, cmp_newtype_eq, hash_newtype, hash_stored_type),
    .substitutions = VEC_NEW,
  };
  return type_builder;
}

type_builder new_type_builder_with_builtins(void) {
  type_builder res = new_type_builder();
  // blit builtin types
  VEC_APPEND(&res.types, builtin_type_amount, builtin_types);
  VEC_APPEND(&res.inds, builtin_type_ind_amount, builtin_type_inds);
  for (VEC_LEN_T i = 0; i < res.types.len; i++) {
    ahm_insert_stored(&res.type_to_index, VEC_GET_PTR(res.types, i), &i, &res);
  }
  return res;
}

void free_type_builder(type_builder tb) {
  VEC_FREE(&tb.inds);
  VEC_FREE(&tb.types);
  VEC_FREE(&tb.substitutions);
}
