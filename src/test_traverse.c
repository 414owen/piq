// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "builtins.h"
#include "test_upto.h"
#include "traverse.h"
#include "test.h"
#include "util.h"

/*
  When I first starte this test, I checked not only the node type,
  but the node index. That's not really a property we want to test,
  because as long as the tree parsed is correct, we don't are what
  order the nodes are in[1].

  Chances are, testing the type of node is specific enough, but
  we can always add tests that variable names match, or that
  arities/cardinalities match, if we need more granularity.

  [1]: Maybe we should, for performance reasons.
*/

static const char *action_names[] = {
#define mk_entry(a) [a] = #a
  mk_entry(TR_PUSH_SCOPE_VAR),
  mk_entry(TR_PREDECLARE_FN),
  mk_entry(TR_NEW_BLOCK),
  mk_entry(TR_VISIT_IN),
  mk_entry(TR_VISIT_OUT),
  mk_entry(TR_POP_TO),
  mk_entry(TR_END),
  mk_entry(TR_LINK_SIG),
#undef mk_entry
};

typedef struct {
  traverse_action action;
  union {
    parse_node_type_all node_type;
    uint32_t amount;
  } data;
} test_traverse_elem;

static int count_traversal_elems(pt_traversal *traversal) {
  int res = 0;
  pt_traverse_elem a = pt_traverse_next(traversal);
  while (a.action != TR_END) {
    res += 1;
    a = pt_traverse_next(traversal);
  }
  return res;
}

static char *print_ctx(test_traverse_elem *elems, int position) {
  stringstream ss;
  ss_init_immovable(&ss);
  fprintf(ss.stream, "Last five expected elements:\n");
  for (int i = MAX(0, position - 4); i <= position; i++) {
    test_traverse_elem elem = elems[i];
    fprintf(ss.stream, "%d %s", i, action_names[elem.action]);
    switch (elem.action) {
      case TR_PUSH_SCOPE_VAR:
      case TR_VISIT_IN:
      case TR_VISIT_OUT:
      case TR_PREDECLARE_FN:
      case TR_LINK_SIG:
        fprintf(ss.stream, ": %s\n", parse_node_strings[elem.data.node_type]);
        break;
      case TR_POP_TO:
        fprintf(ss.stream, ": %d\n", elem.data.amount);
        break;
      case TR_NEW_BLOCK:
        fputc('\n', ss.stream);
        break;
      case TR_END:
        break;
    }
  }
  ss_finalize(&ss);
  return ss.string;
}

static void traversal_fail(test_state *state, char *err,
                           test_traverse_elem *elems, int position) {
  char *ctx = print_ctx(elems, position);
  failf(state, "At element %d: %s\n%s", position, err, ctx);
  free(ctx);
  free(err);
}

static void node_type_mismatch(test_state *state, int position,
                               test_traverse_elem *elems,
                               parse_node_type_all expected,
                               parse_node_type_all got) {
  traversal_fail(
    state,
    format_to_string("Wrong node type at position %d. Expected: %s, got %s",
                     position,
                     parse_node_strings[expected],
                     parse_node_strings[got]),
    elems,
    position);
}

