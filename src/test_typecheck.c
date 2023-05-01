// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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
  type_check_tag tag;
  union {
    struct {
      size_t amt;
      const struct test_type *arr;
    } subs;
    typevar type_var;
  } data;
} test_type;

VEC_DECL(test_type);

typedef struct {
  tc_error_type type;
  test_type type_exp;
  test_type type_got;
} tc_err_test;

typedef struct {
  enum {
    TYPE_MATCHES,
    PRODUCES_ERRORS,
  } type;
  union {
    struct {
      size_t amt;
      test_type *arr;
    } spans;

    struct {
      size_t amt;
      const tc_err_test *arr;
    } errors;
  } data;
} tc_test;

static bool test_type_eq(type *types, node_ind_t *inds, node_ind_t root,
                         test_type t) {
  vec_node_ind stack_a = VEC_NEW;
  VEC_PUSH(&stack_a, root);
  vec_test_type stack_b = VEC_NEW;
  VEC_PUSH(&stack_b, t);
  bool res = true;

  // this associative array maps typevars in a to typevars in b, and back
  vec_typevar substitution_keys = VEC_NEW;
  vec_typevar substitution_vals = VEC_NEW;

  while (res) {
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
    if (type_a.tag.check != type_b.tag) {
      res = false;
      break;
    }
    switch (type_reprs[type_a.tag.check]) {
      case SUBS_EXTERNAL:
        VEC_APPEND(&stack_a,
                   type_a.data.more_subs.amt,
                   inds + type_a.data.more_subs.start);
        break;
      case SUBS_ONE:
        VEC_PUSH(&stack_a, type_a.data.one_sub.ind);
        break;
      case SUBS_TWO:
        VEC_PUSH(&stack_a, type_a.data.two_subs.a);
        VEC_PUSH(&stack_a, type_a.data.two_subs.b);
        break;
      case SUBS_NONE:
        if (type_a.tag.check == TC_VAR) {
          const size_t ind_k = find_range(VEC_DATA_PTR(&substitution_keys),
                                          sizeof(typevar),
                                          substitution_keys.len,
                                          &type_a.data.type_var,
                                          1);
          const size_t ind_v = find_range(VEC_DATA_PTR(&substitution_vals),
                                          sizeof(typevar),
                                          substitution_keys.len,
                                          &type_b.data.type_var,
                                          1);
          res &= ind_k == ind_v;
          if (ind_k == substitution_keys.len) {
            VEC_PUSH(&substitution_keys, type_a.data.type_var);
            VEC_PUSH(&substitution_vals, type_b.data.type_var);
          }
        }
        continue;
    }
    VEC_APPEND(&stack_b, type_b.data.subs.amt, type_b.data.subs.arr);
  }
  VEC_FREE(&substitution_keys);
  VEC_FREE(&substitution_vals);
  VEC_FREE(&stack_a);
  VEC_FREE(&stack_b);
  return res;
}

