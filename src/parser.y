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
  DATA
  .

%type commaexressions stack_ref_t
%type param node_ind_tup
%type pattern_list stack_ref_t
%type pattern_list_rec stack_ref_t
%type pattern_tuple stack_ref_t
%type pattern_tuple_min stack_ref_t
%type patterns stack_ref_t
%type tuple_min stack_ref_t
%type tuple_rec stack_ref_t
%type type_inner_tuple stack_ref_t
%type toplevels stack_ref_t
%type block stack_ref_t
%type int parse_node
%type string parse_node
%type unit parse_node
%type lower_name_node parse_node

%include {

  #include <assert.h>
  #include <time.h>

  #include "defs.h"
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
    vec_node_ind ind_stack;
  } state;

  typedef node_ind_t stack_ref_t;

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

  static node_ind_t desugar_tuple(state*, parse_node_type_all, stack_ref_t);
  static node_ind_t push_node(state *s, parse_node node);

  static buf_ind_t after_token_end(token t) {
    return t.start + t.len;
  }

  static span span_from_token(token t) {
    span res= {
      .start = t.start,
      .len = t.len,
    };
    return res;
  }

  static span span_from_tokens(token start, token end) {
    span res = {
      .start = start.start,
      .len = after_token_end(end) - start.start,
    };
    return res;
  }

  static span span_from_token_inds(token *tokens, node_ind_t a, node_ind_t b) {
    return span_from_tokens(tokens[a], tokens[b]);
  }

  static span span_from_nodes(parse_node start, parse_node end) {
    span res = {
      .start = start.span.start,
      .len = end.span.start - start.span.start + end.span.len,
    };
    return res;
  }

  static span span_from_node_inds(parse_node *nodes, node_ind_t start_ind, node_ind_t end_ind) {
    return span_from_nodes(nodes[start_ind], nodes[end_ind]);
  }
}

%extra_argument { state *s }

root(RES) ::= toplevels(A) EOF . {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_APPEND(&s->inds, A, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - A]);
  VEC_POP_N(&s->ind_stack, A);
  s->root_subs_start = start,
  s->root_subs_amt = A,
  RES = s->nodes.len - 1;
}

toplevels(RES) ::= . {
  BREAK_PARSER;
  RES = 0;
}

toplevels(RES) ::= toplevels(A) toplevel(B) . {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

block(RES) ::= statement(A). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, A);
  RES = 1;
}

block(RES) ::= block(A) statement(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

statement(RES) ::= expression(A). {
  RES = A;
}

statement(RES) ::= OPEN_PAREN(O) block_in_parens(A) CLOSE_PAREN(C). {
  BREAK_PARSER;
  // TODO take actual parse_node, instead of ind
  VEC_DATA_PTR(&s->nodes)[A].span = span_from_token_inds(s->tokens, O, C);
  RES = A;
}

block_in_parens(RES) ::= let(A). {
  RES = A;
}
block_in_parens(RES) ::= sig(A). {
  RES = A;
}

let(RES) ::= LET lower_name(A) expression(B). {
  BREAK_PARSER;
  parse_node n = {
    .type.statement = PT_STATEMENT_LET,
    .sub_a = A,
    .sub_b = B
  };
  RES = push_node(s, n);
}

toplevel(RES) ::= OPEN_PAREN(O) toplevel_under(A) CLOSE_PAREN(C). {
  BREAK_PARSER;
  // TODO redo to take a node
  VEC_DATA_PTR(&s->nodes)[A].span = span_from_token_inds(s->tokens, O, C);
  RES = A;
}

toplevel_under(RES) ::= fun(A). {
  RES = A;
}

toplevel_under(RES) ::= sig(A). {
  RES = A;
}

toplevel_under(RES) ::= datatype_decl(A). {
  RES = A;
}

datatype_decl(RES) ::=
  DATA
  type_param_decls(A)
  data_constructor_decls(B). {
  parse_node n = {
    .type.statement = PT_STATEMENT_DATA_DECLARATION,
    .sub_a = A,
    .sub_b = B,
  };
  RES = push_node(s, n);
}

data_constructor_decl(RES) ::=
  OPEN_PAREN(OO)
  // this is assumed to be below P on the ind_stack
  // if that turns out to be false, I can return a node
  // from upper_name
  upper_name(A)
  OPEN_PAREN
  data_constructor_params(P)
  CLOSE_PAREN
  CLOSE_PAREN(CO). {

  node_ind_t subs_start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_APPEND(&s->inds, P + 1, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - P - 1]);
  parse_node n = {
    .type.all = PT_ALL_MULTI_DATA_DECL,
    .span = span_from_token_inds(s->tokens, OO, CO),
    .subs_start = subs_start,
    .sub_amt = P + 2,
  };
  RES = push_node(&s->ind_stack, n);
}

