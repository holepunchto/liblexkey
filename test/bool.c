#include <assert.h>
#include <compact.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../include/lexkey.h"

int
main () {
  uint8_t buf[8];

  {
    compact_state_t s = {0, 0, NULL};
    lexkey_preencode_bool(&s, true);
    assert(s.end == 1);
    s.buffer = buf;
    lexkey_encode_bool(&s, true);
    assert(s.start == 1);
    assert(buf[0] == 1);

    compact_state_t d = {0, 1, buf};
    bool b;
    assert(lexkey_decode_bool(&d, &b) == 0);
    assert(b == true);
  }

  {
    compact_state_t s = {0, 8, buf};
    lexkey_encode_bool(&s, false);
    assert(s.start == 1);
    assert(buf[0] == 0);

    compact_state_t d = {0, 1, buf};
    bool b;
    assert(lexkey_decode_bool(&d, &b) == 0);
    assert(b == false);
  }

  // false < true
  {
    uint8_t a[1], c[1];
    compact_state_t sa = {0, 1, a};
    compact_state_t sc = {0, 1, c};
    lexkey_encode_bool(&sa, false);
    lexkey_encode_bool(&sc, true);
    assert(memcmp(a, c, 1) < 0);
  }

  return 0;
}
