#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../include/lexkey.h"

static void
roundtrip (uint64_t n) {
  uint8_t buf[16];
  compact_state_t s = {0, 0, NULL};
  int err = lexkey_preencode_uint(&s, n);
  assert(err == 0);
  size_t expected = s.end;

  s.buffer = buf;
  err = lexkey_encode_uint(&s, n);
  assert(err == 0);
  assert(s.start == expected);

  compact_state_t d = {0, s.start, buf};
  uint64_t out;
  err = lexkey_decode_uint(&d, &out);
  assert(err == 0);
  assert(out == n);
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
encode_into (uint8_t *out, uint64_t n) {
  compact_state_t s = {0, 16, out};
  lexkey_encode_uint(&s, n);
  return (int) s.start;
}

int
main () {
  roundtrip(0);
  roundtrip(1);
  roundtrip(0xfb);
  roundtrip(0xfc);
  roundtrip(0xff);
  roundtrip(0xffff);
  roundtrip(0x10000);
  roundtrip(0xffffffffULL);
  roundtrip(0x100000000ULL);
  roundtrip(UINT64_MAX);

  // Size correctness
  {
    compact_state_t s = {0, 0, NULL};
    lexkey_preencode_uint(&s, 0);
    assert(s.end == 1);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, 0xfb);
    assert(s.end == 1);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, 0xfc);
    assert(s.end == 3);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, 0xffff);
    assert(s.end == 3);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, 0x10000);
    assert(s.end == 5);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, 0xffffffffULL);
    assert(s.end == 5);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, 0x100000000ULL);
    assert(s.end == 9);
    s = (compact_state_t) {0, 0, NULL};
    lexkey_preencode_uint(&s, UINT64_MAX);
    assert(s.end == 9);
  }

  // Max sentinel
  {
    uint8_t buf[1];
    compact_state_t s = {0, 0, NULL};
    lexkey_preencode_uint_max(&s);
    assert(s.end == 1);
    s.buffer = buf;
    lexkey_encode_uint_max(&s);
    assert(s.start == 1);
    assert(buf[0] == 0xff);

    compact_state_t d = {0, 1, buf};
    uint64_t n;
    int r = lexkey_decode_uint(&d, &n);
    assert(r == 1);
  }

  // Sort order
  {
    uint64_t values[] = {0, 1, 0xfb, 0xfc, 0xff, 0xffff, 0x10000, 0xffffffffULL, 0x100000000ULL, UINT64_MAX};
    size_t n = sizeof(values) / sizeof(values[0]);
    uint8_t prev[16];
    int prev_len = 0;
    for (size_t i = 0; i < n; i++) {
      uint8_t cur[16];
      int cur_len = encode_into(cur, values[i]);
      if (i > 0) {
        int c = cmp_buf(prev, prev_len, cur, cur_len);
        if (c >= 0) {
          fprintf(stderr, "uint order broken at i=%zu (value=%llu)\n", i, (unsigned long long) values[i]);
          return 1;
        }
      }
      memcpy(prev, cur, cur_len);
      prev_len = cur_len;
    }

    uint8_t maxb[1] = {0};
    compact_state_t s = {0, 1, maxb};
    lexkey_encode_uint_max(&s);
    int c = cmp_buf(prev, prev_len, maxb, 1);
    assert(c < 0);
  }

  // Out-of-bounds decode
  {
    compact_state_t d = {0, 0, NULL};
    uint64_t n;
    assert(lexkey_decode_uint(&d, &n) == -1);
  }
  {
    uint8_t buf[1] = {0xfc};
    compact_state_t d = {0, 1, buf};
    uint64_t n;
    assert(lexkey_decode_uint(&d, &n) == -1);
  }

  // Skip mode (NULL result)
  {
    uint8_t buf[16];
    compact_state_t s = {0, sizeof(buf), buf};
    lexkey_encode_uint(&s, 0xdeadbeef);
    compact_state_t d = {0, s.start, buf};
    assert(lexkey_decode_uint(&d, NULL) == 0);
    assert(d.start == s.start);
  }

  return 0;
}