data_constructor_params(RES) ::= . {
  RES = 0;
}

data_constructor_params(RES) ::= data_constructor_params(A) type(B). {
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

data_constructor_decls(RES) ::= data_constructor_decl(A). {
  VEC_PUSH(&s->ind_stack, A);
  RES = 1;
}

data_constructor_decls(RES) ::= data_constructor_decls(A) data_constructor_decl(B). {
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

type_param_decls(RES) ::= OPEN_BRACKET(O) type_params(P) CLOSE_BRACKET(C). {
  node_ind_t subs_start = s->inds.len;
  VEC_APPEND(&s->inds, P, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - P]);
  parse_node n = {
    .type.statement = PT_ALL_MULTI_TYPE_PARAMS,
    .sub_amt = P,
    .subs_start = subs_start,
    .span = span_from_token_inds(s->tokens, O, C),
  };
  RES = push_node(s, n);
}

type_params(RES) ::= . {
  RES = 0;
}

type_params(RES) ::= type_params(A) lower_name_node(L). {
  VEC_PUSH(&s->ind_stack, L);
  RES = A + 1;
}

// Don't use directly. Wrap.
int(RES) ::= INT(A).  { 
  BREAK_PARSER;
  parse_node n = {
    .sub_amt = 0,
    .span = span_from_token(s->tokens[A]),
  };
  RES = n;
}

expression(RES) ::= lower_name(A). {
  RES = A;
}

expression(RES) ::= upper_name(A). {
  RES = A;
}

expression(RES) ::= unit(A). {
  A.type.expression = PT_EX_UNIT;
  RES = push_node(s, A);
}

expression(RES) ::= string(A). {
  A.type.expression = PT_EX_STRING,
  RES = push_node(s, A);
}

expression(RES) ::= int(A). {
  A.type.expression = PT_EX_INT,
  RES = push_node(s, A);
}

// Don't use directly. Wrap.
string(RES) ::= STRING(A). {
  BREAK_PARSER;
  parse_node n = {
    .sub_amt = 0,
    .span = span_from_token(s->tokens[A]),
  };
  RES = n;
}

upper_name(RES) ::= UPPER_NAME(A). {
  BREAK_PARSER;
  parse_node n = {
    // works in all contexts
    .type.all = PT_ALL_MULTI_UPPER_NAME,
    .sub_amt = 0,
    .span = span_from_token(s->tokens[A]),
  };
  RES = push_node(s, n);
}

lower_name_node(RES) ::= LOWER_NAME(A). {
  BREAK_PARSER;
  parse_node n = {
     // works in all contexts
    .type.all = PT_ALL_MULTI_LOWER_NAME,
    .sub_amt = 0,
    .span = span_from_token(s->tokens[A]),
  };
  RES = n;
}

lower_name(RES) ::= lower_name_node(A). {
  BREAK_PARSER;
  RES = push_node(s, A);
}

expression(RES) ::= OPEN_BRACKET(A) expression_list_contents(B) CLOSE_BRACKET(C). {
  BREAK_PARSER;
  VEC_DATA_PTR(&s->nodes)[B].span = span_from_token_inds(s->tokens, A, C);
  RES = B;
}

expression_list_contents(RES) ::= commaexressions(A). {
  BREAK_PARSER;
  node_ind_t subs_start = s->inds.len;
  VEC_APPEND(&s->inds, A, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - A]);
  parse_node n = {
    .type.expression = PT_EX_LIST,
    .subs_start = subs_start,
    .sub_amt = A,
  };
  VEC_POP_N(&s->ind_stack, A);
  RES = push_node(s, n);
}

