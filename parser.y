%token_prefix TK_
%token_type   int // index into token vec
%default_type NODE_IND_T

// This is so that parser.h changes less
%token
  EOF
  AS
  CLOSE_BRACKET
  CLOSE_PAREN
  COMMA
  FN
  FUN
  IF
  INT
  LOWER_NAME
  OPEN_BRACKET
  OPEN_PAREN
  UPPER_NAME
  .

%type commapred vec_node_ind
%type param node_ind_tup
%type pattern_tuple vec_node_ind
%type patterns vec_node_ind
%type params_tuple vec_node_ind
%type exprs vec_node_ind
%type toplevels vec_node_ind

%include {

  // #include <stdio.h>
  #include <assert.h>

  #include "parse_tree.h"
  #include "token.h"
  #include "util.h"
  #include "vec.h"

  #define YYNOERRORRECOVERY 1
  #define YYSTACKDEPTH 0

  typedef struct {
    token *tokens;
    vec_parse_node nodes;
    vec_node_ind inds;
    token_type *expected;
    NODE_IND_T pos;
    NODE_IND_T root_ind;
    NODE_IND_T error_pos;
    uint8_t expected_amt;
    bool get_expected;
    bool succeeded;
  } state;

  #define mk_leaf(type, start, end) \
    ((parse_node) { .type = type, .leaf_node = {.start = start, .end = end}})

  typedef struct {
    NODE_IND_T a;
    NODE_IND_T b;
  } node_ind_tup;

  static parse_tree_res parse_internal(token *tokens, size_t token_amt, bool get_expected);
}

%extra_argument { state *s }

root(RES) ::= toplevels(A) EOF . {
  NODE_IND_T start = s->inds.len;
  VEC_CAT(&s->inds, &A);
  parse_node n = {
    .type = PT_ROOT,
    .subs_start = start,
    .sub_amt = A.len,
    .start = 0,
    .end = 0,
  };
  if (A.len > 0) {
    n.start = VEC_GET(s->nodes, VEC_GET(A, 0)).start;
    n.end = VEC_GET(s->nodes, VEC_GET(A, A.len - 1)).end;
  }
  VEC_PUSH(&s->nodes, n);
  s->root_ind = s->nodes.len - 1;
  VEC_FREE(&A);
  RES = s->nodes.len - 1;
}

toplevels(RES) ::= . {
  vec_node_ind res = VEC_NEW;
  RES = res;
}

toplevels(RES) ::= toplevels(A) toplevel(B) . {
  VEC_PUSH(&A, B);
  RES = A;
}

toplevel(RES) ::= OPEN_PAREN(O) toplevel_under(A) CLOSE_PAREN(C). {
  token t = s->tokens[O];
  NODE_IND_T A_visible = A;
  VEC_DATA_PTR(&s->nodes)[A].start = t.start;
  t = s->tokens[C];
  VEC_DATA_PTR(&s->nodes)[A].end = t.end;
  RES = A;
}

toplevel_under ::= fun.

