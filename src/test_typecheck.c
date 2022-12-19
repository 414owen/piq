#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "diagnostic.h"
#include "parse_tree.h"
#include "test.h"
#include "tests.h"
#include "test_upto.h"
#include "typecheck.h"
#include "util.h"

static char *marker_start = "→";
static char *marker_end = "←";

typedef struct test_type {
  type_tag tag;
  const struct test_type *subs;
  size_t sub_amt;
} test_type;

VEC_DECL(test_type);

typedef struct {
  tc_error_type type;
  union {
    struct {
      test_type type_exp;
      test_type type_got;
    };
  };
} tc_err_test;

typedef struct {
  enum {
    TYPE_MATCHES,
    PRODUCES_ERRORS,
  } type;
  union {
    struct {
      size_t span_amt;
      test_type *spans;
    };
    struct {
      size_t error_amt;
      const tc_err_test *errors;
    };
  };
} tc_test;

static bool test_type_eq(type *types, node_ind_t *inds, node_ind_t root,
                         test_type t) {
  vec_node_ind stack_a = VEC_NEW;
  VEC_PUSH(&stack_a, root);
  vec_test_type stack_b = VEC_NEW;
  VEC_PUSH(&stack_b, t);
  bool res = true;
  while (true) {
    if (stack_a.len != stack_b.len) {
      res = false;
      break;
    }
    if (stack_a.len == 0)
      break;
    type_ref ind_a;
    VEC_POP(&stack_a, &ind_a);
    type type_a = types[ind_a];
    test_type type_b;
    VEC_POP(&stack_b, &type_b);
    if (type_a.tag != type_b.tag) {
      res = false;
      break;
    }
    switch (type_repr(type_a.all_tag)) {
      case SUBS_EXTERNAL:
        VEC_APPEND(&stack_a, type_a.sub_amt, inds + type_a.subs_start);
        break;
      case SUBS_ONE:
        VEC_PUSH(&stack_a, type_a.sub_a);
        break;
      case SUBS_TWO:
        VEC_PUSH(&stack_a, type_a.sub_a);
        VEC_PUSH(&stack_a, type_a.sub_b);
        break;
      case SUBS_NONE:
        break;
    }
    VEC_APPEND(&stack_b, type_b.sub_amt, type_b.subs);
  }
  VEC_FREE(&stack_a);
  VEC_FREE(&stack_b);
  return res;
}

static bool test_err_eq(parse_tree tree, tc_res res, size_t err_ind,
                        tc_err_test test_err, span err_span) {
  tc_error eA = res.errors[err_ind];
  if (eA.type != test_err.type)
    return false;
  parse_node node = tree.nodes[eA.pos];
  if (!spans_equal(node.span, err_span))
    return false;
  switch (eA.type) {
    case TC_ERR_CONFLICT: {
      return test_type_eq(res.types.types,
                          res.types.type_inds,
                          eA.conflict.expected_ind,
                          test_err.type_exp) &&
             test_type_eq(res.types.types,
                          res.types.type_inds,
                          eA.conflict.got_ind,
                          test_err.type_got);
    }
    default:
      break;
  }
  return true;
}

static bool all_errors_match(parse_tree tree, tc_res res,
                             const tc_err_test *exps, const span *spans,
                             node_ind_t error_amt) {
  if (res.error_amt != error_amt)
    return false;
  for (size_t i = 0; i < error_amt; i++) {
    if (!test_err_eq(tree, res, i, exps[i], spans[i]))
      return false;
  }
  return true;
}

static char *process_spans(const char *input, span *spans, node_ind_t cases) {
  size_t len = strlen(input);
  size_t marker_start_len = strlen(marker_start);
  size_t marker_end_len = strlen(marker_end);
  char *edit = malloc(len + 1);

  char *edit_cursor = edit;
  const char *cursor = input;
  buf_ind_t offset = 0;
  for (node_ind_t i = 0; i < cases; i++) {
    const char *start = strstr(cursor, marker_start);
    spans[i].start = (buf_ind_t)(start - input) - offset;
    offset += marker_start_len;
    size_t amt = start - cursor;
    memcpy(edit_cursor, cursor, amt);
    edit_cursor += amt;
    start += marker_start_len;

    const char *end = strstr(start, marker_end);
    spans[i].len = (buf_ind_t)(end - input) - offset - spans[i].start;
    offset += marker_end_len;
    amt = end - start;
    memcpy(edit_cursor, start, amt);
    edit_cursor += amt;
    cursor = end + marker_end_len;
  }
  if (strstr(cursor, marker_start) != NULL) {
    puts("\nMore markers than there are test cases!");
    exit(1);
  }
  size_t remaining = (input + len) - cursor;
  memcpy(edit_cursor, cursor, remaining);
  edit_cursor += remaining;
  edit_cursor[0] = '\0';

  // for (node_ind_t i = 0; i < cases; i++) {
  //   spans[i].end -= 1;
  // }
  return edit;
}

