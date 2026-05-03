#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexkey.h"

typedef struct {
  int i;
  uint8_t buf[16];
  size_t len;
} item_t;

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

static void
encode (item_t *it, int i, const uint8_t *in, size_t in_len) {
  compact_state_t state = {0, sizeof(it->buf), it->buf};
  int err = lexkey_encode_buffer(&state, in, in_len);
  assert(err == 0);
  it->i = i;
  it->len = state.start;
}

int
main () {
  item_t items[9];

  encode(&items[0], 0, (uint8_t[]) {0}, 1);
  encode(&items[1], 1, (uint8_t[]) {0, 0}, 2);
  encode(&items[2], 2, (uint8_t[]) {0, 0, 0}, 3);
  encode(&items[3], 3, (uint8_t[]) {0, 1}, 2);
  encode(&items[4], 4, (uint8_t[]) {1}, 1);
  encode(&items[5], 5, (uint8_t[]) {1, 0}, 2);
  encode(&items[6], 6, (uint8_t[]) {1, 0, 0}, 3);
  encode(&items[7], 7, (uint8_t[]) {1, 1}, 2);
  encode(&items[8], 8, (uint8_t[]) {2}, 1);

  qsort(items, 9, sizeof(item_t), cmp);

  for (int i = 0; i < 9; i++) {
    if (items[i].i != i) {
      fprintf(stderr, "sorted wrong at %d (got %d)\n", i, items[i].i);
      return 1;
    }
  }

  uint8_t out[16];
  size_t out_len;
  for (int i = 0; i < 9; i++) {
    compact_state_t d = {0, items[i].len, items[i].buf};
    int err = lexkey_decode_buffer(&d, out, &out_len);
    assert(err == 0);
    assert(d.start == items[i].len);
  }

  return 0;
}
