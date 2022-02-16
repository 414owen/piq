#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bitset.h"
#include "consts.h"
#include "diagnostic.h"
#include "source.h"
#include "typecheck.h"
#include "vec.h"

// TODO: replace TC_CLONE_LAST with TC_COMBINE

typedef struct {

  enum {
    TC_POP_VARS,
    TC_NODE,
    TC_NOT_WANTED,
    TC_CLONE,
    TC_COMBINE,
  } tag;

  union {
    NODE_IND_T amt;
    NODE_IND_T node_ind;
    struct {
      NODE_IND_T from;
      NODE_IND_T to;
    };
  };

} tc_action;

VEC_DECL(tc_action);

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
} binding;

VEC_DECL(binding);

typedef union {
  char *builtin;
  struct {
    BUF_IND_T start;
    BUF_IND_T end;
  };
} str_ref;

VEC_DECL(str_ref);

typedef struct {
  tc_res res;
  parse_tree tree;
  NODE_IND_T current_node_ind;

  // TODO technically it's more efficient to have a stack of NODE_IND_T,
  // and a separate stack of actions. It would also let us `VEC_APPEND` node
  // inds in the rest of this file.
  vec_tc_action stack;

  // type names in scope
  vec_str_ref type_env;
  // ref to source, or builtin
  bitset type_is_builtin;
  // index into res.types
  vec_node_ind type_type_inds;

  // term names in scope
  vec_str_ref term_env;
  // ref to source, or builtin
  bitset term_is_builtin;
  // index into res.types
  vec_node_ind term_type_inds;
  // res.node_types is wanted or known
  bitset is_wanted;

  source_file source;
} typecheck_state;

