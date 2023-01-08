#include <stdlib.h>

#include "ast_meta.h"
#include "bitset.h"
#include "builtins.h"
#include "consts.h"
#include "parse_tree.h"
#include "term.h"
#include "typecheck.h"

// Typechecking and inference based on wand's algorithm
//
// Stage one: Annotate tree
// In theory, this is a no-op for us, as we use the node's index as the type
// variable, however in practice, we create a T_VAR(parse_tree_index) type for
// every node, just so that we can easily reuse them in the constraint
// generation stage.
//
// Stage two: Generate constraints
// Our constraints are of the form:
// <type> = <type>
// where <type> can contain type variables
//
// Stage three: Unification
// This amounts to solving a system of equations

typedef struct {
  type_ref a;
  type_ref b;
  node_ind_t provenance;
} tc_constraint;

VEC_DECL(tc_constraint);

typedef struct {
  const parse_tree tree;
  // const typevar *parse_node_type_vars;
  vec_tc_constraint constraints;
  type_builder *type_builder;
  vec_type_ref environment;
  vec_type_ref type_environment;
} tc_constraint_builder;

typedef vec_tc_constraint tc_constraints_res;

static void annotate_parse_tree(const parse_tree tree, type_builder *builder) {
  // things that cause fresh type variables to be generated:
  // * List expressions with zero elements
  // * That's it
  //
  // let's estimate there will be one of them every 100 nodes...
  VEC_RESERVE(&builder->substitutions, tree.node_amt + tree.node_amt / 100);
  builder->substitutions.len = tree.node_amt;
  for (node_ind_t i = 0; i < tree.node_amt; i++) {
    VEC_DATA_PTR(&builder->substitutions)[i] = mk_type_var(builder, i);
  }
}

static void add_type_constraint(tc_constraint_builder *builder, type_ref a,
                                type_ref b, node_ind_t provenance) {
  tc_constraint constraint = {
    .a = a,
    .b = b,
    .provenance = provenance
  };
  VEC_PUSH(&builder->constraints, constraint);
}

static void
generate_constraints_push_environment(tc_constraint_builder *builder,
                                      node_ind_t node_ind) {
  // remember, node index is used as type variable
  VEC_PUSH(&builder->environment,
           VEC_GET(builder->type_builder->node_types, node_ind));
}

static type_ref generate_node_type(tc_constraint_builder *builder,
                                   node_ind_t node_ind) {
  return VEC_GET(builder->type_builder->node_types, node_ind);
}

