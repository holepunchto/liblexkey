#include <assert.h>
#include <compact.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <utf.h>
#include "../include/lexkey.h"

static void
roundtrip (const utf8_t *bytes, size_t len) {
  uint8_t buf[64];
  compact_state_t s = {0, 0, NULL};
  utf8_string_view_t in = utf8_string_view_init(bytes, len);
  lexkey_preencode_string(&s, in);
  size_t expected = s.end;

  s.buffer = buf;
  int err = lexkey_encode_string(&s, in);
  assert(err == 0);
  assert(s.start == expected);

  utf8_string_t out;
  utf8_string_init(&out);
  compact_state_t d = {0, s.start, buf};
  err = lexkey_decode_string(&d, &out);
  assert(err == 0);
  assert(out.len == len);
  assert(memcmp(out.data, bytes, len) == 0);
  assert(d.start == s.start);
  utf8_string_destroy(&out);
}

#define LIT(s) ((const utf8_t *) (s))

int
main () {
  roundtrip(LIT(""), 0);
  roundtrip(LIT("a"), 1);
  roundtrip(LIT("hello"), 5);
  roundtrip(LIT("hello world"), 11);

  utf8_t with_zeros[] = {'a', 0, 'b', 0, 0, 'c'};
  roundtrip(with_zeros, sizeof(with_zeros));

  utf8_t all_zeros[] = {0, 0, 0, 0};
  roundtrip(all_zeros, sizeof(all_zeros));

  // Two strings concatenated decode back individually
  {
    uint8_t buf[64];
    compact_state_t s = {0, sizeof(buf), buf};
    lexkey_encode_string(&s, utf8_string_view_init(LIT("foo"), 3));
    lexkey_encode_string(&s, utf8_string_view_init(LIT("bar"), 3));
    size_t total = s.start;

    compact_state_t d = {0, total, buf};
    utf8_string_t out;
    utf8_string_init(&out);

    assert(lexkey_decode_string(&d, &out) == 0);
    assert(out.len == 3 && memcmp(out.data, "foo", 3) == 0);

    assert(lexkey_decode_string(&d, &out) == 0);
    assert(out.len == 3 && memcmp(out.data, "bar", 3) == 0);

    assert(d.start == total);
    utf8_string_destroy(&out);
  }

  // Skip mode (NULL result)
  {
    uint8_t buf[64];
    compact_state_t s = {0, sizeof(buf), buf};
    lexkey_encode_string(&s, utf8_string_view_init(LIT("skip me"), 7));
    compact_state_t d = {0, s.start, buf};
    assert(lexkey_decode_string(&d, NULL) == 0);
    assert(d.start == s.start);
  }

  // Malformed: missing terminator
  {
    uint8_t bad[] = {0x00, 'a', 'b'};
    compact_state_t d = {0, sizeof(bad), bad};
    utf8_string_t out;
    utf8_string_init(&out);
    assert(lexkey_decode_string(&d, &out) == -1);
    utf8_string_destroy(&out);
  }

  // Malformed: bad start
  {
    uint8_t bad[] = {0x01, 'a', 'b', 0x00, 0x01};
    compact_state_t d = {0, sizeof(bad), bad};
    utf8_string_t out;
    utf8_string_init(&out);
    assert(lexkey_decode_string(&d, &out) == -1);
    utf8_string_destroy(&out);
  }

  return 0;
}
