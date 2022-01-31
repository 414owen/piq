%token_prefix TK_
%token_type   int // index into token vec
%default_type int

%type commalist vec_node_ind
%type commapred vec_node_ind
%type param node_ind_tup
%type params vec_node_ind
%type params_tuple vec_node_ind
%type forms vec_node_ind

%include {

  // #include <stdio.h>
  #include <assert.h>

  #include "parse_tree.h"
  #include "token.h"
  #include "vec.h"

  #define YYNOERRORRECOVERY 1

  typedef struct {
    token *tokens;
    parse_tree_res *res;
    NODE_IND_T pos;
  } state;

  #define mk_leaf(type, start, end) \
    ((parse_node) { .type = type, .leaf_node = {.start = start, .end = end}})

  typedef struct {
    NODE_IND_T a;
    NODE_IND_T b;
  } node_ind_tup;
}

%extra_argument { state s }

root ::= form(B) forms(A) EOF . {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &A);
  VEC_PUSH(&s.res->tree.inds, B);
  parse_node n = {.type = PT_ROOT, .subs_start = start, .sub_amt = A.len + 1};
  VEC_PUSH(&s.res->tree.nodes, n);
  s.res->tree.root_ind = s.res->tree.nodes.len - 1;
  VEC_FREE(&A);
}

forms(RES) ::= forms(A) form(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

forms(RES) ::= . {
  vec_node_ind res = VEC_NEW;
  RES = res;
}

form(RES) ::= INT(A).  { 
  token t = s.tokens[A];
  parse_node n = {.type = PT_INT, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

form ::= name.

name ::= upper_name.
name ::= lower_name.

upper_name(RES) ::= UPPER_NAME(A). {
  token t = s.tokens[A];
  parse_node n = {.type = PT_UPPER_NAME, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

lower_name(RES) ::= LOWER_NAME(A). {
  token t = s.tokens[A];
  parse_node n = {.type = PT_LOWER_NAME, .start = t.start, .end = t.end, .sub_amt = 0};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

form(RES) ::= OPEN_PAREN(A) compound(B) CLOSE_PAREN(C). {
  token a = s.tokens[A];
  s.res->tree.nodes.data[B].start = a.start;
  a = s.tokens[C];
  s.res->tree.nodes.data[B].end = a.end;
  RES = B;
}

compound ::= if.

compound ::= fn.

compound ::= tuple.

compound ::= call.

fn(RES) ::= FN OPEN_PAREN params(A) CLOSE_PAREN form(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &A);
  VEC_PUSH(&s.res->tree.inds, B);
  parse_node n = {.type = PT_FN, .subs_start = start, .sub_amt = A.len + 1};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&A);
}

params(RES) ::= . {
  vec_node_ind v = VEC_NEW;
  RES = v;
}

params ::= params_tuple.

params_tuple(RES) ::= param(A). {
  vec_node_ind v = VEC_NEW;
  VEC_PUSH(&v, A.a);
  VEC_PUSH(&v, A.b);
  RES = v;
}

params_tuple(RES) ::= params_tuple(A) COMMA param(B). {
  VEC_PUSH(&A, B.a);
  VEC_PUSH(&A, B.b);
  RES = A;
}

param(RES) ::= lower_name(A) COLON type(B). {
  node_ind_tup res = { .a = A, .b = B };
  RES = res;
}

type ::= upper_name.

tuple(RES) ::= form(A) COMMA commapred(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_CAT(&s.res->tree.inds, &B);
  parse_node n = {.type = PT_TUP, .subs_start = start, .sub_amt = B.len + 1};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&B);
}

call(RES) ::= form(A) forms(B). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_CAT(&s.res->tree.inds, &B);
  parse_node n = {.type = PT_CALL, .subs_start = start, .sub_amt = B.len + 1};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
  VEC_FREE(&B);
}

commapred(RES) ::= commapred(A) COMMA form(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

commapred(RES) ::= form(A). {
  vec_node_ind res = VEC_NEW;
  VEC_PUSH(&res, A);
  RES = res;
}

if(RES) ::= IF form(A) form(B) form(C). {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_PUSH(&s.res->tree.inds, A);
  VEC_PUSH(&s.res->tree.inds, B);
  VEC_PUSH(&s.res->tree.inds, C);
  parse_node n = {.type = PT_IF, .subs_start = start, .sub_amt = 3};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

%syntax_error {
  vec_token_type expected = VEC_NEW;
  if (s.res->succeeded) {
    s.res->error_pos = s.pos;
    for (token_type i = 0; i < YYNTOKEN; i++) {
      int a = yy_find_shift_action((YYCODETYPE)i, yypParser->yytos->stateno);
      if (a != YY_ERROR_ACTION) VEC_PUSH(&expected, i);
    }
    s.res->expected_amt = expected.len;
    s.res->expected = expected.data;
    s.res->succeeded = false;
  }
}

%parse_failure {
  s.res->succeeded = false;
}

%code {
  parse_tree_res parse(vec_token tokens) {
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

    /*
    FILE *f = fopen("parser.log", "w");
    fprintf(f, "\n\n\n");
    ParseTrace(f, "");
    */
    state state = {
      .tokens = tokens.data,
      .res = &res,
      .pos = 0,
    };
    for (; state.pos < tokens.len; state.pos++) {
      Parse(&xp, tokens.data[state.pos].type, state.pos, state);
    }
    Parse(&xp, 0, 0, state);
    ParseFinalize(&xp);

    return res;
  }
}