static void generate_constraints_visit(tc_constraint_builder *builder,
                                       pt_trav_elem_data elem) {
  const parse_node node = elem.node;
  const node_ind_t node_ind = elem.node_index;
  const type_ref our_type = generate_node_type(builder, node_ind);
  switch (node.type.all) {
    // The callee is a function whose parameters have the same type as
    // the call parameters, and that returns the same type as this call
    // expression
    case PT_ALL_EX_CALL: {
      const node_ind_t callee_ind =
        PT_CALL_CALLEE_IND(builder->tree.inds, node);
      const type_ref callee_type = generate_node_type(builder, callee_ind);
      const node_ind_t num_params = PT_CALL_PARAM_AMT(elem.node);
      type_ref *fn_type_inds = stalloc(sizeof(type_ref) * (num_params + 1));
      for (node_ind_t i = 0; i < num_params; i++) {
        node_ind_t param_ind =
          PT_CALL_PARAM_IND(builder->tree.inds, elem.node, i);
        fn_type_inds[i] = generate_node_type(builder, param_ind);
      }
      fn_type_inds[num_params] = generate_node_type(builder, node_ind);
      const type_ref fn_type =
        mk_type(builder->type_builder, TC_FN, fn_type_inds, num_params + 1);
      add_type_constraint(builder, callee_type, fn_type, node_ind);
      stfree(fn_type_inds, sizeof(type_ref) * num_params);
      break;
    }
    case PT_ALL_TY_FN:
    case PT_ALL_EX_FN: {
      const node_ind_t num_params = PT_FN_PARAM_AMT(node);
      type_ref *fn_type_inds = stalloc(sizeof(type_ref) * (num_params + 1));
      for (node_ind_t i = 0; i < num_params; i++) {
        const node_ind_t param_ind =
          PT_FN_PARAM_IND(builder->tree.inds, elem.node, i);
        fn_type_inds[i] = generate_node_type(builder, param_ind);
      }
      const node_ind_t body_ind = PT_FN_BODY_IND(builder->tree.inds, node);
      const type_ref body_type = generate_node_type(builder, body_ind);
      fn_type_inds[num_params] = body_type;
      const type_ref fn_type =
        mk_type(builder->type_builder, TC_FN, fn_type_inds, num_params + 1);
      add_type_constraint(builder, our_type, fn_type, node_ind);
      stfree(fn_type_inds, sizeof(type_ref) * (num_params + 1));
      break;
    }
    case PT_ALL_EX_UPPER_VAR:
    case PT_ALL_EX_TERM_NAME: {
      const type_ref target_type =
        VEC_GET(builder->environment, node.variable_index);
      add_type_constraint(builder, our_type, target_type, node_ind);
      break;
    }
    case PT_ALL_TY_CONSTRUCTOR_NAME:
    case PT_ALL_TY_LOWER_NAME: {
      const type_ref target_type =
        VEC_GET(builder->type_environment, node.variable_index);
      add_type_constraint(builder, our_type, target_type, node_ind);
      break;
    }
    case PT_ALL_EX_FUN_BODY: {
      const node_ind_t last_stmt_ind =
        PT_FUN_BODY_LAST_SUB_IND(builder->tree.inds, node);
      const type_ref last_stmt_type =
        generate_node_type(builder, last_stmt_ind);
      add_type_constraint(builder, our_type, last_stmt_type, node_ind);
      break;
    }
    case PT_ALL_EX_IF: {
      const node_ind_t cond_ind = PT_IF_COND_IND(builder->tree.inds, node);
      const node_ind_t branch_b_ind = PT_IF_B_IND(builder->tree.inds, node);
      const node_ind_t branch_a_ind = PT_IF_A_IND(builder->tree.inds, node);
      const type_ref cond_type = generate_node_type(builder, cond_ind);
      const type_ref branch_b_type = generate_node_type(builder, branch_b_ind);
      const type_ref branch_a_type = generate_node_type(builder, branch_a_ind);
      const type_ref bool_type =
        mk_primitive_type(builder->type_builder, TC_BOOL);
      add_type_constraint(builder, cond_type, bool_type, node_ind);
      add_type_constraint(builder, our_type, branch_b_type, node_ind);
      add_type_constraint(builder, our_type, branch_a_type, node_ind);
      break;
    }
    case PT_ALL_PAT_INT:
    case PT_ALL_EX_INT:
      break;
    case PT_ALL_EX_AS: {
      const node_ind_t expression_ind = PT_AS_VAL_IND(node);
      const node_ind_t type_ind = PT_AS_TYPE_IND(node);
      const type_ref expression_type =
        generate_node_type(builder, expression_ind);
      const type_ref type_type = generate_node_type(builder, type_ind);
      add_type_constraint(builder, our_type, expression_type, node_ind);
      add_type_constraint(builder, our_type, type_type, node_ind);
      break;
    }
    case PT_ALL_PAT_LIST:
    case PT_ALL_EX_LIST: {
      const node_ind_t sub_amt = PT_LIST_SUB_AMT(node);
      type_ref sub_type;
      if (sub_amt > 0) {
        const node_ind_t first_sub_ind =
          PT_LIST_SUB_IND(builder->tree.inds, node, 0);
        sub_type = generate_node_type(builder, first_sub_ind);
        for (node_ind_t i = 1; i < sub_amt; i++) {
          const node_ind_t sub_ind =
            PT_LIST_SUB_IND(builder->tree.inds, node, i);
          const type_ref sub_type_b = generate_node_type(builder, sub_ind);
          add_type_constraint(builder, sub_type, sub_type_b, node_ind);
        }
      } else {
        sub_type = mk_type_var(builder->type_builder,
                               builder->type_builder->substitutions.len);
        VEC_PUSH(&builder->type_builder->substitutions, sub_type);
      }
      const type_ref list_type =
        mk_type_inline(builder->type_builder, TC_LIST, sub_type, 0);
      add_type_constraint(builder, our_type, list_type, node_ind);
      break;
    }
    case PT_ALL_PAT_STRING:
    case PT_ALL_EX_STRING: {
      const type_ref u8_type =
        mk_primitive_type(builder->type_builder, TC_U8);
      const type_ref string_type =
        mk_type_inline(builder->type_builder, TC_LIST, u8_type, 0);
      add_type_constraint(builder, our_type, string_type, node_ind);
      break;
    }
    case PT_ALL_TY_TUP:
    case PT_ALL_PAT_TUP:
    case PT_ALL_EX_TUP: {
      const node_ind_t sub_a_ind = PT_TUP_SUB_A(node);
      const node_ind_t sub_b_ind = PT_TUP_SUB_B(node);
      const type_ref sub_a_type = generate_node_type(builder, sub_a_ind);
      const type_ref sub_b_type = generate_node_type(builder, sub_b_ind);
      const type_ref tup_type = mk_type_inline(
        builder->type_builder, TC_TUP, sub_a_type, sub_b_type);
      add_type_constraint(builder, our_type, tup_type, node_ind);
      break;
    }
    case PT_ALL_TY_UNIT:
    case PT_ALL_PAT_UNIT:
    case PT_ALL_EX_UNIT: {
      const type_ref unit_type =
        mk_primitive_type(builder->type_builder, TC_UNIT);
      add_type_constraint(builder, our_type, unit_type, node_ind);
      break;
    }
    case PT_ALL_PAT_WILDCARD:
    case PT_ALL_MULTI_UPPER_NAME:
    case PT_ALL_MULTI_TERM_NAME: {
      // This isn't something we'll be checking
      break;
    }
    case PT_ALL_STATEMENT_SIG: {
      const node_ind_t sig_type_ind = PT_SIG_TYPE_IND(node);
      const node_ind_t sig_binding_ind = PT_SIG_BINDING_IND(node);
      const type_ref sig_type_type = generate_node_type(builder, sig_type_ind);
      const type_ref sig_binding_type =
        generate_node_type(builder, sig_binding_ind);
      add_type_constraint(builder, our_type, sig_binding_type, node_ind);
      add_type_constraint(builder, our_type, sig_type_type, node_ind);
      break;
    }
    case PT_ALL_STATEMENT_FUN: {
      const node_ind_t num_params = PT_FUN_PARAM_AMT(node);
      const node_ind_t body_ind = PT_FUN_BODY_IND(builder->tree.inds, node);
      const node_ind_t binding_ind =
        PT_FUN_BINDING_IND(builder->tree.inds, node);
      type_ref *fn_type_inds = stalloc(sizeof(type_ref) * (num_params + 1));
      for (node_ind_t i = 0; i < num_params; i++) {
        const node_ind_t param_ind =
          PT_FUN_PARAM_IND(builder->tree.inds, elem.node, i);
        fn_type_inds[i] = generate_node_type(builder, param_ind);
      }
      const type_ref body_type = generate_node_type(builder, body_ind);
      fn_type_inds[num_params] = body_type;
      const type_ref fn_type =
        mk_type(builder->type_builder, TC_FN, fn_type_inds, num_params + 1);
      const type_ref binding_type = generate_node_type(builder, binding_ind);
      add_type_constraint(builder, our_type, fn_type, node_ind);
      add_type_constraint(builder, our_type, binding_type, node_ind);
      stfree(fn_type_inds, sizeof(type_ref) * (num_params + 1));
      break;
    }
    case PT_ALL_STATEMENT_LET: {
      const node_ind_t binding_ind = PT_LET_BND_IND(node);
      const node_ind_t val_ind = PT_LET_VAL_IND(node);
      const type_ref binding_type = generate_node_type(builder, binding_ind);
      const type_ref val_type = generate_node_type(builder, val_ind);
      add_type_constraint(builder, our_type, binding_type, node_ind);
      add_type_constraint(builder, our_type, val_type, node_ind);
      break;
    }
    case PT_ALL_TY_CONSTRUCTION:
    case PT_ALL_MULTI_TYPE_PARAMS:
    case PT_ALL_MULTI_DATA_CONSTRUCTOR_DECL:
    case PT_ALL_MULTI_DATA_CONSTRUCTORS:
    case PT_ALL_PAT_CONSTRUCTION:
    case PT_ALL_STATEMENT_DATA_DECLARATION:
      break;
    case PT_ALL_TY_LIST: {
      const node_ind_t sub_ind = PT_LIST_TYPE_SUB(node);
      const type_ref sub_type = generate_node_type(builder, sub_ind);
      const type_ref list_type =
        mk_type_inline(builder->type_builder, TC_LIST, sub_type, 0);
      add_type_constraint(builder, our_type, list_type, node_ind);
      break;
    }
  }
}

