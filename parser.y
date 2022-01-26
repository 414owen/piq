%token_prefix T_
%token_type   int // index into token vec
%default_type int

%type commalist vec_node_ind
%type commapred vec_node_ind
%type forms vec_node_ind

%include {

  // #include <stdio.h>
  #include <assert.h>

  #include "parse_tree.h"
  #include "token.h"
  #include "vec.h"

  typedef struct {
    token *tokens;
    parse_tree_res *res;
    NODE_IND_T pos;
  } state;

  #define mk_leaf(type, start, end) \
    ((parse_node) { .type = type, .leaf_node = {.start = start, .end = end}})

}

%extra_argument { state s }

root ::= forms(A) EOF . {
  NODE_IND_T start = s.res->tree.inds.len;
  VEC_CAT(&s.res->tree.inds, &A);
  parse_node n = {.type = PT_ROOT, .subs_start = start, .sub_amt = A.len};
  VEC_PUSH(&s.res->tree.nodes, n);
  s.res->tree.root_ind = s.res->tree.nodes.len - 1;
  VEC_FREE(&A);
}

forms(RES) ::= forms(A) form(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

forms(RES) ::= . { vec_node_ind res = VEC_NEW; RES = res; }

form(RES) ::= NAME(A). {
  token t = s.tokens[A];
  parse_node n = {.type = PT_NAME, .start = t.start, .end = t.end};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

form(RES) ::= INT(A).  { 
  token t = s.tokens[A];
  parse_node n = {.type = PT_INT, .start = t.start, .end = t.end};
  VEC_PUSH(&s.res->tree.nodes, n);
  RES = s.res->tree.nodes.len - 1;
}

form(RES) ::= OPEN_PAREN compound(A) CLOSE_PAREN.{
  RES = A;
}

compound ::= if.

compound ::= tuple.

compound ::= call.

tuple(RES) ::= form(A) COMMA commalist(B). {
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

commalist(RES) ::= commapred(A) form(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

commapred(RES) ::= commapred(A) form(B) COMMA. {
  VEC_PUSH(&A, B);
  RES = A;
}

commapred(RES) ::= . {
  vec_node_ind res = VEC_NEW;
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
  // TODO if we want error recovery
  s.res->error_pos = s.pos;
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

    ParseTrace(stderr, "");
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