static bool test_err_eq(parse_tree tree, tc_res res, size_t err_ind,
                        tc_err_test test_err, span err_span) {
  tc_error eA = res.errors[err_ind];
  if (eA.type != test_err.type)
    return false;
  if (!spans_equal(tree.data.spans[eA.pos], err_span))
    return false;
  switch (eA.type) {
    case TC_ERR_CONFLICT: {
      return test_type_eq(res.types.tree.nodes,
                          res.types.tree.inds,
                          eA.data.conflict.expected_ind,
                          test_err.type_exp) &&
             test_type_eq(res.types.tree.nodes,
                          res.types.tree.inds,
                          eA.data.conflict.got_ind,
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
    print_tc_errors(ss.stream, input, rres.tree.data, res);
    ss_finalize(&ss);
    failf(state, ss.string, input);
    free(ss.string);
  } else {
    for (size_t i = 0; i < cases; i++) {
      test_type exp = exps[i];
      span span = spans[i];
      bool seen = false;
      for (size_t j = 0; j < rres.tree.data.node_amt; j++) {
        if (spans_equal(rres.tree.data.spans[j], span)) {
          seen = true;
          if (!test_type_eq(res.types.tree.nodes,
                            res.types.tree.inds,
                            res.types.node_types[j],
                            exp)) {
            char *type_str;
            char *context_str;
            {
              stringstream ss;
              ss_init_immovable(&ss);
              print_type(ss.stream,
                         res.types.tree.nodes,
                         res.types.tree.inds,
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
  free_parse_tree(rres.tree.data);
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
    if (res.error_amt > 0) {
      fputs("Errors:\n", ss.stream);
    }
    print_tc_errors(ss.stream, input, rres.tree.data, res);
    ss_finalize(&ss);
    failf(state,
          "Expected %d errors, got %d.\n%s",
          cases,
          res.error_amt,
          ss.string);
    free(ss.string);
  }

  free(input);
  free_tc_res(res);
  free_parse_tree(rres.tree.data);
  stfree(spans, span_bytes);
}

static const test_type unit_t = {
  .tag = TC_UNIT,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type bool_t = {
  .tag = TC_BOOL,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type u8_t = {
  .tag = TC_U8,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type string_t = {
  .tag = TC_LIST,
  .data.subs =
    {
      .amt = 1,
      .arr = &u8_t,
    },
};

static const test_type i16_t = {
  .tag = TC_I16,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type i32_t = {
  .tag = TC_I32,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type i64_t = {
  .tag = TC_I64,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type i8_t = {
  .tag = TC_I8,
  .data.subs =
    {
      .amt = 0,
      .arr = NULL,
    },
};

static const test_type VAR_A = {
  .tag = TC_VAR,
  .data.type_var = 0,
};

static const test_type VAR_B = {
  .tag = TC_VAR,
  .data.type_var = 1,
};

static const test_type any_int_type_arr[] = {
  {
    .tag = TC_I8,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_I16,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_I32,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_I64,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_U8,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_U16,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_U32,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
  {
    .tag = TC_U64,
    .data.subs =
      {
        .amt = 0,
        .arr = NULL,
      },
  },
};

static const test_type any_int_t = {
  .tag = TC_OR,
  .data.subs =
    {
      .amt = STATIC_LEN(any_int_type_arr),
      .arr = any_int_type_arr,
    },
};

static void test_typecheck_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  {
    const char *input = "(sig a (Fn →()←))\n"
                        "(fun a () ())";
    test_start(state, "Return type ()");
    test_type types[] = {unit_t};
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

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
    test_start(state, "heterogeneous fn");
    const char *input = "(sig →het← (Fn →I8← →I16← →I32← →I64←))\n"
                        "(fun →het← (→a← →b← →c←) →a← →b← →c← →64←)";
    // params *and* return type
    const test_type het_t_params[] = {
      i8_t,
      i16_t,
      i32_t,
      i64_t,
    };

    const test_type het_t = {
      .tag = TC_FN,
      .data.subs =
        {
          .amt = STATIC_LEN(het_t_params),
          .arr = het_t_params,
        },
    };

    test_type types[] = {
      het_t,
      i8_t,
      i16_t,
      i32_t,
      i64_t,
      het_t,
      i8_t,
      i16_t,
      i32_t,
      i8_t,
      i16_t,
      i32_t,
      i64_t,
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
    test_start(state, "Inner functions can be recursive");
    const char *input = "(sig test (Fn Bool))\n"
                        "(fun test ()\n"
                        "  (sig a (Fn I8))\n"
                        "  (fun a () (a))\n"
                        "  →(test)←)";
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
    const test_type subs[] = {
      VAR_A,
      VAR_B,
    };
    const test_type type = {
      .tag = TC_TUP,
      .data.subs =
        {
          .amt = STATIC_LEN(subs),
          .arr = subs,
        },
    };
    const tc_err_test errors[] = {{
      .type = TC_ERR_CONFLICT,
      .type_exp = i32_t,
      .type_got = type,
    }};
    static const char *prog = "(sig a (Fn I32))\n"
                              "(fun a () →(2, 3)←)";
    test_typecheck_errors(state, prog, errors, STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs (() -> Int)");
    const test_type got = {
      .tag = TC_FN,
      .data.subs =
        {
          .amt = 1,
          .arr = &VAR_A,
        },
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
    const test_type got = {
      .tag = TC_LIST,
      .data.subs =
        {
          .amt = 1,
          .arr = &VAR_A,
        },
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
        .type_exp = i32_t,
        .type_got = string_t,
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
        .type_exp = any_int_t,
        .type_got = string_t,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn String))\n"
                          "(fun a () →321←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "Infinite constraint => ambiguous type");
    const tc_err_test errors[] = {
      {
        .type = TC_ERR_AMBIGUOUS,
      },
    };
    test_typecheck_errors(
      state, "(fun a () →(a)←)", errors, STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "Exact infinite type");
    const tc_err_test errors[] = {
      {
        .type = TC_ERR_INFINITE,
      },
    };
    test_typecheck_errors(state, "(fun a () →a←)", errors, STATIC_LEN(errors));
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

static void test_kitchen_sink(test_state *state) {
  test_group_start(state, "Kitchen sink");

  {
    test_start(state, "One");
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (→a←)\n"
                        "  (let f (if (i32-lte? a 12) i32-add i32-sub))\n"
                        "  (f 3 4))\n";
    test_type types[] = {i32_t};
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

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
  test_errors(state);
  if (!state->config.lite) {
    test_typecheck_stress(state);
  }
  test_kitchen_sink(state);

  test_group_end(state);
}
