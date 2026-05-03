#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexkey.h"

#define N 65536

typedef struct {
  int i;
  uint8_t buf[8];
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

int
main () {
  item_t *items = (item_t *) malloc(sizeof(item_t) * N);
  assert(items != NULL);

  for (int i = 0; i < N; i++) {
    uint8_t in[2] = {(uint8_t) (i >> 8), (uint8_t) (i & 0xff)};
    compact_state_t state = {0, sizeof(items[i].buf), items[i].buf};
    int err = lexkey_encode_buffer(&state, in, 2);
    assert(err == 0);
    items[i].i = i;
    items[i].len = state.start;
  }

  qsort(items, N, sizeof(item_t), cmp);

  for (int i = 0; i < N; i++) {
    if (items[i].i != i) {
      fprintf(stderr, "sorted wrong at %d (got %d)\n", i, items[i].i);
      free(items);
      return 1;
    }
  }

  free(items);
  return 0;
}
