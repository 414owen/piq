#include <stdbool.h>
#include <stdio.h>

#include "diagnostic.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "util.h"

static void print_tc_error(FILE *f, tc_res res, NODE_IND_T err_ind) {
  tc_error error = VEC_GET(res.errors, err_ind);
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
      print_type(f, VEC_DATA_PTR(&res.types), VEC_DATA_PTR(&res.type_inds),
                 error.got);
      fputs("'", f);
      break;
    case LITERAL_MISMATCH:
      fputs("Literal mismatch. Expected '", f);
      print_type(f, VEC_DATA_PTR(&res.types), VEC_DATA_PTR(&res.type_inds),
                 error.expected);
      fputs("'", f);
      break;
    case WRONG_ARITY:
      fputs("Wrong arity", f);
      break;
  }
  // fputs(":\n", f);
  parse_node node = res.tree.nodes[error.pos];
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

typedef struct {
  test_type exp;
  BUF_IND_T start;
  BUF_IND_T end;
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

static bool test_type_eq(vec_type types, vec_node_ind inds, NODE_IND_T root,
                         test_type t) {
  vec_node_ind stackA = VEC_NEW;
  VEC_PUSH(&stackA, root);
  vec_test_type stackB = VEC_NEW;
  VEC_PUSH(&stackB, t);
  bool res = true;
  while (stackA.len > 0) {
    type tA = VEC_GET(types, VEC_POP(&stackA));
    test_type tB = VEC_POP(&stackB);
    if (tA.tag != tB.tag || tA.sub_amt != tB.sub_amt) {
      res = false;
      break;
    }
    for (node_ind i = 0; i < tA.sub_amt; i++) {
      VEC_PUSH(&stackA, VEC_GET(inds, tA.sub_start + i));
      VEC_PUSH(&stackB, tB.subs[i]);
    }
  }
  VEC_FREE(&stackA);
  VEC_FREE(&stackB);
  return res;
}

static bool test_err_eq(tc_res res, size_t err_ind, tc_err_test test_err) {
  tc_error eA = VEC_GET(res.errors, err_ind);
  if (eA.type != test_err.type)
    return false;
  parse_node node = res.tree.nodes[eA.pos];
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

static bool all_errors_match(tc_res res, size_t exp_error_amt,
                             const tc_err_test *exp_errors) {
  if (res.errors.len != exp_error_amt)
    return false;
  for (size_t i = 0; i < exp_error_amt; i++) {
    if (!test_err_eq(res, i, exp_errors[i]))
      return false;
  }
  return true;
}

static void test_types_match(test_state *state, tc_res res, tc_test test) {
  for (size_t i = 0; i < test.span_amt; i++) {
    exp_type_span span = test.spans[i];
    bool seen = false;
    for (size_t j = 0; j < res.tree.node_amt; j++) {
      parse_node node = res.tree.nodes[j];
      if (node.type == PT_ROOT)
        continue;
      if (node.start == span.start && node.end == span.end) {
        seen = true;
        if (!test_type_eq(res.types, res.type_inds, VEC_GET(res.node_types, j),
                          span.exp)) {
          stringstream *ss = ss_init();
          print_type(ss->stream, res.types.data, res.type_inds.data,
                     VEC_GET(res.node_types, j));
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
  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input,
          tres.error_pos);
    goto end_a;
  }

  parse_tree_res pres = parse(tres.tokens, tres.token_amt);
  if (!pres.succeeded) {
    failf(state, "Parsing \"%s\" failed.", input);
    free(pres.expected);
    goto end_b;
  }

  tc_res res = typecheck(test_file, pres.tree);
  switch (test.type) {
    case TYPE_MATCHES: {
      if (res.errors.len > 0) {
        failf(state, "Didn't expect typecheck errors");
      } else {
        test_types_match(state, res, test);
      }
      break;
    }
    case PRODUCES_ERRORS:
      if (!all_errors_match(res, test.error_amt, test.errors)) {
        stringstream *ss = ss_init();
        fprintf(ss->stream, "Expected %zu errors, got %d.\n", test.error_amt,
                res.errors.len);
        if (res.errors.len > 0) {
          fputs("Errors:\n", ss->stream);
        }
        for (size_t i = 0; i < res.errors.len; i++) {
          putc('\n', ss->stream);
          print_tc_error(ss->stream, res, i);
        }
        char *str = ss_finalize(ss);
        failf(state, str, input);
        free(str);
      }
  }

  VEC_FREE(&res.errors);
  VEC_FREE(&res.types);
  VEC_FREE(&res.type_inds);
  VEC_FREE(&res.node_types);

end_b:
  free(pres.tree.inds);
  free(pres.tree.nodes);

end_a:
  free(tres.tokens);
}

static const test_type i16 = {
  .tag = T_I16,
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
    const char *input = "(fn a () I16 2)";
    test_start(state, "Return type");
    {
#define type_amt 1
      exp_type_span spans[] = {{
        .start = 9,
        .end = 11,
        .exp = i16,
      }};
      tc_test test = {
        .type = TYPE_MATCHES,
        .span_amt = type_amt,
        .spans = spans,
      };
#undef type_amt
      run_typecheck_test(state, input, test);
    }
    test_end(state);

    test_start(state, "Return value");
    {
#define type_amt 1
      exp_type_span spans[] = {{
        .start = 13,
        .end = 13,
        .exp = i16,
      }};
      tc_test test = {
        .type = TYPE_MATCHES,
        .span_amt = type_amt,
        .spans = spans,
      };
#undef type_amt
      run_typecheck_test(state, input, test);
    }
    test_end(state);

    test_start(state, "Fn bnd");
    static const test_type fn_type = {
      .tag = T_FN,
      .sub_amt = 1,
      .subs = &i16,
    };

    {
#define type_amt 1
      exp_type_span spans[] = {{
        .start = 4,
        .end = 4,
        .exp = fn_type,
      }};
      tc_test test = {
        .type = TYPE_MATCHES,
        .span_amt = type_amt,
        .spans = spans,
      };
#undef type_amt
      run_typecheck_test(state, input, test);
    }
    test_end(state);
  }

  test_group_end(state);
}

void test_typecheck_errors(test_state *state) {
  test_group_start(state, "Produces errors");

  test_start(state, "I32 vs (Int, Int)");
  {
    static const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 13,
        .end = 18,
      },
    };
    run_typecheck_error_test(state, "(fun a () I32 (1, 2))", STATIC_LEN(errors),
                             errors);
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
        .type = TYPE_MISMATCH,
        .start = 13,
        .end = 25,
        .type_exp = i32,
        .type_got = got,
      },
    };
    run_typecheck_error_test(state, "(fn a () I32 (fn () I32 1))",
                             STATIC_LEN(errors), errors);
  }
  test_end(state);

  test_start(state, "I32 vs [I32]");
  {
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 13,
        .end = 15,
      },
    };
    run_typecheck_error_test(state, "(fn a () I32 [3])", STATIC_LEN(errors),
                             errors);
  }
  test_end(state);

  test_start(state, "I32 vs String");
  {
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 13,
        .end = 16,
      },
    };
    run_typecheck_error_test(state, "(fn a () I32 \"hi\")", STATIC_LEN(errors),
                             errors);
  }
  test_end(state);

  test_start(state, "String vs I32");
  {
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
        .start = 16,
        .end = 18,
      },
    };
    run_typecheck_error_test(state, "(fn a () String 321)", STATIC_LEN(errors),
                             errors);
  }
  test_end(state);

  test_start(state, "String vs String");
  { run_typecheck_error_test(state, "(fn a () String \"hi\")", 0, NULL); }
  test_end(state);

  // TODO enable when I add list type parsing
  // test_start(state, "[U8] vsString");
  // {
  //   run_typecheck_error_test(state, "(fn a () [U8] \"hi\")", 0, NULL);
  // }
  // test_end(state);

  test_group_end(state);
}

void test_typecheck(test_state *state) {
  test_group_start(state, "Typecheck");

  test_typecheck_errors(state);
  test_typecheck_succeeds(state);

  test_group_end(state);
}