static void generate_constraints_link_sigs(tc_constraint_builder *builder,
                                           pt_trav_elem_data elem) {
  const type_ref sig_type =
    VEC_GET(builder->type_builder->node_types, elem.sig_index);
  const type_ref linked_type =
    VEC_GET(builder->type_builder->node_types, elem.linked_index);
  add_type_constraint(builder, sig_type, linked_type, elem.node_index);
}

// I think that, for these to be solved, we have to generate constraints like
// this: a == b, b == c instead of: a == b, a == c
static tc_constraints_res generate_constraints(const parse_tree tree,
                                               type_builder *type_builder) {

  tc_constraint_builder builder = {
    .tree = tree,
    .constraints = VEC_NEW,
    .type_builder = type_builder,
    .environment = VEC_NEW,
    .type_environment = VEC_NEW,
  };

  // add builtin types to type environment
  for (node_ind_t i = 0; i < named_builtin_type_amount; i++) {
    // the index of the name in the environment is also the index of the type
    // in builtin_types[], and hence our encironment
    VEC_PUSH(&builder.type_environment, i);
  }

  VEC_APPEND(&builder.environment, builtin_term_amount, builtin_type_inds);

  pt_traversal traversal = pt_scoped_traverse(tree);
  pt_traverse_elem pt_trav_elem;

  for (;;) {
    pt_trav_elem = pt_scoped_traverse_next(&traversal);
    switch (pt_trav_elem.action) {
      case TR_PUSH_SCOPE_VAR:
        generate_constraints_push_environment(&builder,
                                              pt_trav_elem.data.node_index);
        continue;
      case TR_VISIT_IN:
        generate_constraints_visit(&builder, pt_trav_elem.data);
        continue;
      case TR_POP_TO:
        builder.environment.len = pt_trav_elem.data.new_environment_amount;
        continue;
      case TR_LINK_SIG:
        generate_constraints_link_sigs(&builder, pt_trav_elem.data);
        continue;
      case TR_END:
        break;
    }
    break;
  }

  VEC_REVERSE(&builder.constraints);
  VEC_FREE(&builder.environment);
  VEC_FREE(&builder.type_environment);
  return builder.constraints;
}

