%token_prefix T_
%token_type   int // index into token vec
%default_type int

%type commalist vec_node_ind

%include {

  // #include <stdio.h>
  #include <assert.h>

  #include "parse_tree.h"
  #include "token.h"
  #include "vec.h"

  typedef struct {
    token *tokens;
    parse_tree_res *res
  } state;

  #define mk_leaf(parse_node_type type, NODE_IND_T start, NODE_IND_T end) \
    ((parse_node) { .type = type, .leaf_node = {.start = start, .end = end}})

}

%extra_argument { state s }

root  ::= forms.

forms ::= forms form.

forms ::= .

form(RES) ::= NAME(A). {
  token t = res.tokens[A];
  parse_node n = {.type = PT_NAME, .start = t.start, .end = t.end};
  VEC_PUSH(&s.res->nodes, n);
  RES = s.res->len - 1;
}

form  ::= INT(A).  { printf("Got int %d!\n", A); }

form  ::= if.

form  ::= tuple.

commalist(RES) ::= commalist(A) COMMA form(B). {
  VEC_PUSH(&A, B);
  RES = A;
}

commalist(RES) ::= form(A) COMMA form(B). {
  vec_node_ind inds = VEC_NEW;
  VEC_PUSH(&inds, A);
  VEC_PUSH(&inds, B);
  RES = inds;
}

tuple(RES) ::= OPEN_PAREN commalist(A) CLOSE_PAREN. {
  NODE_IND_T start = s.res->inds.len;
  VEC_CAT(s.res->inds, A);
  VEC_FREE(A);
  parse_node n = {.type = PT_TUP, .subs_start = start, sub_amt = A.len};
  VEC_PUSH(&s.res->nodes, n);
  RES = s.res->nodes.len - 1;
}

if(RES) ::= OPEN_PAREN IF form(A) form(B) form(C) CLOSE_PAREN. {
  NODE_IND_T start = s.res->inds.len;
  VEC_PUSH(s.res->inds, A);
  VEC_PUSH(s.res->inds, B);
  VEC_PUSH(s.res->inds, C);
  parse_node n = {.type = PT_IF, .subs_start = start, sub_amt = 3};
  VEC_PUSH(&s.res->nodes, n);
  RES = s.res->nodes.len - 1;
}

%syntax_error {
  // TODO
}

%parse_failure {
  // TODO
}

%code {
  parse_tree_res parse(vec_token tokens) {
    yyParser xp;
    ParseInit(&xp);
    parse_tree_res res;
    for (NODE_IND_T i = 0; i < tokens.len; i++) {
      Parse(&xp, tokens.data[i].type, i, &res);
    }
    Parse(&xp, T_EOF, 0, &res);
    ParseFinalize(&xp);
  }

  /*
  static token tt(token_type type) {
    return (token) {.type = type};
  }
  
  int main() {
    vec_token test = VEC_NEW;
    VEC_PUSH(&test, tt(T_NAME));
    VEC_PUSH(&test, tt(T_INT));
    parse(test);
  }
  */
}