// For a known-length, and a null-terminated string
static bool strn1eq(char *a, char *b, size_t alen) {
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

// Find index of binding, return bindings.len if not found
static size_t lookup_bnd(typecheck_state *state, vec_str_ref bnds,
                         bitset is_builtin, binding bnd) {
  size_t len = bnd.end - bnd.start + 1;
  char *bndp = state->source.data + bnd.start;
  for (size_t i = 0; i < bnds.len; i++) {
    size_t ind = bnds.len - 1 - i;
    str_ref a = bnds.data[ind];
    if (bs_get(is_builtin, ind)) {
      if (strn1eq(bndp, a.builtin, len)) {
        return ind;
      }
    } else {
      if (a.end - a.start != len)
        continue;
      if (strncmp(bndp, state->source.data + a.start, len) == 0)
        return ind;
    }
  }
  return bnds.len;
}

static void give_up(char *err) {
  fprintf(stderr, "%s.\nThis is a compiler bug! Giving up.", err);
  exit(1);
}

// Add builtin types and custom types to type env
static void setup_type_env(typecheck_state *state) {

  NODE_IND_T unknown_ind;
  {
    type t = {.tag = T_UNKNOWN, .sub_amt = 0, .sub_start = 0};
    VEC_PUSH(&state->res.types, t);
    unknown_ind = state->res.types.len - 1;
  }

  // Nodes have initial type UNKNOWN and not is_wanted
  VEC_REPLICATE(&state->res.node_types, state->res.tree.nodes.len, unknown_ind);
  bs_resize(&state->is_wanted, state->res.tree.nodes.len);
  state->is_wanted.len = state->res.tree.nodes.len;
  memset(&state->is_wanted.data[0], 0, state->is_wanted.cap);

  // Prelude
  {
    static const char *builtin_names[] = {
      "U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64",
    };

    static const type_tag builtin_types[] = {T_U8, T_U16, T_U32, T_U64,
                                             T_I8, T_I16, T_I32, T_I64};

    VEC_APPEND(&state->type_env, STATIC_LEN(builtin_names), builtin_names);
    NODE_IND_T start_type_ind = state->res.types.len;
    for (NODE_IND_T i = 0; i < STATIC_LEN(builtin_types); i++) {
      VEC_PUSH(&state->type_type_inds, start_type_ind + i);
      type t = {.tag = builtin_types[i], .sub_start = 0, .sub_amt = 0};
      VEC_PUSH(&state->res.types, t);
      bs_push(&state->type_is_builtin, true);
    }
  }

  // Declared here
  {
    for (size_t i = 0; i < state->res.tree.nodes.len; i++) {
      parse_node node = state->res.tree.nodes.data[i];
      switch (node.type) {
        // TODO fill in user declared types here
        default:
          break;
      }
    }
  }
}

static void check_int_fits(typecheck_state *state, parse_node node,
                           uint64_t max) {
  char *buf = state->source.data;
  uint64_t last, n = 0;
  for (size_t i = 0; i <= node.end; i++) {
    last = n;
    char digit = buf[node.end - i];
    n *= 10;
    n += digit - '0';
    if (n < last) {
      tc_error err = {
        .type = INT_LARGER_THAN_MAX,
        .pos = state->current_node_ind,
      };
      VEC_PUSH(&state->res.errors, err);
      break;
    } else if (n > max) {
      tc_error err = {
        .type = INT_LARGER_THAN_MAX,
        .pos = state->current_node_ind,
      };
      VEC_PUSH(&state->res.errors, err);
      break;
    }
  }
}

typedef struct {
  NODE_IND_T a;
  NODE_IND_T b;
} node_ind_tup;

VEC_DECL(node_ind_tup);

bool types_equal(typecheck_state *state, NODE_IND_T a, NODE_IND_T b) {
  if (a == b)
    return true;

  vec_node_ind_tup stack = VEC_NEW;
  node_ind_tup initial = {a, b};
  VEC_PUSH(&stack, initial);

  while (stack.len > 0) {
    node_ind_tup tup = VEC_POP(&stack);
    if (tup.a == tup.b)
      continue;
    type ta = state->res.types.data[tup.a];
    type tb = state->res.types.data[tup.b];
    if (ta.tag != tb.tag || ta.sub_amt != tb.sub_amt) {
      VEC_FREE(&stack);
      return false;
    }
    for (size_t i = 0; i < ta.sub_amt; i++) {
      node_ind_tup new_tup = {ta.sub_start + i, tb.sub_amt + i};
      VEC_PUSH(&stack, new_tup);
    }
  }
  VEC_FREE(&stack);
  return true;
}

static size_t prim_ind(typecheck_state *state, type_tag tag) {
  for (size_t i = 0; i < state->type_env.len; i++) {
    NODE_IND_T type_ind =
      state->type_type_inds.data[state->type_type_inds.len - 1 - i];
    if (state->res.types.data[type_ind].tag == tag) {
      return type_ind;
    }
  }
  give_up("Can't find primitive type!");
  return 0;
}

static binding node_to_binding(parse_node node) {
  binding b = {.start = node.start, .end = node.end};
  return b;
}

static void tc_node(typecheck_state *state) {

  NODE_IND_T node_ind = state->current_node_ind;
  parse_node node = state->res.tree.nodes.data[node_ind];
  bool is_wanted = bs_get(state->is_wanted, node_ind);
  NODE_IND_T type_ind = state->res.node_types.data[node_ind];
  type t = state->res.types.data[type_ind];

  switch (node.type) {
    case PT_TYPED: {
      NODE_IND_T val_node_ind = state->res.tree.inds.data[node.subs_start];
      NODE_IND_T type_node_ind = state->res.tree.inds.data[node.subs_start + 1];

#define action_amt 3
      tc_action actions[action_amt] = {
        {.tag = TC_NODE, .node_ind = type_node_ind},
        {.tag = TC_CLONE, .from = type_node_ind, .to = val_node_ind},
        {.tag = TC_CLONE, .from = type_node_ind, .to = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }
    case PT_TUP: {
      if (is_wanted) {
        if (t.tag != T_TUP || t.sub_amt != node.sub_amt) {
          tc_error err = {
            .type = LITERAL_MISMATCH,
            .pos = node_ind,
            .expected = type_ind,
          };
          VEC_PUSH(&state->res.errors, err);
        }
      }
      for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
        tc_action action = {
          .tag = TC_NODE,
          .node_ind = state->res.tree.inds.data[node.subs_start + i],
        };
        VEC_PUSH(&state->stack, action);
      }
      tc_action action = {.tag = TC_COMBINE, .node_ind = node_ind};
      VEC_PUSH(&state->stack, action);
      break;
    }
    case PT_IF: {
      NODE_IND_T cond = state->res.tree.inds.data[node.subs_start];
      NODE_IND_T b1 = state->res.tree.inds.data[node.subs_start + 1];
      NODE_IND_T b2 = state->res.tree.inds.data[node.subs_start + 2];

      // TODO benchmark: could skip if not is_wanted
      state->is_wanted.data[cond] = true;
      state->res.node_types.data[cond] = prim_ind(state, T_BOOL);

      // first branch wanted == overall wanted
      state->is_wanted.data[b1] = state->is_wanted.data[node_ind];
      state->res.node_types.data[b1] = state->res.node_types.data[node_ind];

      // second branch wanted == first branch
      state->is_wanted.data[b2] = true;

#define action_amt 5
      const tc_action actions[action_amt] = {
        {.tag = TC_NODE, .node_ind = cond},
        {.tag = TC_NODE, .node_ind = b1},
        // second branch wanted == first branch
        {.tag = TC_CLONE, .from = b1, .to = b2},
        {.tag = TC_NODE, .node_ind = b2},
        {.tag = TC_COMBINE, .node_ind = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }
    case PT_INT: {
      if (is_wanted) {
        switch (t.tag) {
          case T_U8:
            check_int_fits(state, node, UINT8_MAX);
            break;
          case T_U16:
            check_int_fits(state, node, UINT16_MAX);
            break;
          case T_U32:
            check_int_fits(state, node, UINT32_MAX);
            break;
          case T_U64:
            check_int_fits(state, node, UINT64_MAX);
            break;
          case T_I8:
            check_int_fits(state, node, INT8_MAX);
            break;
          case T_I16:
            check_int_fits(state, node, INT16_MAX);
            break;
          case T_I32:
            check_int_fits(state, node, INT32_MAX);
            break;
          case T_I64:
            check_int_fits(state, node, INT64_MAX);
            break;
          default: {
            tc_error err = {
              .type = LITERAL_MISMATCH,
              .pos = node_ind,
              .expected = type_ind,
            };
            VEC_PUSH(&state->res.errors, err);
            break;
          }
        }
      } else {
        tc_error err = {
          .type = AMBIGUOUS_TYPE,
          .pos = node_ind,
        };
        VEC_PUSH(&state->res.errors, err);
      }
      break;
    }
    case PT_TOP_LEVEL: {
      give_up("Top level node as child of non-root");
      break;
    }
    case PT_ROOT: {
      // TODO for mutually recursive functions, we need the function names to be
      // in scope, so we need to reserve type nodes and such
      for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
        parse_node toplevel = state->res.tree.nodes.data[node.subs_start + i];
        assert(toplevel.type == PT_TOP_LEVEL);
        NODE_IND_T name_ind = state->res.tree.inds.data[toplevel.subs_start];
        NODE_IND_T val_ind = state->res.tree.inds.data[toplevel.subs_start + 1];
        state->current_node_ind = val_ind;
        parse_node val = state->res.tree.nodes.data[val_ind];
        switch (val.type) {
          case PT_FN: {
#define action_amt 2
            tc_action actions[action_amt] = {
              {.tag = TC_NODE, .node_ind = val_ind},
              {.tag = TC_CLONE, .from = val_ind, .to = name_ind},
            };
            VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
            break;
          }
          default:
            give_up("At the moment only functions can be top levels");
            break;
        }
      }
      break;
    }
    case PT_LOWER_NAME: {
      binding b = node_to_binding(node);
      size_t ind =
        lookup_bnd(state, state->term_env, state->term_is_builtin, b);
      if (ind == state->term_env.len) {
        tc_error err = {
          .type = BINDING_NOT_FOUND,
          .pos = node_ind,
        };
        VEC_PUSH(&state->res.errors, err);
        break;
      }
      NODE_IND_T term_ind = state->term_type_inds.data[ind];
      state->res.node_types.data[node_ind] = term_ind;
      break;
    }
    case PT_UPPER_NAME: {
      binding b = {.start = node.start, .end = node.end};
      size_t ind =
        lookup_bnd(state, state->type_env, state->type_is_builtin, b);
      NODE_IND_T type_ind = state->type_type_inds.data[ind];
      if (ind == state->type_env.len) {
        tc_error err = {
          .type = TYPE_NOT_FOUND,
          .pos = node_ind,
        };
        VEC_PUSH(&state->res.errors, err);
      }
      state->res.node_types.data[node_ind] = type_ind;
      break;
    }
    case PT_FN: {
      for (uint16_t i = 0; i < (node.sub_amt - 2) / 2; i++) {
        NODE_IND_T type_ind =
          state->res.tree.inds.data[node.subs_start + i * 2 + 1];
        NODE_IND_T param_ind =
          state->res.tree.inds.data[node.subs_start + i * 2];
#define action_amt 2
        const tc_action actions[action_amt] = {
          {.tag = TC_NODE, .node_ind = type_ind},
          {.tag = TC_CLONE, .from = type_ind, .to = param_ind},
        };
        VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      }
      NODE_IND_T ret_type_ind =
        state->res.tree.inds.data[node.subs_start + node.sub_amt - 2];
      NODE_IND_T body_ind =
        state->res.tree.inds.data[node.subs_start + node.sub_amt - 1];
      bs_set(&state->is_wanted, body_ind, true);
#define action_amt 4
      const tc_action actions[action_amt] = {
        {.tag = TC_NODE, .node_ind = ret_type_ind},
        {.tag = TC_CLONE, .from = ret_type_ind, .to = body_ind},
        {.tag = TC_NODE, .node_ind = body_ind},
        {.tag = TC_COMBINE, .node_ind = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }
    case PT_CALL: {

      NODE_IND_T callee_ind = state->res.tree.inds.data[node.subs_start];
      type fn_type = state->res.types.data[callee_ind];
      if (fn_type.tag == T_UNKNOWN) {
        VEC_PUSH(&state->stack, callee_ind);
        VEC_PUSH(&state->stack, node_ind);
        break;
      }

      NODE_IND_T exp_param_amt = fn_type.sub_amt - 1;
      NODE_IND_T got_param_amt = node.sub_amt - 1;
      if (exp_param_amt != got_param_amt) {
        tc_error err = {
          .type = WRONG_ARITY,
          .pos = node_ind,
          .exp_param_amt = exp_param_amt,
          .got_param_amt = got_param_amt,
        };
        VEC_PUSH(&state->res.errors, err);
      }

      for (size_t i = 1; i < node.sub_amt; i++) {
        // VEC_PUSH(&state.stack, );
      }
    }
  }

  tc_action action = {.tag = TC_NOT_WANTED, .node_ind = node_ind};
  VEC_PUSH(&state->stack, action);
}

static typecheck_state tc_new_state(source_file source, parse_tree tree) {
  typecheck_state state = {
    .res = {.types = VEC_NEW,
            .errors = VEC_NEW,
            .source = source,
            .tree = tree},
    .is_wanted = bs_new(),

    .type_env = VEC_NEW,
    .type_is_builtin = bs_new(),
    .type_type_inds = VEC_NEW,

    .term_env = VEC_NEW,
    .term_is_builtin = bs_new(),
    .term_type_inds = VEC_NEW,

    .stack = VEC_NEW,

    .source = source,
  };
  tc_action action = {.tag = TC_NODE, .node_ind = tree.root_ind};
  VEC_PUSH(&state.stack, action);
  return state;
}

static void tc_combine(typecheck_state *state) {
  NODE_IND_T node_ind = state->current_node_ind;
  parse_node node = state->res.tree.nodes.data[node_ind];
  NODE_IND_T param_amt = (node.sub_amt - 2) / 2;
  switch (node.type) {
    case PT_FN: {

      for (size_t i = 0; i < state->res.types.len; i++) {
        type t = state->res.types.data[i];
        if (t.tag != T_FN)
          continue;
        if (t.sub_amt != param_amt + 1)
          continue;
        bool match = true;
        for (size_t j = 0; j < param_amt; j++) {
          NODE_IND_T sub_node_ind =
            state->res.tree.inds.data[node.subs_start + j * 2 + 1];
          if (state->type_type_inds.data[t.sub_amt + j] !=
              state->res.node_types.data[sub_node_ind]) {
            match = false;
            break;
          }
        }
        if (!match)
          continue;
        state->res.node_types.data[node_ind] = i;
        return;
      }

      NODE_IND_T sub_start = state->res.type_inds.len;
      for (NODE_IND_T i = 0; i < param_amt; i++) {
        VEC_PUSH(&state->res.type_inds,
                 state->res.node_types.data[node.subs_start + i * 2 + 1]);
      }
      VEC_PUSH(&state->res.type_inds,
               state->res.node_types.data[node.start + node.sub_amt - 1]);
      type t = {.sub_start = sub_start, .sub_amt = param_amt + 1, .tag = T_FN};
      VEC_PUSH(&state->res.types, t);
      state->res.node_types.data[state->current_node_ind] =
        state->res.types.len - 1;
      break;
    }
    case PT_TUP: {
      for (size_t i = 0; i < state->res.types.len; i++) {
        type t = state->res.types.data[i];
        if (t.tag != T_TUP)
          continue;
        if (t.sub_amt != node.sub_amt)
          continue;
        bool match = true;
        for (size_t j = 0; j < param_amt; j++) {
          NODE_IND_T sub_node_ind =
            state->res.tree.inds.data[node.subs_start + j];
          if (state->type_type_inds.data[t.sub_amt + j] !=
              state->res.node_types.data[sub_node_ind]) {
            match = false;
            break;
          }
        }
        if (!match)
          continue;
        state->res.node_types.data[node_ind] = i;
        return;
      }

      NODE_IND_T sub_start = state->res.type_inds.len;
      VEC_APPEND(&state->res.type_inds, node.sub_amt,
                 &state->res.node_types.data[node.subs_start]);
      type t = {.sub_start = sub_start, .sub_amt = node.sub_amt, .tag = T_TUP};
      VEC_PUSH(&state->res.types, t);
      state->res.node_types.data[state->current_node_ind] =
        state->res.types.len - 1;
      break;
    }
    default:
      give_up("Can't combine type");
      break;
  }
}

static const char *const action_str[] = {
  [TC_NODE] = "TC Node",      [TC_COMBINE] = "Combine",
  [TC_CLONE] = "Clone",       [TC_NOT_WANTED] = "Not wanted",
  [TC_POP_VARS] = "Pop vars",
};

tc_res typecheck(source_file source, parse_tree tree) {
  typecheck_state state = tc_new_state(source, tree);
  setup_type_env(&state);
  // resolve_types(&state);
#ifdef DEBUG_TC
  FILE *debug_out = fopen("debug-typechecker", "w");
#endif

  while (state.stack.len > 0) {
    tc_action action = VEC_POP(&state.stack);
    size_t actions_start = state.stack.len;

#ifdef DEBUG_TC
    {
      fprintf(debug_out, "\nAction: '%s'\n", action_str[action.tag]);
      switch (action.tag) {
      TC_POP_VARS:
        break;
        default: {
          parse_node node = tree.nodes.data[action.node_ind];
          fprintf(debug_out, "Node: '%s'\n", parse_node_strings[node.type]);
          format_error_ctx(debug_out, source.data, node.start, node.end);
          break;
        }
      }
    }
    size_t starting_err_amt = state.res.errors.len;
#endif

    switch (action.tag) {
      case TC_NOT_WANTED: {
        bs_set(&state.is_wanted, action.node_ind, false);
        break;
      }
      case TC_CLONE:
        state.res.node_types.data[action.to] =
          state.res.node_types.data[action.from];
        break;
      case TC_COMBINE:
        state.current_node_ind = action.node_ind;
        tc_combine(&state);
        break;
      case TC_POP_VARS:
        state.term_env.len -= action.amt;
        break;
      case TC_NODE:
        state.current_node_ind = action.node_ind;
        tc_node(&state);
        break;
    }

#ifdef DEBUG_TC
    if (state.res.errors.len > starting_err_amt) {
      fprintf(debug_out, "Caused an error.\n", action_str[action.tag]);
    }
#endif

    reverse_arbitrary(&state.stack.data[actions_start],
                      MAX(actions_start, state.stack.len) - actions_start,
                      sizeof(tc_action));
  }

  VEC_FREE(&state.stack);

  VEC_FREE(&state.type_env);
  bs_free(&state.type_is_builtin);
  VEC_FREE(&state.type_type_inds);

  VEC_FREE(&state.term_env);
  bs_free(&state.term_is_builtin);
  VEC_FREE(&state.term_type_inds);
  bs_free(&state.is_wanted);

#ifdef DEBUG_TC
  fclose(debug_out);
#endif

  return state.res;
}
