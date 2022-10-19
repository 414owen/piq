#include <stdbool.h>
#include <stdio.h>

#include "diagnostic.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "util.h"

static char *marker_start = "→";
static char *marker_end = "←";

typedef struct test_type {
  type_tag tag;
  const struct test_type *subs;
  const size_t sub_amt;
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
                        tc_err_test test_err, span err_span) {
  tc_error eA = res.errors[err_ind];
  if (eA.type != test_err.type)
    return false;
  parse_node node = tree.nodes[eA.pos];
  if (node.span.start != err_span.start)
    return false;
  if (node.span.end != err_span.end)
    return false;
  switch (eA.type) {
    case TYPE_MISMATCH: {
      return test_type_eq(res.types.types,
                          res.types.type_inds,
                          eA.expected,
                          test_err.type_exp) &&
             test_type_eq(
               res.types.types, res.types.type_inds, eA.got, test_err.type_got);
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

static char *process_spans(const char *input, span **spans_p,
                           node_ind_t cases) {
  size_t len = strlen(input);
  span *spans = malloc(sizeof(span) * cases);
  *spans_p = spans;
  {
    buf_ind_t cursor = 0;
    buf_ind_t offset = 0;
    for (node_ind_t i = 0; i < cases; i++) {
      buf_ind_t start = strstr(&input[cursor], marker_start) - input;
      spans[i].start = start - offset;
      offset += strlen(marker_start);

      buf_ind_t end = strstr(&input[start], marker_end) - input;
      spans[i].end = end - offset;
      offset += strlen(marker_end);
      cursor = end;
    }
  }

  // TODO this doesn't work
  char *edit = malloc(len + 1);
  {
    buf_ind_t cursor = 0;
    node_ind_t span_ind = 0;
    bool start = true;
    buf_ind_t next_ind = spans[span_ind].start;
    buf_ind_t offset = 0;
    for (buf_ind_t i = 0; i < len; i++) {
      if (next_ind + offset == i) {
        if (start) {
          i += strlen(marker_start) - 1;
          offset += strlen(marker_start);
          next_ind = spans[span_ind].end;
        } else {
          i += strlen(marker_end) - 1;
          offset += strlen(marker_end);
          next_ind = spans[++span_ind].start;
        }
        start = !start;
      } else {
        edit[cursor++] = input[i];
      }
    }
    edit[cursor] = '\0';
  }

  for (node_ind_t i = 0; i < cases; i++) {
    spans[i].end -= 1;
  }
  return edit;
}

static void test_types_match(test_state *state, const char *input,
                             test_type *exps, node_ind_t cases) {
  span *spans;
  input = process_spans(input, &spans, cases);

  parse_tree_res pres = test_upto_parse_tree(state, input);
  if (!pres.succeeded) {
    return;
  }

  tc_res res = typecheck(input, pres.tree);

  if (res.error_amt > 0) {
    stringstream *ss = ss_init();
    fprintf(
      ss->stream, "Didn't expect typecheck errors. Got %d:\n", res.error_amt);
    print_tc_errors(ss->stream, input, pres.tree, res);
    failf(state, ss_finalize_free(ss), input);
    return;
  }

  for (size_t i = 0; i < cases; i++) {
    test_type exp = exps[i];
    span span = spans[i];
    bool seen = false;
    for (size_t j = 0; j < pres.tree.node_amt; j++) {
      parse_node node = pres.tree.nodes[j];
      if (node.type == PT_ROOT)
        continue;
      if (node.span.start == span.start && node.span.end == span.end) {
        seen = true;
        if (!test_type_eq(res.types.types,
                          res.types.type_inds,
                          res.types.node_types[j],
                          exp)) {
          stringstream *ss = ss_init();
          print_type(ss->stream, res.types.types, res.types.node_types[j]);
          char *str = ss_finalize_free(ss);
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

static void test_typecheck_errors(test_state *state, const char *input,
                                  const tc_err_test *exps, node_ind_t cases) {

  span *spans;
  input = process_spans(input, &spans, cases);

  parse_tree_res pres = test_upto_parse_tree(state, input);
  if (!pres.succeeded) {
    failf(state, "Parsing failed");
    free_parse_tree_res(pres);
    return;
  }

  tc_res res = typecheck(input, pres.tree);

  if (!all_errors_match(pres.tree, res, exps, spans, cases)) {
    stringstream *ss = ss_init();
    fprintf(ss->stream, "Expected %d errors, got %d.\n", cases, res.error_amt);
    if (res.error_amt > 0) {
      fputs("Errors:\n", ss->stream);
    }
    print_tc_errors(ss->stream, input, pres.tree, res);
    char *str = ss_finalize_free(ss);
    failf(state, str, input);
    free(str);
  }

  free_tc_res(res);
  free_parse_tree_res(pres);
}

static const test_type u8_t = {
  .tag = T_U8,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type i8 = {
  .tag = T_I8,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type i16 = {
  .tag = T_I16,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type i32 = {
  .tag = T_I32,
  .sub_amt = 0,
  .subs = NULL,
};

static const test_type unit = {
  .tag = T_UNIT,
  .sub_amt = 0,
  .subs = NULL,
};

static void test_typecheck_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  {
    const char *input = "(sig a (Fn () →U8←))\n"
                        "(fun a () 2)";
    test_start(state, "Return type U8");
    test_type types[] = {u8_t};
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn () I8))\n"
                        "(fun a () →2←)";
    test_start(state, "Return value");
    test_type types[] = {i8};
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn () I16))\n"
                        "(fun →a← () 2)";
    test_start(state, "Fn bnd");
    const test_type fn_subs[] = { unit, i16 };
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
                        "(fun a 1 [→12←])";
    test_start(state, "[U8] Literals");
    test_type types[] = {
      u8_t,
    };
    test_types_match(state, input, types, STATIC_LEN(types));
    test_end(state);
  }

  {
    const char *input = "(sig a (Fn U8 [U8]))\n"
                        "(fun a 1 (as I32 →2←) [→12←])";
    test_start(state, "Multi-expression block");
    test_type types[] = {
      i32,
      u8_t,
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
      .type = TYPE_HEAD_MISMATCH,
    }};
    static const char *prog = "(sig a (Fn () I32))\n"
                              "(fun a () →(2, 3)←)";
    test_typecheck_errors(state, prog, errors, STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs (() -> Int)");
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
        .type_exp = i32,
        .type_got = got,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn () I32))\n"
                          "(fun a () →(fn () 1)←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs [I32]");
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn () I32))\n"
                          "(fun a () →[3]←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "I32 vs String");
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn () I32))\n"
                          "(fun a () →\"hi\"←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "String vs I32");
    const tc_err_test errors[] = {
      {
        .type = LITERAL_MISMATCH,
      },
    };
    test_typecheck_errors(state,
                          "(sig a (Fn () String))\n"
                          "(fun a () →321←)",
                          errors,
                          STATIC_LEN(errors));
    test_end(state);
  }

  {
    test_start(state, "Param shadows binding");
    test_typecheck_errors(state,
                          "(sig a (Fn () [U8]))\n"
                          "(fun a () "
                          "(sig b (Fn () ()))"
                          "(fun b a a))",
                          NULL,
                          0);
    test_end(state);
  }

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
    test_typecheck_errors(state, ss_finalize_free(ss), NULL, 0);
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

  test_group_end(state);
}
