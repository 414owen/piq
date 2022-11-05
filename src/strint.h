#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum {
  INT_FITS,
  UNDERFLOW,
  OVERFLOW,
} flow_type;

uint8_t parse_uint8_base10(bool *overflowed, const char *str,
                           const size_t str_len);
uint16_t parse_uint16_base10(bool *overflowed, const char *str,
                             const size_t str_len);
uint32_t parse_uint32_base10(bool *overflowed, const char *str,
                             const size_t str_len);
uint64_t parse_uint64_base10(bool *overflowed, const char *str,
                             const size_t str_len);

int8_t parse_int8_base10(flow_type *flow, const char *str,
                         const size_t str_len);
int16_t parse_int16_base10(flow_type *flow, const char *str,
                           const size_t str_len);
int32_t parse_int32_base10(flow_type *flow, const char *str,
                           const size_t str_len);
int64_t parse_int64_base10(flow_type *flow, const char *str,
                           const size_t str_len);

uint8_t parse_uint8_base16(bool *overflowed, const char *str,
                           const size_t str_len);
uint16_t parse_uint16_base16(bool *overflowed, const char *str,
                             const size_t str_len);
uint32_t parse_uint32_base16(bool *overflowed, const char *str,
                             const size_t str_len);
uint64_t parse_uint64_base16(bool *overflowed, const char *str,
                             const size_t str_len);

int8_t parse_int8_base16(flow_type *flow, const char *str,
                         const size_t str_len);
int16_t parse_int16_base16(flow_type *flow, const char *str,
                           const size_t str_len);
int32_t parse_int32_base16(flow_type *flow, const char *str,
                           const size_t str_len);
int64_t parse_int64_base16(flow_type *flow, const char *str,
                           const size_t str_len);
