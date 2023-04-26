// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <hedley.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "util.h"
#include "strint.h"

typedef enum {
  BASE_10,
  BASE_16,
} base;

static uint8_t char_to_base16_digit(char c) {
  if (c >= 'a') {
    return c - 'a' + 10;
  } else if (c >= 'A') {
    return c - 'A' + 10;
  }
  return c - '0';
}

// Can be optimized by removing the noinline, but it generates a lot of code.
static HEDLEY_NEVER_INLINE uint64_t parse_arbitrary_positive_base10(
  bool *restrict overflow, const char *restrict str, const size_t str_len,
  uint64_t maximum) {
  uint64_t res = 0;
  uint64_t limit = maximum / 10;
  for (size_t i = 0; i < str_len; i++) {
    if (res > limit) {
      *overflow = true;
      return res;
    }
    res *= 10;
    uint64_t digit = str[i] - '0';
    if (res > maximum - digit) {
      *overflow = true;
      return res;
    }
    res += digit;
  }
  return res;
}

// Can be optimized by removing the noinline, but it generates a lot of code.
static HEDLEY_NEVER_INLINE uint64_t parse_arbitrary_positive_base16(
  bool *restrict overflow, const char *restrict str, const size_t str_len,
  uint64_t maximum) {
  uint64_t res = 0;
  uint64_t limit = maximum / 16;
  for (size_t i = 0; i < str_len; i++) {
    if (res > limit) {
      *overflow = true;
      return res;
    }
    res *= 16;
    uint64_t digit = char_to_base16_digit(str[i]);
    if (res > maximum - digit) {
      *overflow = true;
      return res;
    }
    res += digit;
  }
  return res;
}

// TODO specialize to base 10 and base 16, because then we can use bitshifting
// for base 16 and it will be faster...
static HEDLEY_NEVER_INLINE uint64_t parse_arbitrary(
  flow_type *restrict flow, bool *restrict negative_p, const char *restrict str,
  size_t str_len, base base, int64_t minimum, int64_t maximum) {
  bool negative = false;
  char c = str[0];
  while (c == '-' || c == '+') {
    // I think we can rely on this being merged with the above condition?
    // should check though.
    if (c == '-') {
      negative = !negative;
    }
    str_len -= 1;
    str++;
    c = str[0];
  }
  *negative_p = negative;
  bool overflow = false;
  uint64_t res = 0;
  size_t limit = negative ? 0 - minimum : maximum;
  switch (base) {
    case BASE_10:
      res = parse_arbitrary_positive_base10(&overflow, str, str_len, limit);
      break;
    case BASE_16:
      res = parse_arbitrary_positive_base16(&overflow, str, str_len, limit);
      break;
  }
  *flow = overflow ? (negative ? UNDERFLOW : OVERFLOW) : INT_FITS;
  return res;
}

uint8_t parse_uint8_base10(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  return (uint8_t)parse_arbitrary_positive_base10(
    overflow, str, str_len, UINT8_MAX);
}

uint16_t parse_uint16_base10(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint16_t)parse_arbitrary_positive_base10(
    overflow, str, str_len, UINT16_MAX);
}

uint32_t parse_uint32_base10(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint32_t)parse_arbitrary_positive_base10(
    overflow, str, str_len, UINT32_MAX);
}

uint64_t parse_uint64_base10(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint64_t)parse_arbitrary_positive_base10(
    overflow, str, str_len, UINT64_MAX);
}

int8_t parse_int8_base10(flow_type *flow, const char *restrict str,
                         const size_t str_len) {
  bool negative = false;
  int8_t res = (int8_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_10, INT8_MIN, INT8_MAX);
  return (negative ? -1 : 1) * res;
}

int16_t parse_int16_base10(flow_type *flow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int16_t res = (int16_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_10, INT16_MIN, INT16_MAX);
  return (negative ? -1 : 1) * res;
}

int32_t parse_int32_base10(flow_type *flow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int32_t res = (int32_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_10, INT32_MIN, INT32_MAX);
  return (negative ? -1 : 1) * res;
}

int64_t parse_int64_base10(flow_type *flow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int64_t res = (int64_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_10, INT64_MIN, INT64_MAX);
  return (negative ? -1 : 1) * res;
}

uint8_t parse_uint8_base16(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  return (uint8_t)parse_arbitrary_positive_base16(
    overflow, str, str_len, UINT8_MAX);
}

uint16_t parse_uint16_base16(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint16_t)parse_arbitrary_positive_base16(
    overflow, str, str_len, UINT16_MAX);
}

uint32_t parse_uint32_base16(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint32_t)parse_arbitrary_positive_base16(
    overflow, str, str_len, UINT32_MAX);
}

uint64_t parse_uint64_base16(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint64_t)parse_arbitrary_positive_base16(
    overflow, str, str_len, UINT64_MAX);
}

int8_t parse_int8_base16(flow_type *flow, const char *restrict str,
                         const size_t str_len) {
  bool negative = false;
  int8_t res = (int8_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_16, INT8_MIN, INT8_MAX);
  return (negative ? -1 : 1) * res;
}

int16_t parse_int16_base16(flow_type *flow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int16_t res = (int16_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_16, INT16_MIN, INT16_MAX);
  return (negative ? -1 : 1) * res;
}

int32_t parse_int32_base16(flow_type *flow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int32_t res = (int32_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_16, INT32_MIN, INT32_MAX);
  return (negative ? -1 : 1) * res;
}

int64_t parse_int64_base16(flow_type *flow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int64_t res = (int64_t)parse_arbitrary(
    flow, &negative, str, str_len, BASE_16, INT64_MIN, INT64_MAX);
  return (negative ? -1 : 1) * res;
}
