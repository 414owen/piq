#include <hedley.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

static uint8_t char_to_base10_digit(char c) { return c - '0'; }

static uint8_t char_to_base16_digit(char c) {
  if (c >= 'a') {
    return c - 'a' + 10;
  } else if (c >= 'A') {
    return c - 'A' + 10;
  }
  return c - '0';
}

typedef uint8_t (*char_to_digit)(char c);

// Can be optimized by removing the noinline, but it generates a lot of code.
static HEDLEY_NEVER_INLINE uint64_t parse_arbitrary_positive(
  bool *restrict overflow, const char *restrict str, const size_t str_len,
  char_to_digit f, uint8_t base, size_t maximum) {
  uint64_t res = 0;
  uint64_t limit = maximum / base;
  for (size_t i = 0; i < str_len; i++) {
    if (res > limit) {
      *overflow = true;
      return res;
    }
    res *= base;
    uint64_t digit = f(str[i]);
    if (res > maximum - digit) {
      *overflow = true;
      return res;
    }
    res += digit;
  }
  return res;
}

static HEDLEY_NEVER_INLINE uint64_t
parse_arbitrary(bool *restrict overflow, bool *restrict negative_p,
                const char *restrict str, size_t str_len, char_to_digit f,
                uint8_t base, size_t minimum, size_t maximum) {
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
  return parse_arbitrary_positive(
    overflow, str, str_len, f, base, negative ? 0 - minimum : maximum);
}

uint8_t parse_uint8_base10(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  return (uint8_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base10_digit, 10, UINT8_MAX);
}

uint16_t parse_uint16_base10(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint16_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base10_digit, 10, UINT16_MAX);
}

uint32_t parse_uint32_base10(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint32_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base10_digit, 10, UINT32_MAX);
}

uint64_t parse_uint64_base10(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint64_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base10_digit, 10, UINT64_MAX);
}

int8_t parse_int8_base10(bool *restrict overflow, const char *restrict str,
                         const size_t str_len) {
  bool negative = false;
  int8_t res = (int8_t)parse_arbitrary(overflow,
                                       &negative,
                                       str,
                                       str_len,
                                       char_to_base10_digit,
                                       10,
                                       INT8_MIN,
                                       INT8_MAX);
  return (negative ? -1 : 1) * res;
}

int16_t parse_int16_base10(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int16_t res = (int16_t)parse_arbitrary(overflow,
                                         &negative,
                                         str,
                                         str_len,
                                         char_to_base10_digit,
                                         10,
                                         INT16_MIN,
                                         INT16_MAX);
  return (negative ? -1 : 1) * res;
}

int32_t parse_int32_base10(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int32_t res = (int32_t)parse_arbitrary(overflow,
                                         &negative,
                                         str,
                                         str_len,
                                         char_to_base10_digit,
                                         10,
                                         INT32_MIN,
                                         INT32_MAX);
  return (negative ? -1 : 1) * res;
}

int64_t parse_int64_base10(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int64_t res = (int64_t)parse_arbitrary(overflow,
                                         &negative,
                                         str,
                                         str_len,
                                         char_to_base10_digit,
                                         10,
                                         INT64_MIN,
                                         INT64_MAX);
  return (negative ? -1 : 1) * res;
}

uint8_t parse_uint8_base16(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  return (uint8_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base16_digit, 16, UINT8_MAX);
}

uint16_t parse_uint16_base16(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint16_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base16_digit, 16, UINT16_MAX);
}

uint32_t parse_uint32_base16(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint32_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base16_digit, 16, UINT32_MAX);
}

uint64_t parse_uint64_base16(bool *restrict overflow, const char *restrict str,
                             const size_t str_len) {
  return (uint64_t)parse_arbitrary_positive(
    overflow, str, str_len, char_to_base16_digit, 16, UINT64_MAX);
}

int8_t parse_int8_base16(bool *restrict overflow, const char *restrict str,
                         const size_t str_len) {
  bool negative = false;
  int8_t res = (int8_t)parse_arbitrary(overflow,
                                       &negative,
                                       str,
                                       str_len,
                                       char_to_base16_digit,
                                       16,
                                       INT8_MIN,
                                       INT8_MAX);
  return (negative ? -1 : 1) * res;
}

int16_t parse_int16_base16(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int16_t res = (int16_t)parse_arbitrary(overflow,
                                         &negative,
                                         str,
                                         str_len,
                                         char_to_base16_digit,
                                         16,
                                         INT16_MIN,
                                         INT16_MAX);
  return (negative ? -1 : 1) * res;
}

int32_t parse_int32_base16(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int32_t res = (int32_t)parse_arbitrary(overflow,
                                         &negative,
                                         str,
                                         str_len,
                                         char_to_base16_digit,
                                         16,
                                         INT32_MIN,
                                         INT32_MAX);
  return (negative ? -1 : 1) * res;
}

int64_t parse_int64_base16(bool *restrict overflow, const char *restrict str,
                           const size_t str_len) {
  bool negative = false;
  int64_t res = (int64_t)parse_arbitrary(overflow,
                                         &negative,
                                         str,
                                         str_len,
                                         char_to_base16_digit,
                                         16,
                                         INT64_MIN,
                                         INT64_MAX);
  return (negative ? -1 : 1) * res;
}
