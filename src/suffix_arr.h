#include <stdint.h>

typedef enum {
  // Found an exact match
  SUFF_FOUND,
  // Found a common prefix that runs to the end
  // This lets us just append the missing elements to the end of the array.
  SUFF_RUNS_OFF_END,
  // Couldn't find wither of the above
  SUFF_NOTHING,
} suffix_lookup_type;

typedef struct {
  suffix_lookup_type type;
  uint32_t haystack_start;
} suffix_arr_lookup_res;

suffix_arr_lookup_res find_range_with_suffix_array_u32(
  const uint32_t *restrict haystack, 
  const uint32_t *suffix_array,
  const uint32_t *restrict needle,
  const uint32_t el_amt,
  const uint32_t needle_el_amt
);
