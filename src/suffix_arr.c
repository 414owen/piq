#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hedley.h>

#include "attrs.h"
#include "util.h"
#include "suffix_arr.h"
#include "vec.h"

typedef struct {
  uint32_t *arr;
  uint32_t len;
} suffix_cmp_ctx_u32;
 
NON_NULL_PARAMS
int cmp_suffixes_u32(const void *a_par, const void *b_par, const void *ctx_p) {
  uint32_t a = *((uint32_t*) a_par);
  uint32_t b = *((uint32_t*) b_par);
  suffix_cmp_ctx_u32 ctx = *((suffix_cmp_ctx_u32*) ctx_p);
  uint32_t *a_suff = &ctx.arr[a];
  uint32_t *b_suff = &ctx.arr[b];
  uint32_t a_suff_len = ctx.len - a;
  uint32_t b_suff_len = ctx.len - b;
  int res = memcmp(a_suff, b_suff, MIN(a_suff_len, b_suff_len));
  if (res == 0) {
    return a_suff_len < b_suff_len ? -1 : 1;
  }
  return res;
}

NON_NULL_PARAMS
uint32_t upsert_with_suffix_arr_u32(
  vec_u32 *arr,      // haystack
  vec_u32 *suffixes,
  uint32_t *elems,  // needle
  uint32_t elem_amt) {
  const uint32_t arr_len = arr->len;
  suffix_arr_lookup_res preexisting = find_range_with_suffix_array_u32(
    VEC_DATA_PTR(arr),
    VEC_DATA_PTR(suffixes),
    elems,
    arr_len,
    elem_amt
  );
  switch (preexisting.type) {
    case SUFF_FOUND:
      return preexisting.haystack_start;
    case SUFF_RUNS_OFF_END: {
      uint32_t n_els_to_append = arr_len - preexisting.haystack_start;
      uint32_t *els_to_append = &elems[elem_amt - n_els_to_append];
      VEC_APPEND(arr, n_els_to_append, els_to_append);
      break;
    }
    case SUFF_NOTHING: {
      VEC_APPEND(arr, elem_amt, elems);
      break;
    }
  }
}

NON_NULL_PARAMS
suffix_arr_lookup_res find_range_with_suffix_array_u32(
  const uint32_t *restrict haystack, 
  const uint32_t *suffix_array,
  const uint32_t *restrict needle,
  const uint32_t haystack_el_amt,
  const uint32_t needle_el_amt
) {

  // smallest index into the suffix array where the
  // suffix was still greater than the needle
  uint32_t smallest_gt_suffix_arr_ind = haystack_el_amt;
  // greatest index into the suffix array where the
  // suffix was still <= to the needle
  uint32_t greatest_eq_suffix_arr_ind = 0;
  uint32_t suffix_arr_ind = 0;
  {
    uint32_t jmp = haystack_el_amt;
    while (jmp > 0 ) {
      while (suffix_arr_ind + jmp < haystack_el_amt) {
        const uint32_t next_suffix_ind = suffix_arr_ind + jmp;
        const uint32_t next_suffix_start = suffix_array[next_suffix_ind];
        const int res = memcmp(&haystack[next_suffix_start], needle, MIN(needle_el_amt, haystack_el_amt - next_suffix_start));
        if (res < 0) {
          suffix_arr_ind += jmp;
        } else {
          jmp >>= 1;
          if (res == 0) {
            greatest_eq_suffix_arr_ind = MAX(greatest_eq_suffix_arr_ind, next_suffix_start);
          } else {
            smallest_gt_suffix_arr_ind = MIN(smallest_gt_suffix_arr_ind, next_suffix_start);
          }
        }
      }
    }
  }
  greatest_eq_suffix_arr_ind = MAX(greatest_eq_suffix_arr_ind, suffix_arr_ind);
  uint32_t jmp = greatest_eq_suffix_arr_ind - smallest_gt_suffix_arr_ind - 1;
  while (jmp > 0) {
    while (greatest_eq_suffix_arr_ind + jmp < smallest_gt_suffix_arr_ind) {
      const uint32_t next_suffix_ind = suffix_arr_ind + jmp;
      const uint32_t next_suffix_start = suffix_array[next_suffix_ind];
      const int res = memcmp(&haystack[next_suffix_start], needle, MIN(needle_el_amt, haystack_el_amt - next_suffix_start));
      if (res == 0) {
        greatest_eq_suffix_arr_ind += jmp;
      }
    }
  }
  suffix_arr_lookup_res res = {
    .type = SUFF_NOTHING,
    .haystack_start = haystack_el_amt,
  };
  for (uint32_t i = suffix_arr_ind + 1; i < greatest_eq_suffix_arr_ind; i++) {
    uint32_t suffix_start = suffix_array[i];
    res.haystack_start = MIN(res.haystack_start, suffix_start);
  }
  res.type = res.haystack_start + needle_el_amt < haystack_el_amt ? SUFF_FOUND : SUFF_RUNS_OFF_END;
  return res;
}
