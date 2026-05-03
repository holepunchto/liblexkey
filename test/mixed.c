// Mirrors the JS "basic" test: encode (UINT, STRING) tuples, sort the encoded
// bytes lexicographically, and verify they come back in the expected order.

#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include "../include/lexkey.h"

typedef struct {
  uint64_t u;
  const char *s;
  size_t s_len;
  uint8_t buf[64];
  size_t len;
} item_t;

static void
encode (item_t *it, uint64_t u, const char *s) {
  size_t s_len = strlen(s);
  compact_state_t st = {0, sizeof(it->buf), it->buf};
  lexkey_encode_uint(&st, u);
  lexkey_encode_string(&st, utf8_string_view_init((const utf8_t *) s, s_len));
  it->u = u;
  it->s = s;
  it->s_len = s_len;
  it->len = st.start;
}

static int
cmp (const void *a, const void *b) {
  const item_t *x = (const item_t *) a;
  const item_t *y = (const item_t *) b;
  size_t m = x->len < y->len ? x->len : y->len;
  int r = memcmp(x->buf, y->buf, m);
  if (r != 0) return r;
  if (x->len < y->len) return -1;
  if (x->len > y->len) return 1;
  return 0;
}

int
main () {
  item_t items[7];
  encode(&items[0], 0, "a");
  encode(&items[1], 0, "b");
  encode(&items[2], 0, "c");
  encode(&items[3], 1, "a");
  encode(&items[4], 2, "a");
  encode(&items[5], 300, "c");
  encode(&items[6], 400, "c");

  item_t shuffled[7];
  int order[] = {3, 0, 5, 6, 1, 4, 2};
  for (int i = 0; i < 7; i++) shuffled[i] = items[order[i]];

  qsort(shuffled, 7, sizeof(item_t), cmp);

  for (int i = 0; i < 7; i++) {
    if (shuffled[i].u != items[i].u || shuffled[i].s_len != items[i].s_len ||
        memcmp(shuffled[i].s, items[i].s, items[i].s_len) != 0) {
      fprintf(stderr, "mixed sort wrong at %d\n", i);
      return 1;
    }
  }

  // Round-trip
  for (int i = 0; i < 7; i++) {
    compact_state_t d = {0, items[i].len, items[i].buf};
    uint64_t u;
    assert(lexkey_decode_uint(&d, &u) == 0);
    utf8_string_t s;
    utf8_string_init(&s);
    assert(lexkey_decode_string(&d, &s) == 0);
    assert(d.start == items[i].len);
    assert(u == items[i].u);
    assert(s.len == items[i].s_len);
    assert(memcmp(s.data, items[i].s, s.len) == 0);
    utf8_string_destroy(&s);
  }

  // Preencode then encode pattern (the canonical compact-style usage).
  {
    utf8_string_view_t hello = utf8_string_view_init((const utf8_t *) "hello", 5);
    compact_state_t s = {0, 0, NULL};
    lexkey_preencode_uint(&s, 42);
    lexkey_preencode_string(&s, hello);
    uint8_t *buf = (uint8_t *) malloc(s.end);
    assert(buf != NULL);
    s.buffer = buf;
    lexkey_encode_uint(&s, 42);
    lexkey_encode_string(&s, hello);

    compact_state_t d = {0, s.start, buf};
    uint64_t u;
    utf8_string_t str;
    utf8_string_init(&str);
    assert(lexkey_decode_uint(&d, &u) == 0);
    assert(lexkey_decode_string(&d, &str) == 0);
    assert(u == 42);
    assert(str.len == 5);
    assert(memcmp(str.data, "hello", 5) == 0);
    utf8_string_destroy(&str);
    free(buf);
  }

  return 0;
}