typedef enum {
  U_SUCCESS,
  U_CONFLICT,
  U_INFINITE,
} unification_result_tag;

typedef struct {
  type_builder *types;
  vec_tc_constraint constraints;
  vec_tc_error errors;
#ifdef DEBUG_TC
  parse_tree tree;
#endif
} unification_state;

static void unify_typevar(unification_state *state, typevar a, type_ref b_ind,
                          type b) {
  if (b.check_tag == TC_VAR && a == b.type_var) {
    return;
  }
  if (type_contains_specific_typevar(state->types, b_ind, a)) {
    tc_error err = {
      .type = TC_ERR_INFINITE,
      .infinite =
        {
          .index = b_ind,
        },
    };
    VEC_PUSH(&state->errors, err);
    return;
  }
  VEC_DATA_PTR(&state->types->substitutions)[a] = b_ind;
#ifdef DEBUG_TC
  printf("Typevar %d := ", a);
  print_type(stdout, VEC_DATA_PTR(&state->types->types), VEC_DATA_PTR(&state->types->inds), b_ind);
  putc('\n', stdout);
  if (a < state->tree.node_amt) {
    if (b.check_tag == TC_VAR) {
      printf(RED "%s" RESET " := " RED "%s" RESET "\n",
        parse_node_strings[state->tree.nodes[a].type.all],
        parse_node_strings[state->tree.nodes[b.type_var].type.all]);
    } else {
      printf(RED "%s" RESET "\n", parse_node_strings[state->tree.nodes[a].type.all]);
    }
  }
#endif
}