int(RES) ::= INT(A).  { 
  token t = s->tokens[A];
  parse_node n = {.type = PT_INT, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

exprs(RES) ::= exprs(A) expr(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

exprs(RES) ::= . {
  vec_node_ind res = VEC_NEW;
  RES = res;
}

expr ::= name.

name ::= upper_name.
name ::= lower_name.

expr ::= string.

expr ::= int.

string(RES) ::= STRING(A). {
  token t = s->tokens[A];
  parse_node n = {.type = PT_STRING, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

upper_name(RES) ::= UPPER_NAME(A). {
  token t = s->tokens[A];
  parse_node n = {.type = PT_UPPER_NAME, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

lower_name(RES) ::= LOWER_NAME(A). {
  token t = s->tokens[A];
  parse_node n = {.type = PT_LOWER_NAME, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

expr(RES) ::= OPEN_BRACKET(A) list_contents(B) CLOSE_BRACKET(C). {
  token a = s->tokens[A];
  VEC_DATA_PTR(&s->nodes)[B].start = a.start;
  a = s->tokens[C];
  VEC_DATA_PTR(&s->nodes)[B].end = a.end;
  RES = B;
}

list_contents(RES) ::= commapred(C). {
  NODE_IND_T subs_start = s->inds.len;
  VEC_CAT(&s->inds, &C);
  parse_node n = {
    .type = PT_LIST,
    .subs_start = subs_start,
    .sub_amt = C.len,
  };
  VEC_PUSH(&s->nodes, n);
  VEC_FREE(&C);
  RES = s->nodes.len - 1;
}

list_contents(RES) ::= . {
  parse_node n = {
    .type = PT_LIST,
    .subs_start = 0,
    .sub_amt = 0,
  };
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

expr(RES) ::= OPEN_PAREN(A) compound_expr(B) CLOSE_PAREN(C). {
  token a = s->tokens[A];
  VEC_DATA_PTR(&s->nodes)[B].start = a.start;
  a = s->tokens[C];
  VEC_DATA_PTR(&s->nodes)[B].end = a.end;
  RES = B;
}

compound_expr ::= if.

compound_expr ::= fun.

compound_expr ::= tuple.

compound_expr ::= call.

compound_expr ::= typed.

compound_expr ::= fn.

fn(RES) ::= FN pattern(B) fun_body(C). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, B);
  VEC_PUSH(&s->inds, C);
  parse_node n = {.type = PT_FN, .subs_start = start, .sub_amt = 2};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

fun(RES) ::= FUN lower_name(A) pattern(B) fun_body(C). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_PUSH(&s->inds, B);
  VEC_PUSH(&s->inds, C);
  parse_node n = {.type = PT_FUN, .subs_start = start, .sub_amt = 3};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

fun_body(RES) ::= expr(A) exprs(B). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_CAT(&s->inds, &B);
  parse_node n = {.type = PT_FUN_BODY, .subs_start = start, .sub_amt = B.len + 1};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
  VEC_FREE(&B);
}

pattern_tuple(RES) ::= pattern(A). {
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

pattern_tuple(RES) ::= pattern_tuple(A) COMMA pattern(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

patterns(RES) ::= pattern(A). {
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

patterns(RES) ::= patterns(A) pattern(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

pattern ::= lower_name.

pattern ::= unit.

pattern ::= int.

unit(RES) ::= UNIT(A). {
  token t = s->tokens[A];
  parse_node n = {
    .type = PT_UNIT,
    .start = t.start,
    .end = t.end,
    .sub_a = 0,
    .sub_b = 0
  };
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

pattern(RES) ::= OPEN_PAREN(O) pattern_tuple(A) CLOSE_PAREN(C). {
  NODE_IND_T start = s->inds.len;
  VEC_CAT(&s->inds, &A);
  parse_node n = {
    .type = PT_TUP,
    .start = s->tokens[O].start,
    .end = s->tokens[C].end,
    .subs_start = start,
    .sub_amt = A.len
  };
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
  VEC_FREE(&A);
}

pattern(RES) ::= OPEN_PAREN(O) upper_name(A) patterns(B) CLOSE_PAREN(C). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, &A);
  VEC_CAT(&s->inds, &B);
  parse_node n = {
    .type = PT_CONSTRUCTION,
    .start = s->tokens[O].start,
    .end = s->tokens[C].end,
    .subs_start = start,
    .sub_amt = B.len + 1
  };
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
  VEC_FREE(&B);
}

pattern(RES) ::= OPEN_BRACKET pattern_tuple(A) CLOSE_BRACKET. {
  NODE_IND_T start = s->inds.len;
  VEC_CAT(&s->inds, &A);
  parse_node n = {.type = PT_LIST, .subs_start = start, .sub_amt = A.len};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
  VEC_FREE(&A);
}

type ::= upper_name.

typed(RES) ::= AS type(A) expr(B). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_PUSH(&s->inds, B);
  parse_node n = {.type = PT_AS, .subs_start = start, .sub_amt = 2};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

tuple(RES) ::= expr(A) COMMA commapred(B). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_CAT(&s->inds, &B);
  parse_node n = {
    .type = PT_TUP,
    .subs_start = start,
    .sub_amt = B.len + 1,
  };
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
  VEC_FREE(&B);
}

call(RES) ::= expr(A) exprs(B). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_CAT(&s->inds, &B);
  parse_node n = {.type = PT_CALL, .subs_start = start, .sub_amt = B.len + 1};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
  VEC_FREE(&B);
}

commapred(RES) ::= commapred(A) COMMA expr(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

commapred(RES) ::= expr(A). {
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

if(RES) ::= IF expr(A) expr(B) expr(C). {
  NODE_IND_T start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_PUSH(&s->inds, B);
  VEC_PUSH(&s->inds, C);
  parse_node n = {.type = PT_IF, .subs_start = start, .sub_amt = 3};
  VEC_PUSH(&s->nodes, n);
  RES = s->nodes.len - 1;
}

%syntax_error {
  vec_token_type expected = VEC_NEW;
  if (s->succeeded) {
    s->error_pos = s->pos;
    for (token_type i = 0; i < YYNTOKEN; i++) {
      int a = yy_find_shift_action((YYCODETYPE)i, yypParser->yytos->stateno);
      if (a != YY_ERROR_ACTION) VEC_PUSH(&expected, i);
    }
    s->expected_amt = expected.len;
    s->expected = VEC_FINALIZE(&expected);
    s->succeeded = false;
  }

/*
  // re-run the parser to find valid tokens
  if (s->succeeded) {
    vec_token_type expected = VEC_NEW;
    s->succeeded = false;
    s->error_pos = s->pos;
    if (s->get_expected) {
      token_type backup = s->tokens[s]pos).type;
      for (token_type i = 0; i < YYNTOKEN; i++) {
        VEC_DATA_PTR(&s->tokens)[s->pos].type = i;
        parse_tree_res sub = parse_internal(s->tokens, false);
        if (sub.error_pos > s->pos) {
          VEC_PUSH(&expected, i);
        }
      }
      VEC_DATA_PTR(&s->tokens)[s->pos].type = backup;
      s->expected_amt = expected.len;
      s->expected = VEC_FINALIZE(&expected);
    }
  }
*/
}

%parse_failure {
  s->succeeded = false;
}

%code {
  static parse_tree_res parse_internal(token *tokens, size_t token_amt, bool get_expected) {
    yyParser xp;
    ParseInit(&xp);
    state state = {
      .tokens = tokens,
      .succeeded = true,
      .root_ind = 0,
      .nodes = VEC_NEW,
      .inds = VEC_NEW,
      .pos = 0,
      .get_expected = get_expected,
      .error_pos = -1,
      .expected_amt = 0,
      .expected = NULL,
    };
    for (; state.pos < token_amt; state.pos++) {
      Parse(&xp, tokens[state.pos].type, state.pos, &state);
    }
    Parse(&xp, 0, 0, &state);
    ParseFinalize(&xp);

    parse_tree_res res =  {
      .succeeded = state.succeeded,
      .tree = {
        .node_amt = state.nodes.len,
        .root_ind = state.root_ind,
        .nodes = VEC_FINALIZE(&state.nodes),
        .inds = VEC_FINALIZE(&state.inds),
      },
      .error_pos = state.error_pos,
      .expected_amt = state.expected_amt,
      .expected = state.expected,
    };
    return res;
  }

  parse_tree_res parse(token *tokens, size_t token_amt) {
    return parse_internal(tokens, token_amt, true);
  }
}
