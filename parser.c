#include "consts.h"
#include "parse_tree.h"
#include "source.h"
#include "token.h"

parse_tree build_parse_tree(vec_token tokens) {
  parse_tree res;
  BUF_IND_T token_ind = 0;
  for (;;) {
    token_res tres = scan(file, token_ind);
    if (!tres.succeeded) {
    }
  }
}