static void test_types_match(test_state *state, const char *input_p,
                             test_type *exps, node_ind_t cases) {
  size_t span_bytes = sizeof(span) * cases;
  span *spans = stalloc(span_bytes);
  char *input = process_spans(input_p, spans, cases);

  upto_resolution_res rres = test_upto_resolution(state, input);
  if (!rres.success) {
    return;
  }

  tc_res res = typecheck(rres.tree);

  add_typecheck_timings(state, rres.tree, res);

  if (res.error_amt > 0) {
    stringstream ss;
    ss_init_immovable(&ss);
    fprintf(
      ss.stream, "Didn't expect typecheck errors. Got %d:\n", res.error_amt);
    print_tc_errors(ss.stream, input, rres.tree, res);
    ss_finalize(&ss);
    failf(state, ss.string, input);
    free(ss.string);
  } else {
    for (size_t i = 0; i < cases; i++) {
      test_type exp = exps[i];
      span span = spans[i];
      bool seen = false;
      for (size_t j = 0; j < rres.tree.node_amt; j++) {
        parse_node node = rres.tree.nodes[j];
        if (spans_equal(node.span, span)) {
          seen = true;
          if (!test_type_eq(res.types.types,
                            res.types.type_inds,
                            res.types.node_types[j],
                            exp)) {
            char *type_str;
            char *context_str;
            {
              stringstream ss;
              ss_init_immovable(&ss);
              print_type(ss.stream,
                         res.types.types,
                         res.types.type_inds,
                         res.types.node_types[j]);
              ss_finalize(&ss);
              type_str = ss.string;
            }
            {
              stringstream ss;
              ss_init_immovable(&ss);
              format_error_ctx(ss.stream, input, span.start, span.len);
              ss_finalize(&ss);
              context_str = ss.string;
            }
            failf(state,
                  "Type mismatch in test. Got: %s\n%s",
                  type_str,
                  context_str);
            free(type_str);
            free(context_str);
          }
        }
      }
      if (!seen) {
        failf(state,
              "No node matching position %d-%d",
              span.start,
              span.start + span.len - 1);
      }
    }
  }

  free_tc_res(res);
  stfree(spans, span_bytes);
  free_parse_tree(rres.tree);
  free(input);
}

static void test_typecheck_errors(test_state *state, const char *input_p,
                                  const tc_err_test *exps, node_ind_t cases) {
  size_t span_bytes = sizeof(span) * cases;
  span *spans = stalloc(span_bytes);
  char *input = process_spans(input_p, spans, cases);

  upto_resolution_res rres = test_upto_resolution(state, input);
  if (!rres.success) {
    stfree(spans, span_bytes);
    free(input);
    failf(state, "Parsing failed");
    return;
  }

  tc_res res = typecheck(rres.tree);

  add_typecheck_timings(state, rres.tree, res);

  if (!all_errors_match(rres.tree, res, exps, spans, cases)) {
    stringstream ss;
    ss_init_immovable(&ss);
    fprintf(ss.stream, "Expected %d errors, got %d.\n", cases, res.error_amt);
    if (res.error_amt > 0) {
      fputs("Errors:\n", ss.stream);
    }
    print_tc_errors(ss.stream, input, rres.tree, res);
    ss_finalize(&ss);
    failf(state, ss.string, input);
    free(ss.string);
  }

  free(input);
  free_tc_res(res);
  free_parse_tree(rres.tree);
  stfree(spans, span_bytes);
}

