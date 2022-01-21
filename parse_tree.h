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
      struct parse_ast *sub;
    };
  };
} parse_ast;