static void test_elems_match(test_state *state, pt_traversal *traversal,
                             test_traverse_elem *elems, int amount) {
  for (int i = 0; i < amount; i++) {
    pt_traverse_elem a = pt_traverse_next(traversal);
    test_traverse_elem b = elems[i];
    if (a.action == TR_END) {
      traversal_fail(
        state,
        format_to_string(
          "Not enough traversal items. Expected %d, got %d", amount, i),
        elems,
        i);
      return;
    }
    if (a.action != b.action) {
      char *ctx = print_ctx(elems, i);
      traversal_fail(
        state,
        format_to_string(
          "Traversal item mismatch on element %d. Expected '%s', got '%s'",
          i,
          action_names[b.action],
          action_names[a.action]),
        elems,
        i);
      free(ctx);
      return;
    }
    switch (a.action) {
      case TR_PREDECLARE_FN:
      case TR_PUSH_SCOPE_VAR:
      case TR_VISIT_IN:
      case TR_VISIT_OUT:
        if (b.data.node_type != a.data.node_data.node.type.all) {
          node_type_mismatch(
            state, i, elems, b.data.node_type, a.data.node_data.node.type.all);
          return;
        }
        break;
      case TR_POP_TO:
        if (b.data.amount != a.data.new_environment_amount) {
          traversal_fail(state,
                         format_to_string("Wrong environment (pop) amount at "
                                          "position %d. Expected: %d, got %d",
                                          i,
                                          b.data.amount,
                                          a.data.new_environment_amount),
                         elems,
                         i);
          return;
        }
        break;
      case TR_END:
        // impossible
      case TR_NEW_BLOCK:
        break;
      case TR_LINK_SIG: {
        parse_node_type_all at =
          traversal->nodes[a.data.link_sig_data.linked_index].type.all;
        if (at != b.data.node_type) {
          node_type_mismatch(state, i, elems, b.data.node_type, at);
          return;
        }
        break;
      }
    }
  }
  pt_traverse_elem b = pt_traverse_next(traversal);
  if (b.action != TR_END) {
    const int traversal_elem_amt = count_traversal_elems(traversal) + amount;
    failf(state,
          "Too many traversal items. Expected %d, got %d",
          amount,
          traversal_elem_amt + 1);
  }
}

static void test_traversal(test_state *state, const char *input,
                           traverse_mode mode, test_traverse_elem *elems,
                           int amount) {
  parse_tree_res pres = test_upto_parse_tree(state, input);
  if (!pres.success) {
    return;
  }
  pt_traversal traversal = pt_traverse(pres.tree, mode);
  test_elems_match(state, &traversal, elems, amount);
  free_parse_tree_res(pres);
}

static const char *input = "(sig a (Fn I8 (I8, I8) I8))\n"
                           "(fun a (b (c, d))\n"       // env + 4
                           "  (let e (if True 2 1))\n" // env + 5
                           "  (fun f () ())\n"         // env + 6
                           "  (add ((fn () 2) ()) 3))";

#define node_act(_action_type, _node_type)                                     \
  { .action = (_action_type), .data.node_type = (_node_type), }

#define in(_node_type) node_act(TR_VISIT_IN, _node_type)

#define out(_node_type) node_act(TR_VISIT_OUT, _node_type)

#define inout(_node_type) in(_node_type), out(_node_type)

#define add_block                                                              \
  { .action = TR_NEW_BLOCK }

#define predeclare_fn node_act(TR_PREDECLARE_FN, PT_ALL_STATEMENT_FUN)

#define push_env(_node_type) node_act(TR_PUSH_SCOPE_VAR, _node_type)

#define pop_env_to(amt)                                                        \
  { .action = TR_POP_TO, .data.amount = amt, }

#define link_sig(_node_type) node_act(TR_LINK_SIG, _node_type)

