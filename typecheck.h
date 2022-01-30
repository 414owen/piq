#include "parse_tree.h"
#include "vec.h"

typedef enum {
  I8, U8, I16, U16, I32, U32, I64, U64,
  
} type;

VEC_DECL(type);

typedef struct {
  NODE_IND_T pos;
  type expected;
  type got;
} tc_error;

VEC_DECL(tc_error);

typedef struct {
  bool successful;
  union {
    vec_tc_error errors;
    vec_type types;
  };
} tc_res;