expression_list_contents(RES) ::= . {
  BREAK_PARSER;
  parse_node n = {
    .type.expression = PT_EX_LIST,
    .subs_start = 0,
    .sub_amt = 0,
  };
  RES = push_node(s, n);
}

expression(RES) ::= OPEN_PAREN(A) compound_expression(B) CLOSE_PAREN(C). {
  BREAK_PARSER;
  parse_node *node = VEC_GET_PTR(s->nodes, B);
  node->span = span_from_token_inds(s->tokens, A, C);
  RES = B;
}

compound_expression(RES) ::= if(A). {
  RES = A;
}

compound_expression(RES) ::= fun(A). {
  RES = A;
}

compound_expression(RES) ::= tuple(A). {
  RES = A;
}

compound_expression(RES) ::= call(A). {
  RES = A;
}

compound_expression(RES) ::= typed(A). {
  RES = A;
}

compound_expression(RES) ::= fn(A). {
  RES = A;
}

// compound_expression ::= expression.

fn(RES) ::= FN pattern(B) fun_body(C). {
  BREAK_PARSER;
  parse_node n = {
    .type.expression = PT_EX_FN,
    .sub_a = B,
    .sub_b = C
  };
  RES = push_node(s, n);
}

sig(RES) ::= SIG lower_name(A) type(B). {
  BREAK_PARSER;
  parse_node n = {.type.statement = PT_STATEMENT_SIG, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

fun(RES) ::= FUN lower_name(A) pattern(B) fun_body(C). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_PUSH(&s->inds, A);
  VEC_PUSH(&s->inds, B);
  VEC_PUSH(&s->inds, C);
  parse_node n = {
    .type.statement = PT_STATEMENT_FUN,
    .subs_start = start,
    .sub_amt = 3
  };
  RES = push_node(s, n);
}

fun_body(RES) ::= block(A). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_APPEND(&s->inds, A, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - A]);
  parse_node n = {
    .type.expression = PT_EX_FUN_BODY,
    .subs_start = start,
    .sub_amt = A,
    .span = span_from_node_inds(VEC_DATA_PTR(&s->nodes), VEC_GET(s->ind_stack, s->ind_stack.len - A), VEC_LAST(s->ind_stack)),
  };
  VEC_POP_N(&s->ind_stack, A);
  RES = push_node(s, n);
}

pattern_tuple_min(RES) ::= pattern(A) COMMA pattern(B). {
  BREAK_PARSER;
  node_ind_t inds[2] = {A, B};
  VEC_APPEND_STATIC(&s->ind_stack, inds);
  RES = 2;
}

pattern_tuple(RES) ::= pattern_tuple(A) COMMA pattern(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

pattern_tuple(RES) ::= pattern_tuple_min(A). {
  RES = A;
}

pattern_list(RES) ::= . {
  BREAK_PARSER;
  RES = 0;
}

pattern_list_rec(RES) ::= pattern_list_rec(A) COMMA pattern(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

pattern_list_rec(RES) ::= pattern(A). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, A);
  RES = 1;
}

pattern_list(RES) ::= pattern_list_rec(A). {
  RES = A;
}

patterns(RES) ::= pattern(A). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, A);
  RES = 1;
}

