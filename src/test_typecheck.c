#include <stdbool.h>
#include <stdio.h>

#include "diagnostic.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "util.h"

/*

# Ideas:

* Use markers insted of indices for spans

*/

typedef struct test_type {
  type_tag tag;
  const struct test_type *subs;
  const size_t sub_amt;
} test_type;

VEC_DECL(test_type);

typedef struct {
  tc_error_type type;
  buf_ind_t start;
  buf_ind_t end;
  union {
    struct {
      test_type type_exp;
      test_type type_got;
    };
  };
} tc_err_test;

typedef struct {
  test_type exp;
  buf_ind_t start;
  buf_ind_t end;
} exp_type_span;

typedef struct {
  enum {
    TYPE_MATCHES,
    PRODUCES_ERRORS,
  } type;
  union {
    struct {
      size_t span_amt;
      exp_type_span *spans;
    };
    struct {
      size_t error_amt;
      const tc_err_test *errors;
    };
  };
} tc_test;

static bool test_type_eq(type *types, node_ind_t *inds, node_ind_t root,
                         test_type t) {
  vec_node_ind stackA = VEC_NEW;
  VEC_PUSH(&stackA, root);
  vec_test_type stackB = VEC_NEW;
  VEC_PUSH(&stackB, t);
  bool res = true;
  while (true) {
    if (stackA.len != stackB.len) {
      res = false;
      break;
    }
    if (stackA.len == 0)
      break;
    type tA = types[VEC_POP(&stackA)];
    test_type tB = VEC_POP(&stackB);
    if (tA.tag != tB.tag) {
      res = false;
      break;
    }
    switch (type_repr(tA.tag)) {
      case SUBS_EXTERNAL:
        VEC_APPEND(&stackA, tA.sub_amt, inds + tA.subs_start);
        break;
      case SUBS_ONE:
        VEC_PUSH(&stackA, tA.sub_a);
        break;
      case SUBS_TWO:
        VEC_PUSH(&stackA, tA.sub_a);
        VEC_PUSH(&stackA, tA.sub_b);
        break;
      case SUBS_NONE:
        break;
    }
    VEC_APPEND(&stackB, tB.sub_amt, tB.subs);
  }
  VEC_FREE(&stackA);
  VEC_FREE(&stackB);
  return res;
}

static bool test_err_eq(parse_tree tree, tc_res res, size_t err_ind,
                        tc_err_test test_err) {
  tc_error eA = res.errors[err_ind];
  if (eA.type != test_err.type)
    return false;
  parse_node node = tree.nodes[eA.pos];
  if (node.span.start != test_err.start)
    return false;
  if (node.span.end != test_err.end)
    return false;
  switch (eA.type) {
    case TYPE_MISMATCH: {
      return test_type_eq(res.types.types, res.types.type_inds, eA.expected,
                          test_err.type_exp) &&
             test_type_eq(res.types.types, res.types.type_inds, eA.got,
                          test_err.type_got);
    }
    default:
      break;
  }
  return true;
}

static bool all_errors_match(parse_tree tree, tc_res res, size_t exp_error_amt,
                             const tc_err_test *exp_errors) {
  if (res.error_amt != exp_error_amt)
    return false;
  for (size_t i = 0; i < exp_error_amt; i++) {
    if (!test_err_eq(tree, res, i, exp_errors[i]))
      return false;
  }
  return true;
}

static void test_types_match(test_state *state, parse_tree tree, tc_res res,
                             tc_test test) {
  for (size_t i = 0; i < test.span_amt; i++) {
    exp_type_span span = test.spans[i];
    bool seen = false;
    for (size_t j = 0; j < tree.node_amt; j++) {
      parse_node node = tree.nodes[j];
      if (node.type == PT_ROOT)
        continue;
      if (node.span.start == span.start && node.span.end == span.end) {
        seen = true;
        if (!test_type_eq(res.types.types, res.types.type_inds,
                          res.types.node_types[j], span.exp)) {
          stringstream *ss = ss_init();
          print_type(ss->stream, res.types.types, res.types.node_types[j]);
          char *str = ss_finalize(ss);
          failf(state, "Type mismatch in test. Got: %s", str);
          free(str);
        }
      }
    }
    if (!seen) {
      failf(state, "No node matching position %d-%d", span.start, span.end);
    }
  }
}

