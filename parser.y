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
    vec_token tokens;
    parse_tree_res *res;
    NODE_IND_T pos;
    bool get_expected;
  } state;

  #define mk_leaf(type, start, end) \
    ((parse_node) { .type = type, .leaf_node = {.start = start, .end = end}})

  typedef struct {
    NODE_IND_T a;
    NODE_IND_T b;
  } node_ind_tup;

  static parse_tree_res parse_internal(vec_token tokens, bool get_expected);
}

%extra_argument { state s }

root(RES) ::= toplevels(A) EOF . {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &A);
  parse_node n = {
    .type = PT_ROOT,
    .subs_start = start,
    .sub_amt = A.len,
    .start = 0,
    .end = 0,
  };
  if (A.len > 0) {
    n.start = VEC_GET(s.res->tree.nodes, VEC_GET(A, 0)).start;
    n.end = VEC_GET(s.res->tree.nodes, VEC_GET(A, A.len - 1)).end;
  }
  VEC_PUSH(&s.res->tree.nodes, n);
  s.res->tree.root_ind = s.res->tree.nodes.len - 1;
  VEC_FREE(&A);
  RES = s.res->tree.nodes.len - 1;
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
  token t = VEC_GET(s.tokens, O);
  VEC_GET(s.res->tree.nodes, A).start = t.start;
  t = VEC_GET(s.tokens, C);
  VEC_GET(s.res->tree.nodes, A).end = t.end;
  RES = A;
}

toplevel_under ::= fun.

int(RES) ::= INT(A).  { 
  token t = VEC_GET(s.tokens, A);
  parse_node n = {.type = PT_INT, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
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
  token t = VEC_GET(s.tokens, A);
  parse_node n = {.type = PT_STRING, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

upper_name(RES) ::= UPPER_NAME(A). {
  token t = VEC_GET(s.tokens, A);
  parse_node n = {.type = PT_UPPER_NAME, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

lower_name(RES) ::= LOWER_NAME(A). {
  token t = VEC_GET(s.tokens, A);
  parse_node n = {.type = PT_LOWER_NAME, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

expr(RES) ::= OPEN_BRACKET(A) list_contents(B) CLOSE_BRACKET(C). {
  token a = VEC_GET(s.tokens, A);
  VEC_GET(s.res->tree.nodes, B).start = a.start;
  a = VEC_GET(s.tokens, C);
  VEC_GET(s.res->tree.nodes, B).end = a.end;
  RES = B;
}

list_contents(RES) ::= commapred(C). {
  NODE_IND_T subs_start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &C);
  parse_node n = {
    .type = PT_LIST,
    .subs_start = subs_start,
    .sub_amt = C.len,
  };
  VEC_PUSH(&s.res->tree.nodes, n);
  VEC_FREE(&C);
  RES = s.res->tree.nodes.len - 1;
}

list_contents(RES) ::= . {
  parse_node n = {
    .type = PT_LIST,
    .subs_start = 0,
    .sub_amt = 0,
  };
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

expr(RES) ::= OPEN_PAREN(A) compound_expr(B) CLOSE_PAREN(C). {
  token a = VEC_GET(s.tokens, A);
  VEC_GET(s.res->tree.nodes, B).start = a.start;
  a = VEC_GET(s.tokens, C);
  VEC_GET(s.res->tree.nodes, B).end = a.end;
  RES = B;
}

compound_expr ::= if.

compound_expr ::= fun.

compound_expr ::= tuple.

compound_expr ::= call.

compound_expr ::= typed.

compound_expr ::= fn.

fn(RES) ::= FN pattern(B) fun_body(C). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, B);
  VEC_PUSH(&s.res->tree.inds, C);
  parse_node n = {.type = PT_FN, .subs_start = start, .sub_amt = 2};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

fun(RES) ::= FUN lower_name(A) pattern(B) fun_body(C). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_PUSH(&s.res->tree.inds, B);
  VEC_PUSH(&s.res->tree.inds, C);
  parse_node n = {.type = PT_FUN, .subs_start = start, .sub_amt = 3};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

fun_body(RES) ::= expr(A) exprs(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_CAT(&s.res->tree.inds, &B);
  parse_node n = {.type = PT_FUN_BODY, .subs_start = start, .sub_amt = B.len + 1};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
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
  token t = VEC_GET(s.tokens, A);
  parse_node n = {
    .type = PT_UNIT,
    .start = t.start,
    .end = t.end,
    .sub_a = 0,
    .sub_b = 0
  };
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

pattern(RES) ::= OPEN_PAREN(O) pattern_tuple(A) CLOSE_PAREN(C). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &A);
  parse_node n = {
    .type = PT_TUP,
    .start = VEC_GET(s.tokens, O).start,
    .end = VEC_GET(s.tokens, C).end,
    .subs_start = start,
    .sub_amt = A.len
  };
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&A);
}

pattern(RES) ::= OPEN_PAREN(O) upper_name(A) patterns(B) CLOSE_PAREN(C). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, &A);
  VEC_CAT(&s.res->tree.inds, &B);
  parse_node n = {
    .type = PT_CONSTRUCTION,
    .start = VEC_GET(s.tokens, O).start,
    .end = VEC_GET(s.tokens, C).end,
    .subs_start = start,
    .sub_amt = B.len + 1
  };
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&B);
}

pattern(RES) ::= OPEN_BRACKET pattern_tuple(A) CLOSE_BRACKET. {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &A);
  parse_node n = {.type = PT_LIST, .subs_start = start, .sub_amt = A.len};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&A);
}