static void add_conflict(vec_tc_error *errs, tc_constraint c) {
  tc_error err = {
    .type = TC_ERR_CONFLICT,
    .pos = c.provenance,
    .conflict =
      {
        .expected_ind = c.a,
        .got_ind = c.b,
      },
  };
#ifdef DEBUG_TC
  puts("Added conflict");
#endif
  VEC_PUSH(errs, err);
}

static bool get_substitute_layer(type_builder *types, typevar t,
                                 type_ref *res) {
  type_ref substitution_ind = VEC_GET(types->substitutions, t);
  type substitution = VEC_GET(types->types, substitution_ind);
  *res = substitution_ind;
  return substitution.check_tag != TC_VAR || substitution.type_var != t;
}

// This functions follows the substitution chain to the end
// and also reduces any chains to chains of one
static node_ind_t resolve_type(type_builder *types, type_ref root_ind) {
  type_ref target = root_ind;
  {
    type_ref type_ind = root_ind;
    for (;;) {
      type a = VEC_GET(types->types, type_ind);
      if (a.tag == TC_VAR && get_substitute_layer(types, a.type_var, &target)) {
        type_ind = target;
      } else {
        break;
      }
    }
  }
  type_ref type_ind = root_ind;
  while (type_ind != target) {
    type a = VEC_GET(types->types, type_ind);
    type_ind = VEC_GET(types->substitutions, a.type_var);
    type_ref *sub = VEC_GET_PTR(types->substitutions, a.type_var);
    *sub = target;
  }
  return target;
}

