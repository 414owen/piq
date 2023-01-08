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
    if (memcmp(p1, subs, sub_amt * sizeof(type_ref)) != 0)
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
  ind = find_range(
    VEC_DATA_PTR(&tb->inds), sizeof(subs[0]), tb->inds.len, subs, sub_amt);
  if (ind == tb->inds.len) {
    VEC_APPEND(&tb->inds, sub_amt, subs);
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

static exited_early type_contains_typevar_by(const type_builder *types,
                                             type_ref root, typevar_step step,
                                             const void *data) {
  bool res = false;
  vec_type_ref stack = VEC_NEW;
  VEC_PUSH(&stack, root);
  for (;;) {
    if (stack.len == 0) {
      break;
    }
    type_ref node_ind;
    VEC_POP(&stack, &node_ind);
    type node = VEC_GET(types->types, node_ind);
    switch (node.check_tag) {
      case TC_VAR: {
        var_step_res step_res = step(node.type_var, data);
        if (step_res.early_exit) {
          res = true;
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
    switch (type_repr(node.check_tag)) {
      case SUBS_NONE:
        break;
      case SUBS_TWO:
        VEC_PUSH(&stack, node.sub_b);
        HEDLEY_FALL_THROUGH;
      case SUBS_ONE:
        VEC_PUSH(&stack, node.sub_a);
        break;
      case SUBS_EXTERNAL:
        VEC_APPEND(
          &stack, node.sub_amt, VEC_GET_PTR(types->inds, node.subs_start));
        break;
    }
    break;
  }

end:
  VEC_FREE(&stack);
  return res;
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

static var_step_res is_unsubstituted_typevar(typevar a, const void *data) {
  type_builder *builder = (type_builder *)data;
  type_ref type_ind = VEC_GET(builder->substitutions, a);
  type t = VEC_GET(builder->types, type_ind);
  if (t.check_tag == TC_VAR && a == t.type_var) {
    var_step_res res = {
      .early_exit = true,
    };
    return res;
  }
  var_step_res res = {
    .early_exit = false,
    .redirect_to_next = true,
    .next = type_ind,
  };
  return res;
}

bool type_contains_unsubstituted_typevar(const type_builder *builder,
                                         type_ref root) {
  return type_contains_typevar_by(
    builder, root, is_unsubstituted_typevar, builder);
}

void free_type_builder(type_builder tb) {
  VEC_FREE(&tb.inds);
  VEC_FREE(&tb.types);
  VEC_FREE(&tb.substitutions);
}