static void run_typecheck_test(test_state *state, const char *input,
                               tc_test test) {
  parse_tree_res pres = test_upto_parse_tree(state, input);
  if (!pres.succeeded) {
    free_parse_tree_res(pres);
    return;
  }

  tc_res res = typecheck(input, pres.tree);
  switch (test.type) {
    case TYPE_MATCHES: {
      if (res.error_amt > 0) {
        stringstream *ss = ss_init();
        fprintf(ss->stream, "Didn't expect typecheck errors. Got %d:\n",
                res.error_amt);
        print_tc_errors(ss->stream, input, pres.tree, res);
        failf(state, ss_finalize(ss), input);
      } else {
        test_types_match(state, pres.tree, res, test);
      }
      break;
    }
    case PRODUCES_ERRORS:
      if (!all_errors_match(pres.tree, res, test.error_amt, test.errors)) {
        stringstream *ss = ss_init();
        fprintf(ss->stream, "Expected %zu errors, got %d.\n", test.error_amt,
                res.error_amt);
        if (res.error_amt > 0) {
          fputs("Errors:\n", ss->stream);
        }
        print_tc_errors(ss->stream, input, pres.tree, res);
        char *str = ss_finalize(ss);
        failf(state, str, input);
        free(str);
      }
      break;
  }

  free_tc_res(res);
  free_parse_tree_res(pres);
}

static const test_type i16 = {
  .tag = T_I16,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type unit = {
  .tag = T_UNIT,
  .sub_amt = 0,
  .subs = NULL,
};

static void run_typecheck_error_test(test_state *state, const char *input,
                                     size_t error_amt,
                                     const tc_err_test *errors) {
  tc_test test = {
    .type = PRODUCES_ERRORS,
    .error_amt = error_amt,
    .errors = errors,
  };
  run_typecheck_test(state, input, test);
}

static void test_typecheck_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  {
    const char *input = "(sig a (Fn () I16))\n"
                        "(fun a () 2)";
    test_start(state, "Return type");
    {
      exp_type_span spans[] = {{
        .start = 14,
        .end = 16,
        .exp = i16,
      }};
      tc_test test = {
        .type = TYPE_MATCHES,
        .span_amt = STATIC_LEN(spans),
        .spans = spans,
      };
      run_typecheck_test(state, input, test);
    }
    test_end(state);

    test_start(state, "Return value");
    {
      exp_type_span spans[] = {{
        .start = 30,
        .end = 30,
        .exp = i16,
      }};
      tc_test test = {
        .type = TYPE_MATCHES,
        .span_amt = STATIC_LEN(spans),
        .spans = spans,
      };
      run_typecheck_test(state, input, test);
    }
    test_end(state);

    test_start(state, "Fn bnd");
    const test_type fn_subs[] = {unit, i16};
    const test_type fn_type = {
      .tag = T_FN,
      .sub_amt = STATIC_LEN(fn_subs),
      .subs = fn_subs,
    };

    {
      exp_type_span spans[] = {{
        .start = 25,
        .end = 25,
        .exp = fn_type,
      }};
      tc_test test = {
        .type = TYPE_MATCHES,
        .span_amt = STATIC_LEN(spans),
        .spans = spans,
      };
      run_typecheck_test(state, input, test);
    }
    test_end(state);
  }

  test_start(state, "[U8] Literals");
  {
    exp_type_span spans[] = {{
      .start = 31,
      .end = 32,
      .exp = {.tag = T_U8},
    }};
    tc_test test = {
      .type = TYPE_MATCHES,
      .span_amt = STATIC_LEN(spans),
      .spans = spans,
    };
    const char *input = "(sig a (Fn U8 [U8]))\n"
                        "(fun a 1 [12])";
    run_typecheck_test(state, input, test);
  }
  test_end(state);

  test_start(state, "Multi-expression block");
  {
    exp_type_span spans[] = {
      {
        .start = 42,
        .end = 43,
        .exp = {.tag = T_U8},
      },
      {
        .start = 38,
        .end = 38,
        .exp = {.tag = T_I32},
      },
    };
    tc_test test = {
      .type = TYPE_MATCHES,
      .span_amt = STATIC_LEN(spans),
      .spans = spans,
    };
    const char *input = "(sig a (Fn U8 [U8]))\n"
                        "(fun a 1 (as I32 2) [12])";
    run_typecheck_test(state, input, test);
  }
  test_end(state);

  test_start(state, "Param shadows binding");
  {
    run_typecheck_error_test(state,
                             "(sig a (Fn () [U8]))\n"
                             "(fun a () "
                             "(sig b (Fn () ()))"
                             "(fun b a a))",
                             0, NULL);
  }

  test_end(state);

  test_group_end(state);
}

