%token_prefix TK_
%token_type   int // index into token vec
%default_type node_ind_t

// This is so that parser.h changes less
%token
  EOF
  AS
  CLOSE_BRACKET
  CLOSE_PAREN
  COMMA
  FN
  FN_TYPE
  FUN
  IF
  INT
  LOWER_NAME
  OPEN_BRACKET
  OPEN_PAREN
  SIG
  UPPER_NAME
  LET
  .

%type commaexprs vec_node_ind
%type param node_ind_tup
%type pattern_list vec_node_ind
%type pattern_list_rec vec_node_ind
%type pattern_tuple vec_node_ind
%type pattern_tuple_min vec_node_ind
%type patterns vec_node_ind
%type tuple_min vec_node_ind
%type tuple_rec vec_node_ind
%type type_inner_tuple vec_node_ind
%type params_tuple vec_node_ind
%type toplevels vec_node_ind
%type block vec_node_ind
%type comma_types vec_node_ind
%type int parse_node
%type string parse_node
%type unit parse_node

%include {

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
    node_ind_t root_subs_start;
    node_ind_t root_subs_amt;
    token_type *expected;
    node_ind_t pos;
    node_ind_t error_pos;
    uint8_t expected_amt;
    bool get_expected;
    bool succeeded;
  } state;

  typedef struct {
    node_ind_t a;
    node_ind_t b;
  } node_ind_tup;

  static parse_tree_res parse_internal(token *tokens, size_t token_amt, bool get_expected);

  #ifdef DEBUG
    void break_parser(void) {}
    #define BREAK_PARSER break_parser()
  #else
    #define BREAK_PARSER do {} while(0)
  #endif

  static node_ind_t desugar_tuple(state*, parse_node_type_all,  vec_node_ind);
  static node_ind_t push_node(state *s, parse_node node);
}

%extra_argument { state *s }

root(RES) ::= toplevels(A) EOF . {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_CAT(&s->inds, &A);
  s->root_subs_start = start,
  s->root_subs_amt = A.len,
  VEC_FREE(&A);
  RES = s->nodes.len - 1;
}

toplevels(RES) ::= . {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  RES = res;
}

