#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../include/lexkey.h"

static void
roundtrip (int64_t n) {
  uint8_t buf[16];
  compact_state_t s = {0, 0, NULL};
  lexkey_preencode_int(&s, n);
  size_t expected = s.end;

  s.buffer = buf;
  int err = lexkey_encode_int(&s, n);
  assert(err == 0);
  assert(s.start == expected);

  compact_state_t d = {0, s.start, buf};
  int64_t out;
  err = lexkey_decode_int(&d, &out);
  assert(err == 0);
  if (out != n) {
    fprintf(stderr, "roundtrip failed: in=%lld out=%lld\n", (long long) n, (long long) out);
    assert(0);
  }
  assert(d.start == s.start);
}

static int
cmp_buf (const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len) {
  size_t m = a_len < b_len ? a_len : b_len;
  int r = memcmp(a, b, m);
  if (r != 0) return r;
  if (a_len < b_len) return -1;
  if (a_len > b_len) return 1;
  return 0;
}

static int
encode_into (uint8_t *out, int64_t n) {
  compact_state_t s = {0, 16, out};
  lexkey_encode_int(&s, n);
  return (int) s.start;
}

int
main () {
  roundtrip(0);
  roundtrip(123);
  roundtrip(-123);
  roundtrip(400);
  roundtrip(-400);
  roundtrip(0xf6);
  roundtrip(0xf7);
  roundtrip(0xff);
  roundtrip(-0xff);
  roundtrip(0x100);
  roundtrip(-0x100);
  roundtrip(0xffff);
  roundtrip(-0xffff);
  roundtrip(0x10000);
  roundtrip(-0x10000);
  roundtrip(0xffffffffLL);
  roundtrip(-0xffffffffLL);
  roundtrip(0x100000000LL);
  roundtrip(-0x100000000LL);
  roundtrip(11491632000000LL);
  roundtrip(-11491632000000LL);
  roundtrip(INT64_MAX);
  roundtrip(INT64_MIN);

  // Sentinels
  {
    uint8_t buf[1];
    compact_state_t s = {0, 1, buf};
    lexkey_encode_int_max(&s);
    assert(buf[0] == 0xff);
    compact_state_t d = {0, 1, buf};
    int64_t n;
    assert(lexkey_decode_int(&d, &n) == 1);
  }
  {
    uint8_t buf[1];
    compact_state_t s = {0, 1, buf};
    lexkey_encode_int_min(&s);
    assert(buf[0] == 0x00);
    compact_state_t d = {0, 1, buf};
    int64_t n;
    assert(lexkey_decode_int(&d, &n) == 2);
  }

  // Sort order across the full range, including sentinels.
  {
    int64_t values[] = {
      INT64_MIN,
      -11491632000000LL,
      -0x100000000LL,
      -0xffffffffLL,
      -0x10000LL,
      -0xffffLL,
      -0x100LL,
      -0xffLL,
      -1,
      0,
      1,
      0xf6LL,
      0xf7LL,
      0xffLL,
      0x100LL,
      0xffffLL,
      0x10000LL,
      0xffffffffLL,
      0x100000000LL,
      11491632000000LL,
      INT64_MAX
    };
    size_t n = sizeof(values) / sizeof(values[0]);

    uint8_t minb[1];
    {
      compact_state_t s = {0, 1, minb};
      lexkey_encode_int_min(&s);
    }

    uint8_t maxb[1];
    {
      compact_state_t s = {0, 1, maxb};
      lexkey_encode_int_max(&s);
    }

    uint8_t first[16];
    int first_len = encode_into(first, values[0]);
    assert(cmp_buf(minb, 1, first, first_len) < 0);

    uint8_t prev[16];
    int prev_len = encode_into(prev, values[0]);
    for (size_t i = 1; i < n; i++) {
      uint8_t cur[16];
      int cur_len = encode_into(cur, values[i]);
      int c = cmp_buf(prev, prev_len, cur, cur_len);
      if (c >= 0) {
        fprintf(stderr, "int order broken at i=%zu (value=%lld)\n", i, (long long) values[i]);
        return 1;
      }
      memcpy(prev, cur, cur_len);
      prev_len = cur_len;
    }

    assert(cmp_buf(prev, prev_len, maxb, 1) < 0);
  }

  // Negative ordering by magnitude
  {
    uint8_t a[16], b[16];
    int a_len = encode_into(a, -100);
    int b_len = encode_into(b, -1000000);
    assert(cmp_buf(a, a_len, b, b_len) > 0);
  }
  {
    uint8_t a[16], b[16];
    int a_len = encode_into(a, 100);
    int b_len = encode_into(b, -1000000);
    assert(cmp_buf(a, a_len, b, b_len) > 0);
  }

  return 0;
}