type ::= upper_name.

typed(RES) ::= AS type(A) expr(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_PUSH(&s.res->tree.inds, B);
  parse_node n = {.type = PT_AS, .subs_start = start, .sub_amt = 2};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

tuple(RES) ::= expr(A) COMMA commapred(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_CAT(&s.res->tree.inds, &B);
  parse_node n = {
    .type = PT_TUP,
    .subs_start = start,
    .sub_amt = B.len + 1,
  };
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&B);
}

call(RES) ::= expr(A) exprs(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_CAT(&s.res->tree.inds, &B);
  parse_node n = {.type = PT_CALL, .subs_start = start, .sub_amt = B.len + 1};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
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
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_PUSH(&s.res->tree.inds, B);
  VEC_PUSH(&s.res->tree.inds, C);
  parse_node n = {.type = PT_IF, .subs_start = start, .sub_amt = 3};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

%syntax_error {
  if (s.res->succeeded) {
    vec_token_type expected = VEC_NEW;
    s.res->succeeded = false;
    s.res->error_pos = s.pos;
    if (s.get_expected) {
      token_type backup = VEC_GET(s.tokens, s.pos).type;
      for (token_type i = 0; i < YYNTOKEN; i++) {
        VEC_GET(s.tokens, s.pos).type = i;
        parse_tree_res sub = parse_internal(s.tokens, false);
        if (sub.error_pos > s.pos) {
          VEC_PUSH(&expected, i);
        }
      }
      VEC_GET(s.tokens, s.pos).type = backup;
      s.res->expected_amt = expected.len;
      s.res->expected = VEC_FINALIZE(&expected);
    }
  }
}

%parse_failure {
  s.res->succeeded = false;
}

%code {
  static parse_tree_res parse_internal(vec_token tokens, bool get_expected) {
    yyParser xp;
    ParseInit(&xp);
    parse_tree_res res =  {
      .succeeded = true,
      .tree = {
        .root_ind = 0,
        .nodes = VEC_NEW,
        .inds = VEC_NEW,
      },
    };
    state state = {
      .tokens = tokens,
      .res = &res,
      .pos = 0,
      .get_expected = get_expected,
    };
    for (; state.pos < tokens.len; state.pos++) {
      Parse(&xp, VEC_GET(tokens, state.pos).type, state.pos, state);
    }
    Parse(&xp, 0, 0, state);
    ParseFinalize(&xp);

    return res;
  }

  parse_tree_res parse(vec_token tokens) {
    return parse_internal(tokens, true);
  }
}