toplevels(RES) ::= toplevels(A) toplevel(B) . {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

block(RES) ::= stmt(A). {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

block(RES) ::= block(A) stmt(B). {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

stmt ::= expr.

stmt(RES) ::= OPEN_PAREN(O) block_in_parens(A) CLOSE_PAREN(C). {
  BREAK_PARSER;
  token t = s->tokens[O];
  VEC_DATA_PTR(&s->nodes)[A].span.start = t.start;
  t = s->tokens[C];
  VEC_DATA_PTR(&s->nodes)[A].span.end = t.end;
  RES = A;
}

block_in_parens ::= let.
block_in_parens ::= sig.

let(RES) ::= LET lower_name(A) expr(B). {
  BREAK_PARSER;
  parse_node n = {.type = PT_STMT_LET, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

toplevel(RES) ::= OPEN_PAREN(O) toplevel_under(A) CLOSE_PAREN(C). {
  BREAK_PARSER;
  token t = s->tokens[O];
  VEC_DATA_PTR(&s->nodes)[A].span.start = t.start;
  t = s->tokens[C];
  VEC_DATA_PTR(&s->nodes)[A].span.end = t.end;
  RES = A;
}

toplevel_under ::= fun.
toplevel_under ::= sig.

// Don't use directly. Wrap.
int(RES) ::= INT(A).  { 
  BREAK_PARSER;
  token t = s->tokens[A];
  parse_node n = {
    .sub_amt = 0,
    .span = {
      .start = t.start,
      .end = t.end,
    },
  };
  RES = n;
}

name ::= upper_name.
name ::= lower_name.

expr ::= name.

expr(RES) ::= unit(A). {
  A.type.type = PT_EX_UNIT;
  RES = push_node(s, A);
}

expr(RES) ::= string(A). {
  A.type = PT_EX_STRING,
  RES = push_node(s, A);
}

expr(RES) ::= int(A). {
  A.type = PT_EX_INT,
  RES = push_node(s, A);
}

// Don't use directly. Wrap.
string(RES) ::= STRING(A). {
  BREAK_PARSER;
  token t = s->tokens[A];
  parse_node n = {
    .sub_amt = 0,
    .span = {
      .start = t.start,
      .end = t.end,
    },
  };
  RES = push_node(s, n);
}

upper_name(RES) ::= UPPER_NAME(A). {
  BREAK_PARSER;
  token t = s->tokens[A];
  parse_node n = {
    .type = PT_UPPER_NAME,
    .sub_amt = 0,
    .span = {
      .start = t.start,
      .end = t.end,
    },
  };
  RES = push_node(s, n);
}

lower_name(RES) ::= LOWER_NAME(A). {
  BREAK_PARSER;
  token t = s->tokens[A];
  parse_node n = {
    .type = PT_LOWER_NAME,
    .sub_amt = 0,
    .span = {
      .start = t.start,
      .end = t.end,
    },
  };
  RES = push_node(s, n);
}

expr(RES) ::= OPEN_BRACKET(A) list_contents(B) CLOSE_BRACKET(C). {
  BREAK_PARSER;
  token a = s->tokens[A];
  VEC_DATA_PTR(&s->nodes)[B].span.start = a.start;
  a = s->tokens[C];
  VEC_DATA_PTR(&s->nodes)[B].span.end = a.end;
  RES = B;
}

list_contents(RES) ::= commaexprs(C). {
  BREAK_PARSER;
  node_ind_t subs_start = s->inds.len;
  VEC_CAT(&s->inds, &C);
  parse_node n = {
    .type = PT_LIST,
    .subs_start = subs_start,
    .sub_amt = C.len,
  };
  VEC_FREE(&C);
  RES = push_node(s, n);
}

list_contents(RES) ::= . {
  BREAK_PARSER;
  parse_node n = {
    .type = PT_LIST,
    .subs_start = 0,
    .sub_amt = 0,
  };
  RES = push_node(s, n);
}

expr(RES) ::= OPEN_PAREN(A) compound_expr(B) CLOSE_PAREN(C). {
  BREAK_PARSER;
  parse_node *node = VEC_GET_PTR(s->nodes, B);
  node->span.start = s->tokens[A].start;
  node->span.end = s->tokens[C].end;
  RES = B;
}

compound_expr ::= if.

compound_expr ::= fun.

compound_expr ::= tuple.

compound_expr ::= call.

compound_expr ::= typed.

compound_expr ::= fn.

// compound_expr ::= expr.

fn(RES) ::= FN pattern(B) fun_body(C). {
  BREAK_PARSER;
  parse_node n = {.type = PT_FN, .sub_a = B, .sub_b = C};
  RES = push_node(s, n);
}

sig(RES) ::= SIG lower_name(A) type(B). {
  BREAK_PARSER;
  parse_node n = {.type = PT_SIG, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

fun(RES) ::= FUN lower_name(A) pattern(B) fun_body(C). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_PUSH(&s->inds, B);
  VEC_PUSH(&s->inds, C);
  parse_node n = {.type = PT_FUN, .subs_start = start, .sub_amt = 3};
  RES = push_node(s, n);
}

fun_body(RES) ::= block(B). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_CAT(&s->inds, &B);
  parse_node n = {
    .type = PT_FUN_BODY,
    .subs_start = start,
    .sub_amt = B.len,
    .span = {
      .start = VEC_GET(s->nodes, VEC_FIRST(B)).span.start,
      .end = VEC_GET(s->nodes, VEC_LAST(B)).span.end,
    },
  };
  VEC_FREE(&B);
  RES = push_node(s, n);
}

pattern_tuple_min(RES) ::= pattern(A) COMMA pattern(B). {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  node_ind_t inds[2] = {A, B};
  VEC_APPEND_STATIC(&res, inds);
  RES = res;
}

pattern_tuple(RES) ::= pattern_tuple(A) COMMA pattern(B). {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

pattern_tuple ::= pattern_tuple_min.

pattern_list(RES) ::= . {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  RES = res;
}

pattern_list_rec(RES) ::= pattern_list_rec(A) COMMA pattern(B). {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

pattern_list_rec(RES) ::= pattern(A). {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

pattern_list ::= pattern_list_rec.

patterns(RES) ::= pattern(A). {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

patterns(RES) ::= patterns(A) pattern(B). {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

pattern ::= lower_name.

pattern(RES) ::= unit(A). {
  A.type.type = PT_PAT_UNIT;
  RES = push_node(s, A);
}

pattern(RES) ::= int(A). {
  A.type = PT_PAT_INT,
  RES = push_node(s, A);
}

pattern(RES) ::= string(A). {
  A.type = PT_PAT_STRING,
  RES = push_node(s, A);
}

unit(RES) ::= UNIT(A). {
  BREAK_PARSER;
  token t = s->tokens[A];
  parse_node n = {
    .sub_a = 0,
    .sub_b = 0,
    .span = {
      .start = t.start,
      .end = t.end,
    },
  };
  RES = push_node(s, n);
}

pattern(RES) ::= OPEN_PAREN(O) pattern_in_parens(A) CLOSE_PAREN(C). {
  BREAK_PARSER;
  parse_node *node = VEC_GET_PTR(s->nodes, A);
  node->span.start = s->tokens[O].start;
  node->span.end = s->tokens[C].end;
  RES = A;
}

pattern_in_parens ::= pattern_construction.

pattern_in_parens(RES) ::= pattern_tuple(A). {
  RES = desugar_tuple(s, PT_PAT_TUP, A);
}

pattern_construction(RES) ::= upper_name(A) patterns(B). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_PUSH(&s->inds, &A);
  VEC_CAT(&s->inds, &B);
  parse_node n = {
    .type.pattern = PT_PAT_CONSTRUCTION,
    .subs_start = start,
    .sub_amt = B.len + 1,
  };
  VEC_FREE(&B);
  RES = push_node(s, n);
}

pattern(RES) ::= OPEN_BRACKET(O) pattern_list(A) CLOSE_BRACKET(C). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_CAT(&s->inds, &A);
  parse_node n = {
    .type.pattern = PT_PAT_LIST,
    .subs_start = start,
    .sub_amt = A.len,
    .span = {
      .start = s->tokens[O].start,
      .end = s->tokens[C].start,
    },
  };
  VEC_FREE(&A);
  RES = push_node(s, n);
}

type ::= upper_name.

type(RES) ::= unit(A). {
  A.type.type = PT_TY_UNIT;
  RES = push_node(s, A);
}

type(RES) ::= OPEN_PAREN(A) type_inside_parens(B) CLOSE_PAREN(D). {
  BREAK_PARSER;
  VEC_GET_PTR(s->nodes, B)->span.start = s->tokens[A].start;
  VEC_GET_PTR(s->nodes, B)->span.end = s->tokens[D].end;
  RES = B;
}

type(RES) ::= OPEN_BRACKET(O) enclosed_type(A) CLOSE_BRACKET(C). {
  BREAK_PARSER;
  parse_node n = {
    .type.type = PT_TY_LIST,
    .sub_a = A,
    .span = {
      .start = s->tokens[O].start,
      .end = s->tokens[C].start,
    },
  };
  RES = push_node(s, n);
}

enclosed_type(RES) ::= FN_TYPE type(A) type(B). {
  BREAK_PARSER;
  parse_node n = {
    .type.type = PT_TY_FN,
    .sub_a = A,
    .sub_b = B,
  };
  RES = push_node(s, n);
}

enclosed_type ::= type.

type_inner_tuple(RES) ::= enclosed_type(A) COMMA enclosed_type(B). {
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  VEC_PUSH(&res, B);
  RES = res;
}

type_inner_tuple(RES) ::= type_inner_tuple(A) COMMA enclosed_type(B). {
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&A, B);
  RES = A;
}

type_inside_parens ::= enclosed_type.

type_inside_parens(RES) ::= type_inner_tuple(A). {
  BREAK_PARSER;
  RES = desugar_tuple(s, PT_TY_TUP, A);
}

type(RES) ::= OPEN_PAREN(O) type(A) type(B) CLOSE_PAREN(C). {
  BREAK_PARSER;
  parse_node n = {
    .type.type = PT_TY_CONSTRUCTION,
    .sub_a = A,
    .sub_b = B,
    .span = {
      .start = s->tokens[O].start,
      .end = s->tokens[C].end,
    },
  };
  RES = push_node(s, n);
}

typed(RES) ::= AS type(A) expr(B). {
  BREAK_PARSER;
  parse_node n = {.type.expression = PT_EX_AS, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

tuple(RES) ::= tuple_rec(A) COMMA expr(B). {
  BREAK_PARSER;
  // start and end get set by compound_expr
  VEC_PUSH(&A, B);
  RES = desugar_tuple(s, PT_EX_TUP, A);
}

tuple_min(RES) ::= expr(A). {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

tuple_rec(RES) ::= tuple_rec(A) COMMA expr(B). {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

tuple_rec ::= tuple_min.

call(RES) ::= expr(A) expr(B). {
  BREAK_PARSER;
  parse_node n = {.type = PT_CALL, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

commaexprs(RES) ::= expr(A). {
  BREAK_PARSER;
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

commaexprs(RES) ::= commaexprs(A) COMMA expr(B). {
  BREAK_PARSER;
  VEC_PUSH(&A, B);
  RES = A;
}

if(RES) ::= IF expr(A) expr(B) expr(C). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_PUSH(&s->inds, B);
  VEC_PUSH(&s->inds, C);
  parse_node n = {.type.expression = PT_EX_IF, .subs_start = start, .sub_amt = 3};
  RES = push_node(s, n);
}

%syntax_error {
  BREAK_PARSER;
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
}

%parse_failure {
  BREAK_PARSER;
  s->succeeded = false;
}

%code {
  static node_ind_t push_node(state *s, parse_node node) {
    VEC_PUSH(&s->nodes, node);
    return s->nodes.len - 1;
  }

  static node_ind_t desugar_tuple(state *s, parse_node_type_all tag, vec_node_ind elems) {
    node_ind_t node_amt = s->nodes.len;

    // The generated tuples will have their end set to the end
    // of the last syntactic element. Best we can do.
    buf_ind_t inner_end = VEC_GET(s->nodes, elems.len - 1).span.end;

    for (node_ind_t i = 0; i < elems.len - 2; i++) {
      node_ind_t sub_a_ind = VEC_GET(elems, i);
      parse_node n = {
        .type.all = tag,
        .sub_a = sub_a_ind,
        .sub_b = node_amt + i + 1,
        .span = {
          .start = VEC_GET(s->nodes, sub_a_ind).span.start,
          .end = inner_end,
        },
      };
      VEC_PUSH(&s->nodes, n);
    }

    node_ind_t sub_a_ind = VEC_GET(elems, elems.len - 2);
    parse_node last_node = {
      .type.all = tag,
      .sub_a = sub_a_ind,
      .sub_b = VEC_GET(elems, elems.len - 1),
      .span = {
        .start = VEC_GET(s->nodes, sub_a_ind).span.start,
        .end = inner_end,
      },
    };
    VEC_PUSH(&s->nodes, last_node);

    VEC_FREE(&elems);
    return node_amt;
  }

  static parse_tree_res parse_internal(token *tokens, size_t token_amt, bool get_expected) {
    yyParser xp;
    ParseInit(&xp);
    state state = {
      .tokens = tokens,
      .succeeded = true,
      .root_subs_start = 0,
      .root_subs_amt = 0,
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
        .root_subs_start = state.root_subs_start,
        .root_subs_amt = state.root_subs_amt,
      },
      .error_pos = state.error_pos,
      .expected_amt = state.expected_amt,
      .expected = state.expected,
    };
    if (res.succeeded) {
      res.tree.nodes = VEC_FINALIZE(&state.nodes);
      res.tree.inds = VEC_FINALIZE(&state.inds);
    } else {
      VEC_FREE(&state.nodes);
      VEC_FREE(&state.inds);
    }
    return res;
  }

  parse_tree_res parse(token *tokens, size_t token_amt) {
    return parse_internal(tokens, token_amt, true);
  }
}