static test_traverse_elem print_mode_elems[] = {
  in(PT_ALL_STATEMENT_SIG),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_TY_FN),
  inout(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_TUP),
  inout(PT_ALL_TY_CONSTRUCTOR_NAME),
  inout(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_TY_TUP),
  inout(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_TY_FN),
  out(PT_ALL_STATEMENT_SIG),

  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),
  inout(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_TUP),
  inout(PT_ALL_PAT_WILDCARD),
  inout(PT_ALL_PAT_WILDCARD),
  out(PT_ALL_PAT_TUP),
  in(PT_ALL_EX_FUN_BODY),

  in(PT_ALL_STATEMENT_LET),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_EX_IF),
  inout(PT_ALL_EX_UPPER_NAME),
  inout(PT_ALL_EX_INT),
  inout(PT_ALL_EX_INT),
  out(PT_ALL_EX_IF),
  out(PT_ALL_STATEMENT_LET),

  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_EX_FUN_BODY),
  inout(PT_ALL_EX_UNIT),
  out(PT_ALL_EX_FUN_BODY),
  out(PT_ALL_STATEMENT_FUN),

  in(PT_ALL_EX_CALL),
  inout(PT_ALL_EX_TERM_NAME),
  in(PT_ALL_EX_CALL),
  in(PT_ALL_EX_FN),
  in(PT_ALL_EX_FUN_BODY),
  inout(PT_ALL_EX_INT),
  out(PT_ALL_EX_FUN_BODY),
  out(PT_ALL_EX_FN),
  inout(PT_ALL_EX_UNIT),
  out(PT_ALL_EX_CALL),
  inout(PT_ALL_EX_INT),
  out(PT_ALL_EX_CALL),
  out(PT_ALL_EX_FUN_BODY),
  out(PT_ALL_STATEMENT_FUN),
};

/*
static const char *input = "(sig a (Fn I8 (I8, I8) I8))\n"
                           "(fun a (b (c, d))\n"            // env + 4
                           "  (let e 23)\n"                 // env + 5
                           "  (fun f () ())\n"              // env + 6
                           "  (add ((fn () 2) ()) 3))";
*/

static test_traverse_elem resolve_bindings_elems[] = {
  predeclare_fn,

  in(PT_ALL_STATEMENT_SIG),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_TY_FN),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_TUP),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_STATEMENT_SIG),

  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),

  // params
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_TUP),
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),

  in(PT_ALL_EX_FUN_BODY),

  in(PT_ALL_STATEMENT_LET),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_EX_IF),
  in(PT_ALL_EX_UPPER_NAME),
  in(PT_ALL_EX_INT),
  in(PT_ALL_EX_INT),
  out(PT_ALL_STATEMENT_LET),
  push_env(PT_ALL_STATEMENT_LET),

  predeclare_fn,
  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_EX_FUN_BODY),
  in(PT_ALL_EX_UNIT),
  out(PT_ALL_STATEMENT_FUN),
  pop_env_to(builtin_term_amount + 6),

  in(PT_ALL_EX_CALL),
  in(PT_ALL_EX_TERM_NAME),
  in(PT_ALL_EX_CALL),
  in(PT_ALL_EX_FN),
  in(PT_ALL_EX_FUN_BODY),
  in(PT_ALL_EX_INT),

  // inner lambda, so still has outer fn's params
  pop_env_to(builtin_term_amount + 6),
  in(PT_ALL_EX_UNIT),
  in(PT_ALL_EX_INT),

  // outer function
  out(PT_ALL_STATEMENT_FUN),
  pop_env_to(builtin_term_amount + 1),
};

static test_traverse_elem typecheck_elems[] = {
  predeclare_fn,

  in(PT_ALL_STATEMENT_SIG),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_TY_FN),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_TUP),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  in(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_STATEMENT_SIG),
  link_sig(PT_ALL_STATEMENT_FUN),

  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),

  // params
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_TUP),
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),

  in(PT_ALL_EX_FUN_BODY),

  in(PT_ALL_STATEMENT_LET),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_EX_IF),
  in(PT_ALL_EX_UPPER_NAME),
  in(PT_ALL_EX_INT),
  in(PT_ALL_EX_INT),
  out(PT_ALL_STATEMENT_LET),
  push_env(PT_ALL_STATEMENT_LET),

  predeclare_fn,
  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),
  in(PT_ALL_EX_FUN_BODY),
  in(PT_ALL_EX_UNIT),
  out(PT_ALL_STATEMENT_FUN),
  pop_env_to(builtin_term_amount + 6),

  in(PT_ALL_EX_CALL),
  in(PT_ALL_EX_TERM_NAME),
  in(PT_ALL_EX_CALL),
  in(PT_ALL_EX_FN),
  in(PT_ALL_EX_FUN_BODY),
  in(PT_ALL_EX_INT),

  // inner lambda, so still has outer fn's params
  pop_env_to(builtin_term_amount + 6),
  in(PT_ALL_EX_UNIT),
  in(PT_ALL_EX_INT),

  out(PT_ALL_STATEMENT_FUN),
  pop_env_to(builtin_term_amount + 1),
};