#ifdef DEBUG_TC
static vec_tc_error solve_constraints(tc_constraints_res p_constraints,
                                      type_builder *type_builder, parse_tree tree) {
  unification_state state = {
    .tree = tree,
    .errors = VEC_NEW,
    .types = type_builder,
    .constraints = p_constraints,
  };
#else
static vec_tc_error solve_constraints(tc_constraints_res p_constraints,
                                      type_builder *type_builder) {
  unification_state state = {
    .errors = VEC_NEW,
    .types = type_builder,
    .constraints = p_constraints,
  };
#endif
  while (state.constraints.len > 0) {
    tc_constraint constraint;
    VEC_POP(&state.constraints, &constraint);
    constraint.a = resolve_type(type_builder, constraint.a);
    constraint.b = resolve_type(type_builder, constraint.b);
    type a = VEC_GET(type_builder->types, constraint.a);
    type b = VEC_GET(type_builder->types, constraint.b);
#ifdef DEBUG_TC
    printf("solve: %d = %d\n", constraint.a, constraint.b);
#endif

    // switcheroo
    if (b.check_tag == TC_VAR && a.check_tag != TC_VAR) {
      tc_constraint c = {
        .a = constraint.b,
        .b = constraint.a,
        .provenance = constraint.provenance,
      };
      constraint = c;
      type tmp = a;
      a = b;
      b = tmp;
#ifdef DEBUG_TC
      printf("solve: %d = %d\n", constraint.a, constraint.b);
#endif
    }

    if (a.check_tag == TC_VAR) {
      unify_typevar(&state, a.type_var, constraint.b, b);
      continue;
    }

    if (a.check_tag != b.check_tag) {
      add_conflict(&state.errors, constraint);
      continue;
    }

    switch (type_repr(a.check_tag)) {
      case SUBS_NONE:
        break;
      case SUBS_TWO: {
        tc_constraint c = {
          .a = a.sub_b,
          .b = b.sub_b,
          .provenance = constraint.provenance,
        };
        VEC_PUSH(&state.constraints, c);
        HEDLEY_FALL_THROUGH;
      }
      case SUBS_ONE: {
        tc_constraint c = {
          .a = a.sub_a,
          .b = b.sub_a,
          .provenance = constraint.provenance,
        };
        VEC_PUSH(&state.constraints, c);
        break;
      }
      case SUBS_EXTERNAL:
        if (a.sub_amt != b.sub_amt) {
          add_conflict(&state.errors, constraint);
          break;
        }
        for (type_ref i = 0; i < a.sub_amt; i++) {
          tc_constraint c = {
            .a = VEC_GET(state.types->inds, a.subs_start + i),
            .b = VEC_GET(state.types->inds, b.subs_start + i),
            .provenance = constraint.provenance,
          };
          VEC_PUSH(&state.constraints, c);
        }
        break;
    }
  }
  return state.errors;
}

void print_tyvar_parse_node(parse_tree tree, type *types, type_ref ref) {
  type t = types[ref];
  if (t.check_tag != TC_VAR || t.type_var >= tree.node_amt) {
    return;
  }
  fputs("Parse node: ", stdout);
  puts(parse_node_strings[tree.nodes[t.type_var].type.all]);
  putc('\n', stdout);
}

static void check_ambiguities(node_ind_t parse_node_amt, type_builder *builder,
                              vec_tc_error *errors) {
  for (node_ind_t node_ind = 0; node_ind < parse_node_amt; node_ind++) {
    if (type_contains_unsubstituted_typevar(
          builder, VEC_GET(builder->substitutions, node_ind))) {
      tc_error err = {
        .type = TC_ERR_AMBIGUOUS,
        .pos = node_ind,
      };
      VEC_PUSH(errors, err);
    }
  }
}

static type_ref copy_type(const type_builder *old, type_builder *builder,
                          type_ref root_type) {
  bitset first_pass_stack = bs_new();
  bs_push_true(&first_pass_stack);
  vec_type_ref stack = VEC_NEW;
  VEC_PUSH(&stack, root_type);
  vec_type_ref return_stack = VEC_NEW;
  while (stack.len > 0) {
    type_ref type_ind;
    VEC_POP(&stack, &type_ind);
    bool first_pass = bs_pop(&first_pass_stack);
    type t = VEC_GET(old->types, type_ind);
    if (t.check_tag == TC_VAR) {
      VEC_PUSH(&stack, VEC_GET(old->substitutions, t.type_var));
      bs_push(&first_pass_stack, first_pass);
      continue;
    }
    switch (type_repr(t.check_tag)) {
      case SUBS_NONE: {
        // first pass
        VEC_PUSH(&return_stack, mk_primitive_type(builder, t.check_tag));
        break;
      }
      case SUBS_ONE: {
        if (first_pass) {
          VEC_PUSH(&stack, type_ind);
          bs_push_false(&first_pass_stack);
          VEC_PUSH(&stack, t.sub_a);
          bs_push_true(&first_pass_stack);
        } else {
          type_ref sub_a;
          VEC_POP(&return_stack, &sub_a);
          VEC_PUSH(&return_stack, mk_type_inline(builder, t.check_tag, sub_a, 0));
        }
        break;
      }
      case SUBS_TWO: {
        if (first_pass) {
          VEC_PUSH(&stack, type_ind);
          bs_push_false(&first_pass_stack);
          VEC_PUSH(&stack, t.sub_b);
          bs_push_true(&first_pass_stack);
          VEC_PUSH(&stack, t.sub_a);
          bs_push_true(&first_pass_stack);
        } else {
          type_ref sub_a;
          VEC_POP(&return_stack, &sub_a);
          type_ref sub_b;
          VEC_POP(&return_stack, &sub_b);
          VEC_PUSH(&return_stack,
                   mk_type_inline(builder, t.check_tag, sub_a, sub_b));
        }
        break;
      }
      case SUBS_EXTERNAL: {
        if (first_pass) {
          VEC_PUSH(&stack, type_ind);
          bs_push_false(&first_pass_stack);
          // first on stack = last processed = first on result stack
          for (node_ind_t i = 0; i < t.sub_amt; i++) {
            VEC_PUSH(&stack, VEC_GET(old->inds, t.subs_start + i));
            bs_push_true(&first_pass_stack);
          }
        } else {
          type_ref *subs_ptr =
            &VEC_DATA_PTR(&return_stack)[return_stack.len - t.sub_amt];
          VEC_PUSH(&return_stack,
                   mk_type(builder, t.check_tag, subs_ptr, t.sub_amt));
        }
        break;
      }
    }
  }
  type_ref res;
  VEC_POP(&return_stack, &res);
  VEC_FREE(&return_stack);
  VEC_FREE(&stack);
  bs_free(&first_pass_stack);
  return res;
}

// This is basically a bag-of-types specific tracing copying garbage collector,
// which gets
static type_info cleanup_types(node_ind_t parse_node_amt, type_builder *old) {
  type_builder builder = {
    .types = VEC_NEW,
    .inds = VEC_NEW,
    .substitutions = VEC_NEW,
  };

  VEC_APPEND(&builder.types, builtin_type_amount, builtin_types);
  VEC_APPEND(&builder.inds, builtin_type_ind_amount, builtin_type_inds);

  type_ref *node_types = malloc(sizeof(type_ref) * parse_node_amt);

  for (node_ind_t i = 0; i < parse_node_amt; i++) {
    type_ref type_ind = VEC_GET(old->substitutions, i);
    node_types[i] = copy_type(old, &builder, type_ind);
  }

  type_ref type_amt = builder.types.len;
  type_info res = {
    .node_types = node_types,
    .type_amt = type_amt,
    .types = VEC_FINALIZE(&builder.types),
    .type_inds = VEC_FINALIZE(&builder.inds),
  };

  return res;
}

tc_res typecheck(const parse_tree tree) {
#ifdef TIME_TYPECHECK
  struct timespec start = get_monotonic_time();
#endif
  type_builder type_builder = {
    .types = VEC_NEW,
    .inds = VEC_NEW,
    .substitutions = VEC_NEW,
  };

  // blit builtin types
  VEC_APPEND(&type_builder.types, builtin_type_amount, builtin_types);
  VEC_APPEND(&type_builder.inds, builtin_type_ind_amount, builtin_type_inds);

  // every parse_node index has a corresponding entry in the substitutions
  annotate_parse_tree(tree, &type_builder);
  tc_constraints_res constraints_res =
    generate_constraints(tree, &type_builder);
#ifdef DEBUG_TC
  printf("Generated %d constraints: ", constraints_res.len);
  for (node_ind_t i = 0; i < constraints_res.len; i++) {
    tc_constraint c = VEC_GET(constraints_res, i);
    type *types = VEC_DATA_PTR(&type_builder.types);
    type_ref *inds = VEC_DATA_PTR(&type_builder.inds);
    printf("Constraint: %d=%d\n", c.a, c.b);
    print_type(stdout, types, inds, c.a);
    putc('\n', stdout);
    print_tyvar_parse_node(tree, VEC_DATA_PTR(&type_builder.types), c.a);
    print_type(stdout, types, inds, c.b);
    putc('\n', stdout);
    print_tyvar_parse_node(tree, VEC_DATA_PTR(&type_builder.types), c.b);
    puts("\n---\n");
  }

  vec_tc_error errors = solve_constraints(constraints_res, &type_builder, tree);
#else
  vec_tc_error errors = solve_constraints(constraints_res, &type_builder);
#endif

  VEC_FREE(&constraints_res);
  if (errors.len == 0) {
    check_ambiguities(tree.node_amt, &type_builder, &errors);
  }

  if (errors.len == 0) {
    type_info clean_types = cleanup_types(tree.node_amt, &type_builder);
    free_type_builder(type_builder);

    VEC_FREE(&errors);
    tc_res res = {
      .error_amt = 0,
      .errors = NULL,
      .types = clean_types,
#ifdef TIME_TYPECHECK
      .time_taken = time_since_monotonic(start),
#endif
    };
    return res;
  }

  VEC_LEN_T error_amt = errors.len;

  tc_res res = {
    .error_amt = error_amt,
    .errors = VEC_FINALIZE(&errors),
    .types =
      {
        .type_amt = type_builder.types.len,
        .type_inds = VEC_FINALIZE(&type_builder.inds),
        .types = VEC_FINALIZE(&type_builder.types),
        .node_types = VEC_FINALIZE(&type_builder.substitutions),
      },
#ifdef TIME_TYPECHECK
    .time_taken = time_since_monotonic(start),
#endif
  };
  return res;
}

void free_tc_res(tc_res res) {
  free(res.types.types);
  free(res.types.type_inds);
  free(res.types.node_types);
  if (res.error_amt > 0) {
    free(res.errors);
  }
}
