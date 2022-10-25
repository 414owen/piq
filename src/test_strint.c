#include <stdlib.h>
#include <stdint.h>

#include "strint.h"
#include "test.h"

static void test_u8_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint8_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u8_parses_base10(test_state *state, char *str, uint8_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint8_t res = parse_uint8_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_u8_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint8_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u8_parses_base16(test_state *state, char *str, uint8_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint8_t res = parse_uint8_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_u8(test_state *state) {
  test_group_start(state, "u8");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u8_overflows_base10(state, "256");
        // overflows on scale *10
        test_u8_overflows_base10(state, "1000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u8_parses_base10(state, "0", 0);
        test_u8_parses_base10(state, "1", 1);
        test_u8_parses_base10(state, "5", 5);
        test_u8_parses_base10(state, "9", 9);
        test_u8_parses_base10(state, "10", 10);
        test_u8_parses_base10(state, "11", 11);
        test_u8_parses_base10(state, "97", 97);
        test_u8_parses_base10(state, "0000000000042", 42);
        test_u8_parses_base10(state, "255", 255);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u8_overflows_base16(state, "100");
        // I don't think there's a way to make base 16 overflow with +digit
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u8_parses_base16(state, "0", 0x0);
        test_u8_parses_base16(state, "1", 0x1);
        test_u8_parses_base16(state, "5", 0x5);
        test_u8_parses_base16(state, "9", 0x9);
        test_u8_parses_base16(state, "16", 0x16);
        test_u8_parses_base16(state, "11", 0x11);
        test_u8_parses_base16(state, "97", 0x97);
        test_u8_parses_base16(state, "0000000000042", 0x42);
        test_u8_parses_base16(state, "fe", 0xfe);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // u8
}

static void test_i8_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int8_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i8_parses_base10(test_state *state, char *str, int8_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int8_t res = parse_int8_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_i8_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int8_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i8_parses_base16(test_state *state, char *str, int8_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int8_t res = parse_int8_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_i8(test_state *state) {
  test_group_start(state, "i8");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        test_i8_overflows_base10(state, "128");
        test_i8_overflows_base10(state, "129");
        test_i8_overflows_base10(state, "256");
        test_i8_overflows_base10(state, "-129");
        // overflows on scale *10
        test_i8_overflows_base10(state, "1000");
        test_i8_overflows_base10(state, "-1000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i8_parses_base10(state, "0", 0);
        test_i8_parses_base10(state, "1", 1);
        test_i8_parses_base10(state, "5", 5);
        test_i8_parses_base10(state, "9", 9);
        test_i8_parses_base10(state, "10", 10);
        test_i8_parses_base10(state, "11", 11);
        test_i8_parses_base10(state, "97", 97);
        test_i8_parses_base10(state, "0000000000042", 42);
        test_i8_parses_base10(state, "127", 127);
        test_i8_parses_base10(state, "-127", -127);
        test_i8_parses_base10(state, "-128", -128);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        test_i8_overflows_base16(state, "80");
        test_i8_overflows_base16(state, "81");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i8_parses_base16(state, "0", 0x0);
        test_i8_parses_base16(state, "1", 0x1);
        test_i8_parses_base16(state, "5", 0x5);
        test_i8_parses_base16(state, "9", 0x9);
        test_i8_parses_base16(state, "16", 0x16);
        test_i8_parses_base16(state, "11", 0x11);
        test_i8_parses_base16(state, "0000000000042", 0x42);
        test_i8_parses_base16(state, "7f", 0x7f);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // i8
}

static void test_u16_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint16_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u16_parses_base10(test_state *state, char *str, uint16_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint16_t res = parse_uint16_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_u16_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint16_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u16_parses_base16(test_state *state, char *str, uint16_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint16_t res = parse_uint16_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_u16(test_state *state) {
  test_group_start(state, "u16");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u16_overflows_base10(state, "65536");
        // overflows on scale *10
        test_u16_overflows_base10(state, "100000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u16_parses_base10(state, "0", 0);
        test_u16_parses_base10(state, "42", 42);
        test_u16_parses_base10(state, "0000000000010000", 10000);
        test_u16_parses_base10(state, "65535", 65535);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u16_overflows_base16(state, "10000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u16_parses_base16(state, "0", 0x0);
        test_u16_parses_base16(state, "42", 0x42);
        test_u16_parses_base16(state, "00000000000fff", 0xfff);
        test_u16_parses_base16(state, "ffff", 0xffff);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // u16
}

static void test_i16_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int16_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i16_parses_base10(test_state *state, char *str, int16_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int16_t res = parse_int16_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_i16_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int16_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i16_parses_base16(test_state *state, char *str, int16_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int16_t res = parse_int16_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_i16(test_state *state) {
  test_group_start(state, "i16");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_i16_overflows_base10(state, "32768");
        test_i16_overflows_base10(state, "-32769");
        // overflows on scale *10
        test_i16_overflows_base10(state, "100000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i16_parses_base10(state, "0", 0);
        test_i16_parses_base10(state, "42", 42);
        test_i16_parses_base10(state, "0000000000010000", 10000);
        test_i16_parses_base10(state, "32767", 32767);
        test_i16_parses_base10(state, "-32768", -32768);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_i16_overflows_base16(state, "8000");
        test_i16_overflows_base16(state, "-8001");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i16_parses_base16(state, "0", 0x0);
        test_i16_parses_base16(state, "42", 0x42);
        test_i16_parses_base16(state, "-42", -0x42);
        test_i16_parses_base16(state, "000000000006000", 0x6000);
        test_i16_parses_base16(state, "-8000", -0x8000);
        test_i16_parses_base16(state, "7fff", 0x7fff);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // i16
}

static void test_u32_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint32_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u32_parses_base10(test_state *state, char *str, uint32_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint32_t res = parse_uint32_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_u32_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint32_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u32_parses_base16(test_state *state, char *str, uint32_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint32_t res = parse_uint32_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_u32(test_state *state) {
  test_group_start(state, "u32");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u32_overflows_base10(state, "4294967296");
        // overflows on scale *10
        test_u32_overflows_base10(state, "10000000000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u32_parses_base10(state, "0", 0);
        test_u32_parses_base10(state, "42", 42);
        test_u32_parses_base10(state, "0000000000010000", 10000);
        test_u32_parses_base10(state, "4294967295", 4294967295);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u32_overflows_base16(state, "100000000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u32_parses_base16(state, "0", 0x0);
        test_u32_parses_base16(state, "42", 0x42);
        test_u32_parses_base16(state, "000000000001234", 0x1234);
        test_u32_parses_base16(state, "ffffffff", 0xffffffff);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // u32
}

static void test_i32_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int32_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i32_parses_base10(test_state *state, char *str, int32_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int32_t res = parse_int32_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_i32_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int32_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i32_parses_base16(test_state *state, char *str, int32_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int32_t res = parse_int32_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_i32(test_state *state) {
  test_group_start(state, "i32");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_i32_overflows_base10(state, "2147483648");
        // overflows on scale *10
        test_i32_overflows_base10(state, "10000000000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i32_parses_base10(state, "0", 0);
        test_i32_parses_base10(state, "42", 42);
        test_i32_parses_base10(state, "0000000000010000", 10000);
        test_i32_parses_base10(state, "2147483647", 2147483647);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_i32_overflows_base16(state, "80000000");
        test_i32_overflows_base16(state, "-80000001");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i32_parses_base16(state, "0", 0x0);
        test_i32_parses_base16(state, "42", 0x42);
        test_i32_parses_base16(state, "000000000001234", 0x1234);
        test_i32_parses_base16(state, "7fffffff", 0x7fffffff);
        test_i32_parses_base16(state, "-80000000", -0x80000000);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // i32
}

static void test_u64_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint64_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u64_parses_base10(test_state *state, char *str, uint64_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint64_t res = parse_uint64_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_u64_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_uint64_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_u64_parses_base16(test_state *state, char *str, uint64_t exp) {
  test_start(state, str);

  bool overflowed = false;
  uint64_t res = parse_uint64_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_u64(test_state *state) {
  test_group_start(state, "u64");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u64_overflows_base10(state, "18446744073709551616");
        // overflows on scale *10
        test_u64_overflows_base10(state, "101111111111111111111");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u64_parses_base10(state, "0", 0);
        test_u64_parses_base10(state, "42", 42);
        test_u64_parses_base10(
          state, "0000000000018446744073709551613", 18446744073709551613u);
        test_u64_parses_base10(
          state, "18446744073709551615", 18446744073709551615u);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_u64_overflows_base16(state, "10000000000000000");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_u64_parses_base16(state, "0", 0x0);
        test_u64_parses_base16(state, "42", 0x42);
        test_u64_parses_base16(state, "000000000001234", 0x1234);
        test_u64_parses_base16(state, "ffffffffffffffff", 0xffffffffffffffff);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // u64
}

static void test_i64_overflows_base10(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int64_base10(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i64_parses_base10(test_state *state, char *str, int64_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int64_t res = parse_int64_base10(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_i64_overflows_base16(test_state *state, char *str) {
  test_start(state, str);

  bool overflowed = false;
  parse_int64_base16(&overflowed, str, strlen(str));
  test_assert(state, overflowed);

  test_end(state);
}

static void test_i64_parses_base16(test_state *state, char *str, int64_t exp) {
  test_start(state, str);

  bool overflowed = false;
  int64_t res = parse_int64_base16(&overflowed, str, strlen(str));
  test_assert(state, !overflowed);
  test_assert_eq(state, res, exp);

  test_end(state);
}

static void test_strint_i64(test_state *state) {
  test_group_start(state, "i64");
  {
    test_group_start(state, "base 10");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_i64_overflows_base10(state, "9223372036854775808");
        // overflows on scale *10
        test_i64_overflows_base10(state, "101111111111111111111");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i64_parses_base10(state, "0", 0);
        test_i64_parses_base10(state, "42", 42);
        test_i64_parses_base10(
          state, "000000000009223372036854775807", 9223372036854775807);
        test_i64_parses_base10(
          state, "9223372036854775807", 9223372036854775807);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 10
    test_group_start(state, "base 16");
    {
      test_group_start(state, "overflow");
      {
        // overflows on +digit
        test_i64_overflows_base16(state, "8000000000000000");
        test_i64_overflows_base16(state, "-8000000000000001");
      }
      test_group_end(state); // overflow

      test_group_start(state, "no overflow");
      {
        test_i64_parses_base16(state, "0", 0x0);
        test_i64_parses_base16(state, "42", 0x42);
        test_i64_parses_base16(state, "000000000001234567890", 0x1234567890);
        test_i64_parses_base16(state, "-8000000000000000", -0x8000000000000000);
        test_i64_parses_base16(state, "7fffffffffffffff", 0x7fffffffffffffff);
      }
      test_group_end(state); // no overflow
    }
    test_group_end(state); // base 16
  }
  test_group_end(state); // i64
}

void test_strint(test_state *state) {
  test_group_start(state, "strint");

  test_strint_u8(state);
  test_strint_i8(state);
  test_strint_u16(state);
  test_strint_i16(state);
  test_strint_u32(state);
  test_strint_i32(state);
  test_strint_u64(state);
  test_strint_i64(state);

  test_group_end(state);
}