patterns(RES) ::= patterns(A) pattern(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

pattern(RES) ::= lower_name_node(A). {
  A.type.pattern = PT_PAT_WILDCARD;
  RES = push_node(s, A);
}

pattern(RES) ::= unit(A). {
  A.type.pattern = PT_PAT_UNIT;
  RES = push_node(s, A);
}

pattern(RES) ::= int(A). {
  A.type.pattern = PT_PAT_INT;
  RES = push_node(s, A);
}

pattern(RES) ::= string(A). {
  A.type.pattern = PT_PAT_STRING;
  RES = push_node(s, A);
}

unit(RES) ::= UNIT(A). {
  BREAK_PARSER;
  token t = s->tokens[A];
  parse_node n = {
    .sub_a = 0,
    .sub_b = 0,
    .span = span_from_token(s->tokens[A]),
  };
  RES = n;
}

pattern(RES) ::= OPEN_PAREN(O) pattern_in_parens(A) CLOSE_PAREN(C). {
  BREAK_PARSER;
  parse_node *node = VEC_GET_PTR(s->nodes, A);
  node->span = span_from_token_inds(s->tokens, O, C);
  RES = A;
}

pattern_in_parens(RES) ::= pattern_construction(A). {
  RES = A;
}

pattern_in_parens(RES) ::= pattern_tuple(A). {
  RES = desugar_tuple(s, PT_ALL_PAT_TUP, A);
}

pattern_construction(RES) ::= upper_name(A) patterns(B). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_PUSH(&s->inds, &A);
  VEC_APPEND(&s->inds, B, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - B]);
  VEC_POP_N(&s->ind_stack, B);
  parse_node n = {
    .type.pattern = PT_PAT_CONSTRUCTION,
    .subs_start = start,
    .sub_amt = B + 1,
  };
  RES = push_node(s, n);
}

pattern(RES) ::= OPEN_BRACKET(O) pattern_list(A) CLOSE_BRACKET(C). {
  BREAK_PARSER;
  node_ind_t start = s->inds.len;
  VEC_APPEND(&s->inds, A, &VEC_DATA_PTR(&s->ind_stack)[s->ind_stack.len - A]);
  VEC_POP_N(&s->ind_stack, A);
  parse_node n = {
    .type.pattern = PT_PAT_LIST,
    .subs_start = start,
    .sub_amt = A,
    .span = span_from_token_inds(s->tokens, O, C),
  };
  RES = push_node(s, n);
}

type(RES) ::= upper_name(A). {
  RES = A;
}

type(RES) ::= OPEN_BRACKET(O) type_inside_brackets(B) CLOSE_BRACKET(C). {
  BREAK_PARSER;
  parse_node n = {
    .type.type = PT_TY_LIST,
    .sub_a = B,
    .span = span_from_token_inds(s->tokens, O, C),
  };
  RES = push_node(s, n);
}

type_inside_brackets(RES) ::= type_inside_parens(A). {
  RES = A;
}

type_inside_brackets(RES) ::= type(A). {
  RES = A;
}

type(RES) ::= unit(A). {
  A.type.type = PT_TY_UNIT;
  RES = push_node(s, A);
}

type(RES) ::= OPEN_PAREN(A) type_inside_parens_or_tuple(B) CLOSE_PAREN(D). {
  BREAK_PARSER;
  VEC_GET_PTR(s->nodes, B)->span = span_from_token_inds(s->tokens, A, D);
  RES = B;
}

type_inside_parens(RES) ::= fn_type(A). {
  RES = A;
}

type_inside_parens_or_tuple(RES) ::= type_inner_tuple(A). {
  BREAK_PARSER;
  RES = desugar_tuple(s, PT_ALL_TY_TUP, A);
}

type_inside_parens_or_tuple(RES) ::= type_inside_parens(A). {
  RES = A;
}

// TODO multi param types
type_inside_parens(RES) ::= type(A) type(B). {
  BREAK_PARSER;
  parse_node n = {
    .type.type = PT_TY_CONSTRUCTION,
    .sub_a = A,
    .sub_b = B,
  };
  RES = push_node(s, n);
}

fn_type(RES) ::= FN_TYPE type(A) type(B). {
  BREAK_PARSER;
  parse_node n = {
    .type.type = PT_TY_FN,
    .sub_a = A,
    .sub_b = B,
  };
  RES = push_node(s, n);
}

