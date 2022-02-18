#include <stdbool.h>
#include <stdio.h>

#include "diagnostic.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "util.h"

static void print_tc_error(FILE *f, tc_res res, NODE_IND_T err_ind) {
  tc_error error = res.errors.data[err_ind];
  switch (error.type) {
    case INT_LARGER_THAN_MAX:
      fputs("Int doesn't fit into type", f);
      break;
    case TYPE_NOT_FOUND:
      fputs("Type not in scope", f);
      break;
    case AMBIGUOUS_TYPE:
      fputs("Ambiguous type", f);
      break;
    case BINDING_NOT_FOUND:
      fputs("Binding not found", f);
      break;
    case TYPE_MISMATCH:
      fputs("Type mismatch. Expected '", f);
      print_type(f, res.types.data, res.type_inds.data, error.expected);
      fputs("', got '", f);
      print_type(f, res.types.data, res.type_inds.data, error.got);
      fputs("'", f);
      break;
    case LITERAL_MISMATCH:
      fputs("Literal mismatch. Expected '", f);
      print_type(f, res.types.data, res.type_inds.data, error.expected);
      fputs("'", f);
      break;
    case WRONG_ARITY:
      fputs("Wrong arity", f);
      break;
  }
  // fputs(":\n", f);
  parse_node node = res.tree.nodes.data[error.pos];
  fprintf(f, " at %d-%d:\n", node.start, node.end);
  format_error_ctx(f, res.source.data, node.start, node.end);
}

typedef struct test_type {
  type_tag tag;
  const struct test_type *subs;
  const size_t sub_amt;
} test_type;

VEC_DECL(test_type);

typedef struct {
  tc_error_type type;
  BUF_IND_T start;
  BUF_IND_T end;
  union {
    struct {
      test_type type_exp;
      test_type type_got;
    };
  };
} tc_err_test;

static bool test_type_eq(vec_type types, vec_node_ind inds, NODE_IND_T root,
                         test_type t) {
  vec_node_ind stackA = VEC_NEW;
  VEC_PUSH(&stackA, root);
  vec_test_type stackB = VEC_NEW;
  VEC_PUSH(&stackB, t);
  bool res = true;
  while (stackA.len > 0) {
    type tA = types.data[VEC_POP(&stackA)];
    test_type tB = VEC_POP(&stackB);
    if (tA.tag != tB.tag || tA.sub_amt != tB.sub_amt) {
      res = false;
      break;
    }
    for (node_ind i = 0; i < tA.sub_amt; i++) {
      VEC_PUSH(&stackA, inds.data[tA.sub_start + i]);
      VEC_PUSH(&stackB, tB.subs[i]);
    }
  }
  VEC_FREE(&stackA);
  VEC_FREE(&stackB);
  return res;
}

static bool test_err_eq(tc_res res, size_t err_ind, tc_err_test test_err) {
  tc_error eA = res.errors.data[err_ind];
  if (eA.type != test_err.type)
    return false;
  parse_node node = res.tree.nodes.data[eA.pos];
  if (node.start != test_err.start)
    return false;
  if (node.end != test_err.end)
    return false;
  switch (eA.type) {
    case TYPE_MISMATCH: {
      return test_type_eq(res.types, res.type_inds, eA.expected,
                          test_err.type_exp) &&
             test_type_eq(res.types, res.type_inds, eA.got, test_err.type_got);
    }
    default:
      break;
  }
  return true;
}

static bool all_errors_match(tc_res res, size_t exp_err_amt,
                             const tc_err_test *exp_errs) {
  if (res.errors.len != exp_err_amt)
    return false;
  for (size_t i = 0; i < exp_err_amt; i++) {
    if (!test_err_eq(res, i, exp_errs[i]))
      return false;
  }
  return true;
}

static void test_typecheck_produces(test_state *state, char *input,
                                    size_t error_amt,
                                    const tc_err_test *errors) {
  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input,
          tres.error_pos);
    goto end_a;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (!pres.succeeded) {
    failf(state, "Parsing \"%s\" failed.", input);
    goto end_b;
  }

  tc_res res = typecheck(test_file, pres.tree);
  if (!all_errors_match(res, error_amt, errors)) {
    stringstream *ss = ss_init();
    fprintf(ss->stream, "Expected %zu errors, got %d.\n", error_amt,
            res.errors.len);
    if (res.errors.len > 0) {
      fputs("Errors:\n", ss->stream);
      for (size_t i = 0; i < res.errors.len; i++) {
        putc('\n', ss->stream);
        print_tc_error(ss->stream, res, i);
      }
    }
    char *str = ss_finalize(ss);
    failf(state, str, input);
    free(str);
  }

  VEC_FREE(&res.errors);
  VEC_FREE(&res.types);
  VEC_FREE(&res.type_inds);
  VEC_FREE(&res.node_types);

end_b:
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
}

void test_typecheck(test_state *state) {
  test_group_start(state, "Typecheck");

  test_start(state, "I32 vs (Int, Int)");
  {
    static const tc_err_test errs[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 13,
        .end = 18,
      },
    };
    test_typecheck_produces(state, "(fn a () I32 (1, 2))", STATIC_LEN(errs),
                            errs);
  }
  test_end(state);

  test_start(state, "I32 vs (() -> Int)");
  {
    const test_type i32 = {
      .tag = T_I32,
      .sub_amt = 0,
      .subs = NULL,
    };
    const test_type got = {
      .tag = T_FN,
      .sub_amt = 1,
      .subs = &i32,
    };
    const tc_err_test errs[] = {
      {
        .type = TYPE_MISMATCH,
        .start = 13,
        .end = 25,
        .type_exp = i32,
        .type_got = got,
      },
    };
    test_typecheck_produces(state, "(fn a () I32 (fn () I32 1))",
                            STATIC_LEN(errs), errs);
  }
  test_end(state);

  test_start(state, "I32 vs [I32]");
  {
    const tc_err_test errs[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 13,
        .end = 15,
      },
    };
    test_typecheck_produces(state, "(fn a () I32 [3])", STATIC_LEN(errs), errs);
  }
  test_end(state);

  test_start(state, "I32 vs String");
  {
    const tc_err_test errs[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 13,
        .end = 16,
      },
    };
    test_typecheck_produces(state, "(fn a () I32 \"hi\")", STATIC_LEN(errs),
                            errs);
  }
  test_end(state);

  test_start(state, "String vs I32");
  {
    const tc_err_test errs[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 16,
        .end = 18,
      },
    };
    test_typecheck_produces(state, "(fn a () String 321)", STATIC_LEN(errs),
                            errs);
  }
  test_end(state);

  test_start(state, "String vs String");
  { test_typecheck_produces(state, "(fn a () String \"hi\")", 0, NULL); }
  test_end(state);

  // TODO enable when I add list type parsing
  // test_start(state, "[U8] vsString");
  // {
  //   test_typecheck_produces(state, "(fn a () [U8] \"hi\")", 0, NULL);
  // }
  // test_end(state);

  test_group_end(state);
}
