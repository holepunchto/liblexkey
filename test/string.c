#include <assert.h>
#include <compact.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/lexkey.h"

static void
roundtrip (const char *str, size_t len) {
  uint8_t buf[64];
  compact_state_t s = {0, 0, NULL};
  lexkey_preencode_string(&s, str, len);
  size_t expected = s.end;

  s.buffer = buf;
  int err = lexkey_encode_string(&s, str, len);
  assert(err == 0);
  assert(s.start == expected);

  char out[64];
  size_t out_len;
  compact_state_t d = {0, s.start, buf};
  err = lexkey_decode_string(&d, out, sizeof(out), &out_len);
  assert(err == 0);
  assert(out_len == len);
  assert(memcmp(out, str, len) == 0);
  assert(d.start == s.start);
}

int
main () {
  roundtrip("", 0);
  roundtrip("a", 1);
  roundtrip("hello", 5);
  roundtrip("hello world", 11);

  uint8_t with_zeros[] = {'a', 0, 'b', 0, 0, 'c'};
  roundtrip((const char *) with_zeros, sizeof(with_zeros));

  uint8_t all_zeros[] = {0, 0, 0, 0};
  roundtrip((const char *) all_zeros, sizeof(all_zeros));

  // Two strings concatenated decode back individually
  {
    uint8_t buf[64];
    compact_state_t s = {0, sizeof(buf), buf};
    lexkey_encode_string(&s, "foo", 3);
    lexkey_encode_string(&s, "bar", 3);
    size_t total = s.start;

    compact_state_t d = {0, total, buf};
    char out[16];
    size_t out_len;

    assert(lexkey_decode_string(&d, out, sizeof(out), &out_len) == 0);
    assert(out_len == 3 && memcmp(out, "foo", 3) == 0);

    assert(lexkey_decode_string(&d, out, sizeof(out), &out_len) == 0);
    assert(out_len == 3 && memcmp(out, "bar", 3) == 0);

    assert(d.start == total);
  }

  // Skip mode (NULL result)
  {
    uint8_t buf[64];
    compact_state_t s = {0, sizeof(buf), buf};
    lexkey_encode_string(&s, "skip me", 7);
    compact_state_t d = {0, s.start, buf};
    size_t out_len;
    assert(lexkey_decode_string(&d, NULL, 0, &out_len) == 0);
    assert(out_len == 7);
    assert(d.start == s.start);
  }

  // Malformed: missing terminator
  {
    uint8_t bad[] = {0x00, 'a', 'b'};
    compact_state_t d = {0, sizeof(bad), bad};
    char out[8];
    size_t out_len;
    assert(lexkey_decode_string(&d, out, sizeof(out), &out_len) == -1);
  }

  // Malformed: bad start
  {
    uint8_t bad[] = {0x01, 'a', 'b', 0x00, 0x01};
    compact_state_t d = {0, sizeof(bad), bad};
    char out[8];
    size_t out_len;
    assert(lexkey_decode_string(&d, out, sizeof(out), &out_len) == -1);
  }

  return 0;
}