static test_traverse_elem codegen_elems[] = {
  predeclare_fn,

  in(PT_ALL_STATEMENT_SIG),
  inout(PT_ALL_MULTI_TERM_NAME),
  out(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_TY_TUP),
  out(PT_ALL_TY_CONSTRUCTOR_NAME),
  out(PT_ALL_TY_FN),
  out(PT_ALL_STATEMENT_SIG),

  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),

  // params
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_TUP),
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),
  in(PT_ALL_PAT_WILDCARD),
  push_env(PT_ALL_PAT_WILDCARD),

  in(PT_ALL_STATEMENT_LET),
  inout(PT_ALL_MULTI_TERM_NAME),
  out(PT_ALL_EX_UPPER_NAME),
  add_block,
  out(PT_ALL_EX_INT),
  add_block,
  out(PT_ALL_EX_INT),
  out(PT_ALL_EX_IF),
  out(PT_ALL_STATEMENT_LET),
  push_env(PT_ALL_STATEMENT_LET),

  predeclare_fn,
  in(PT_ALL_STATEMENT_FUN),
  inout(PT_ALL_MULTI_TERM_NAME),
  out(PT_ALL_EX_UNIT),
  out(PT_ALL_EX_FUN_BODY),
  out(PT_ALL_STATEMENT_FUN),
  pop_env_to(builtin_term_amount + 6),

  out(PT_ALL_EX_TERM_NAME),
  out(PT_ALL_EX_INT),
  out(PT_ALL_EX_FUN_BODY),
  out(PT_ALL_EX_FN),
  pop_env_to(builtin_term_amount + 6),

  out(PT_ALL_EX_UNIT),
  out(PT_ALL_EX_CALL),
  out(PT_ALL_EX_INT),
  out(PT_ALL_EX_CALL),
  out(PT_ALL_EX_FUN_BODY),
  out(PT_ALL_STATEMENT_FUN),
  pop_env_to(builtin_term_amount + 1),
};

#undef link_sigs
#undef push_env
#undef predeclare_fn
#undef pop_env_to
#undef inout
#undef out
#undef in
#undef node_act

static const int PRINT_ELEM_AMT = STATIC_LEN(print_mode_elems);
static const int RESOLVE_BINDINGS_ELEM_AMT = STATIC_LEN(resolve_bindings_elems);
static const int TYPECHECK_ELEM_AMT = STATIC_LEN(typecheck_elems);
static const int CODEGEN_ELEM_AMT = STATIC_LEN(codegen_elems);

void test_traverse(test_state *state) {
  test_group_start(state, "AST Traversal");

  test_start(state, "Print tree mode");
  test_traversal(
    state, input, TRAVERSE_PRINT_TREE, print_mode_elems, PRINT_ELEM_AMT);
  test_end(state);

  test_start(state, "Resolve bindings");
  test_traversal(state,
                 input,
                 TRAVERSE_RESOLVE_BINDINGS,
                 resolve_bindings_elems,
                 RESOLVE_BINDINGS_ELEM_AMT);
  test_end(state);

  test_start(state, "Typecheck");
  test_traversal(
    state, input, TRAVERSE_TYPECHECK, typecheck_elems, TYPECHECK_ELEM_AMT);
  test_end(state);

  test_start(state, "Codegen");
  test_traversal(
    state, input, TRAVERSE_CODEGEN, codegen_elems, CODEGEN_ELEM_AMT);
  test_end(state);

  test_group_end(state);
}
