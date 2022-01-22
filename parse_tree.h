#pragma once

typedef enum {
  PT_PARENS,
  PT_NAME,
  PT_NUM,
} parse_tree_type;

typedef struct {
  parse_ast_type type;
  union {

    // name, num
    struct {
      BUF_IND_T start;
      BUF_IND_T end;
    };

    // parens
    struct {
      uint16_t sub_num;
      uint32_t *subs;
    };
  };
} parse_ast_node;

VEC_DECL(parse_ast_node);

typedef vec_parse_ast_node parse_ast;