static const test_type bool_t = {
  .tag = T_BOOL,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type u8_t = {
  .tag = T_U8,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type i8_t = {
  .tag = T_I8,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type i16_t = {
  .tag = T_I16,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type i32_t = {
  .tag = T_I32,
  .sub_amt = 0,
  .subs = NULL,
};

// static const test_type unit = {
//   .tag = T_UNIT,
//   .sub_amt = 0,
//   .subs = NULL,
// };

static void test_typecheck_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  {
    const char *input = "(sig a (Fn →U8←))\n"
                        "(fun a () 2)";
    test_start(state, "Return type U8");
    test_type types[] = {u8_t};
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn I8))\n"
                        "(fun a () →2←)";
    test_start(state, "Return value");
    test_type types[] = {i8_t};
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn I16))\n"
                        "(fun →a← () 2)";
    test_start(state, "Fn bnd");
    const test_type fn_subs[] = {i16_t};
    const test_type fn_type = {
      .tag = T_FN,
      .sub_amt = STATIC_LEN(fn_subs),
      .subs = fn_subs,
    };

    test_type types[] = {
      fn_type,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn U8 [U8]))\n"
                        "(fun a (1) [→12←])";
    test_start(state, "[U8] Literals");
    test_type types[] = {
      u8_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn U8 [U8]))\n"
                        "(fun a (1) (as I32 →2←) [→12←])";
    test_start(state, "Multi-expression block");
    test_type types[] = {
      i32_t,
      u8_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn [U8]))\n"
                        "(fun a () \n"
                        "  (sig b (Fn () ()))\n"
                        "  (fun b (a) ())\n"
                        "  [2])";
    test_start(state, "Param shadows binding");
    test_types_match(state, input, NULL, 0);
    test_end(state);
  }

  {
    test_start(state, "Uses global");
    const char *input = "(sig sndpar (Fn I16 I32 I32))\n"
                        "(fun sndpar (a b) b)\n"
                        "\n"
                        "(sig test (Fn I32))\n"
                        "(fun test () (sndpar →1← →2←))";
    test_type types[] = {
      i16_t,
      i32_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    test_start(state, "Uses let");
    const char *input = "(sig test (Fn I32))\n"
                        "(fun test ()\n"
                        " (sig b I32)\n"
                        " (let b →3←)\n"
                        " →b←)";
    test_type types[] = {
      i32_t,
      i32_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    test_start(state, "Recognizes bools");
    const char *input = "(sig test (Fn Bool))\n"
                        "(fun test () (let a →False←) →True←)";
    test_type types[] = {
      bool_t,
      bool_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    test_start(state, "Functions can be recursive");
    const char *input = "(sig test (Fn Bool))\n"
                        "(fun test () →(test)←)";
    test_type types[] = {
      bool_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    test_start(state, "Functions can be mutually recursive");
    const char *input = "(sig a (Fn Bool))\n"
                        "(fun a () →(b)←)\n"
                        "(sig b (Fn Bool))\n"
                        "(fun b () →(a)←)\n";
    test_type types[] = {
      bool_t,
      bool_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    test_start(state, "Type inference");
    const char *input = "(sig a (Fn Bool))\n"
                        "(fun a () (let b 3) (i32-gt? →b← 2))";
    test_type types[] = {
      i32_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  test_group_end(state);
}

static void test_errors(test_state *state) {
  test_group_start(state, "Produces errors");

  {
    test_start(state, "I32 vs (Int, Int)");
    const tc_err_test errors[] = {{
      .type = TC_ERR_CONFLICT,
    }};
    static const char *prog = "(sig a (Fn I32))\n"
                              "(fun a () →(2, 3)←)";
    test_typecheck_errors(state, prog, errors, STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs (() -> Int)");
    const test_type i32_t = {
      .tag = T_I32,
      .sub_amt = 0,
      .subs = NULL,
    };
    const test_type got = {
      .tag = T_FN,
      .sub_amt = 1,
      .subs = &i32_t,
    };
    const tc_err_test errors[] = {
      {
        .type = TC_ERR_CONFLICT,
        .type_exp = i32_t,
        .type_got = got,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn I32))\n"
                          "(fun a () →(fn () 1)←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs [I32]");
    const tc_err_test errors[] = {
      {
        .type = TC_ERR_CONFLICT,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn I32))\n"
                          "(fun a () →[3]←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs String");
    const tc_err_test errors[] = {
      {
        .type = TC_ERR_CONFLICT,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn I32))\n"
                          "(fun a () →\"hi\"←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "String vs I32");
    const tc_err_test errors[] = {
      {
        .type = TC_ERR_CONFLICT,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn String))\n"
                          "(fun a () →321←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  /*
  test_start(state, "Blocks must end in expressions");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (let f 1))";
    const tc_err_test errors[] = {
      {
        .type = BLOCK_ENDS_IN_STATEMENT,
      },
    };
    // test_typecheck_errors(state, input, errors, STATIC_LEN(errors));
  }
  test_end(state);
  */

  test_group_end(state);
}

static void test_typecheck_stress(test_state *state) {
  test_start(state, "Stress");
  {
    stringstream ss;
    ss_init_immovable(&ss);
    const unsigned n = 10;
    fputs("(sig a (Fn (", ss.stream);
    for (unsigned i = 1; i < n; i++) {
      fputs("U8,", ss.stream);
    }
    fputs("U8", ss.stream);
    fputs(") (", ss.stream);
    for (unsigned i = 1; i < n; i++) {
      fputs("U8,", ss.stream);
    }
    fputs("U8", ss.stream);
    fputs(")))\n", ss.stream);
    fputs("(fun a (b) b)", ss.stream);
    ss_finalize(&ss);
    test_typecheck_errors(state, ss.string, NULL, 0);
    free(ss.string);
  }
  test_end(state);
}

void test_typecheck(test_state *state) {
  test_group_start(state, "Typecheck");

  test_typecheck_succeeds(state);
  // test_errors(state);
  if (!state->config.lite) {
    test_typecheck_stress(state);
  }

  test_group_end(state);
}