type_inner_tuple(RES) ::= type(A) COMMA type(B). {
  BREAK_PARSER;
  node_ind_t inds[] = {A, B};
  VEC_APPEND_STATIC(&s->ind_stack, inds);
  RES = 2;
}

type_inner_tuple(RES) ::= type_inner_tuple(A) COMMA type(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

typed(RES) ::= AS type(A) expression(B). {
  BREAK_PARSER;
  parse_node n = {.type.expression = PT_EX_AS, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

tuple(RES) ::= tuple_rec(A) COMMA expression(B). {
  BREAK_PARSER;
  // start and end get set by compound_expression
  VEC_PUSH(&s->ind_stack, B);
  RES = desugar_tuple(s, PT_ALL_EX_TUP, A + 1);
}

tuple_min(RES) ::= expression(A). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, A);
  RES = 1;
}

tuple_rec(RES) ::= tuple_rec(A) COMMA expression(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

tuple_rec(RES) ::= tuple_min(A). {
  RES = A;
}

call(RES) ::= expression(A) expression(B). {
  BREAK_PARSER;
  parse_node n = {.type.expression = PT_EX_CALL, .sub_a = A, .sub_b = B};
  RES = push_node(s, n);
}

commaexressions(RES) ::= expression(A). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, A);
  RES = 1;
}

commaexressions(RES) ::= commaexressions(A) COMMA expression(B). {
  BREAK_PARSER;
  VEC_PUSH(&s->ind_stack, B);
  RES = A + 1;
}

if(RES) ::= IF expression(A) expression(B) expression(C). {
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

  static node_ind_t desugar_tuple(state *s, parse_node_type_all tag, stack_ref_t el_amount) {
    vec_node_ind ind_stack = s->ind_stack;
    node_ind_t node_amt = s->nodes.len;

    // The generated tuples will have their end set to the end
    // of the last syntactic element. Best we can do.
    parse_node last_inner_node = VEC_GET(s->nodes, VEC_PEEK(ind_stack));
    buf_ind_t after_inner_end = last_inner_node.span.start + last_inner_node.span.len;

    for (node_ind_t i = 0; i < el_amount - 2; i++) {
      node_ind_t sub_a_ind = VEC_GET(ind_stack, ind_stack.len - el_amount + i);
      buf_ind_t current_node_start = VEC_GET(s->nodes, sub_a_ind).span.start;
      parse_node n = {
        .type.all = tag,
        .sub_a = sub_a_ind,
        .sub_b = node_amt + i + 1,
        .span = {
          .start = current_node_start,
          .len = after_inner_end - current_node_start,
        },
      };
      VEC_PUSH(&s->nodes, n);
    }

    node_ind_t sub_a_ind = VEC_GET(ind_stack, ind_stack.len - 2);
    buf_ind_t first_node_buf_start = VEC_GET(s->nodes, sub_a_ind).span.start;
    parse_node last_node = {
      .type.all = tag,
      .sub_a = sub_a_ind,
      .sub_b = VEC_LAST(ind_stack),
      .span = {
        .start = first_node_buf_start,
        .len = after_inner_end - first_node_buf_start,
      },
    };
    VEC_PUSH(&s->nodes, last_node);

    VEC_POP_N(&s->ind_stack, el_amount);
    return node_amt;
  }

  static parse_tree_res parse_internal(token *tokens, size_t token_amt, bool get_expected) {
#ifdef TIME_PARSER
    struct timespec start = get_monotonic_time();
#endif
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
      // TODO remove? I think this is something to do with error checking that isn't
      // necessary anymore
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
    VEC_FREE(&state.ind_stack);
  #ifdef TIME_PARSER
    res.time_taken = time_since_monotonic(start);
  #endif
    return res;
  }

  parse_tree_res parse(token *tokens, size_t token_amt) {
    return parse_internal(tokens, token_amt, true);
  }
}