static void test_typecheck_errors(test_state *state) {
  test_group_start(state, "Produces errors");

  test_start(state, "I32 vs (Int, Int)");
  {
    static const tc_err_test errors[] = {
      {
        .type = TYPE_HEAD_MISMATCH,
        .start = 30,
        .end = 35,
      },
    };
    static const char *prog = "(sig a (Fn () I32))\n"
                              "(fun a () (2, 3))";
    run_typecheck_error_test(state, prog, STATIC_LEN(errors), errors);
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
    const tc_err_test errors[] = {
      {
        .type = TYPE_HEAD_MISMATCH,
        .start = 30,
        .end = 38,
        .type_exp = i32,
        .type_got = got,
      },
    };
    run_typecheck_error_test(state,
                             "(sig a (Fn () I32))\n"
                             "(fun a () (fn () 1))",
                             STATIC_LEN(errors), errors);
  }
  test_end(state);

  test_start(state, "I32 vs [I32]");
  {
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 30,
        .end = 32,
      },
    };
    run_typecheck_error_test(state,
                             "(sig a (Fn () I32))\n"
                             "(fun a () [3])",
                             STATIC_LEN(errors), errors);
  }
  test_end(state);

  test_start(state, "I32 vs String");
  {
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 30,
        .end = 33,
      },
    };
    run_typecheck_error_test(state,
                             "(sig a (Fn () I32))\n"
                             "(fun a () \"hi\")",
                             STATIC_LEN(errors), errors);
  }
  test_end(state);

  test_start(state, "String vs I32");
  {
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 33,
        .end = 35,
      },
    };
    run_typecheck_error_test(state,
                             "(sig a (Fn () String))\n"
                             "(fun a () 321)",
                             STATIC_LEN(errors), errors);
  }
  test_end(state);

  test_group_end(state);
}

static void test_typecheck_stress(test_state *state) {
  test_start(state, "Stress");
  {
    stringstream *ss = ss_init();
    const unsigned n = 10;
    fputs("(sig a (Fn (", ss->stream);
    for (unsigned i = 1; i < n; i++) {
      fputs("U8,", ss->stream);
    }
    fputs("U8", ss->stream);
    fputs(") (", ss->stream);
    for (unsigned i = 1; i < n; i++) {
      fputs("U8,", ss->stream);
    }
    fputs("U8", ss->stream);
    fputs(")))\n", ss->stream);
    fputs("(fun a b b)", ss->stream);
    run_typecheck_error_test(state, ss_finalize(ss), 0, NULL);
  }
  test_end(state);
}

void test_typecheck(test_state *state) {
  test_group_start(state, "Typecheck");

  test_typecheck_succeeds(state);
  test_typecheck_errors(state);
  if (!state->config.lite) {
    test_typecheck_stress(state);
  }

  test_group_end(state);
}
