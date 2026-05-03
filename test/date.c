#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <string.h>
#include "../include/lexkey.h"

static void
roundtrip (int64_t ms) {
  uint8_t buf[16];
  compact_state_t s = {0, 0, NULL};
  lexkey_preencode_date(&s, ms);
  size_t expected = s.end;

  s.buffer = buf;
  lexkey_encode_date(&s, ms);
  assert(s.start == expected);

  compact_state_t d = {0, s.start, buf};
  int64_t out;
  assert(lexkey_decode_date(&d, &out) == 0);
  assert(out == ms);
}

int
main () {
  roundtrip(0);
  roundtrip(584172000000LL);
  roundtrip(1473292800000LL);
  roundtrip(1754006400000LL);
  roundtrip(-11491632000000LL);

  // Date shares wire format with INT.
  uint8_t a[16], b[16];
  {
    compact_state_t s = {0, sizeof(a), a};
    lexkey_encode_date(&s, 1473292800000LL);
  }
  {
    compact_state_t s = {0, sizeof(b), b};
    lexkey_encode_int(&s, 1473292800000LL);
  }
  compact_state_t sz = {0, 0, NULL};
  lexkey_preencode_int(&sz, 1473292800000LL);
  assert(memcmp(a, b, sz.end) == 0);

  return 0;
}
