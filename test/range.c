// Verify the terminator helper: appending lexkey_encode_terminate to a partial
// key produces an inclusive upper bound that sorts after any extension of
// that key but before any longer key starting with a strictly greater value.

#include <assert.h>
#include <compact.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../include/lexkey.h"

static int
cmp_buf (const uint8_t *a, size_t a_len, const uint8_t *b, size_t b_len) {
  size_t m = a_len < b_len ? a_len : b_len;
  int r = memcmp(a, b, m);
  if (r != 0) return r;
  if (a_len < b_len) return -1;
  if (a_len > b_len) return 1;
  return 0;
}

int
main () {
  uint8_t k1a[16], k1b[16], k2aa[16], k2bb[16], k3aaa[16], k3bbb[16];
  size_t k1a_len, k1b_len, k2aa_len, k2bb_len, k3aaa_len, k3bbb_len;

  #define ENC(buf, buf_len, u, s) do { \
    compact_state_t st = {0, sizeof(buf), (buf)}; \
    lexkey_encode_uint(&st, (u)); \
    lexkey_encode_string(&st, (s), strlen(s)); \
    (buf_len) = st.start; \
  } while (0)

  ENC(k1a, k1a_len, 1, "a");
  ENC(k1b, k1b_len, 1, "b");
  ENC(k2aa, k2aa_len, 2, "aa");
  ENC(k2bb, k2bb_len, 2, "bb");
  ENC(k3aaa, k3aaa_len, 3, "aaa");
  ENC(k3bbb, k3bbb_len, 3, "bbb");

  // gt = encode([1]) + terminate, lt = encode([3])
  uint8_t gt[8];
  size_t gt_len;
  {
    compact_state_t st = {0, sizeof(gt), gt};
    lexkey_encode_uint(&st, 1);
    lexkey_encode_terminate(&st);
    gt_len = st.start;
  }

  uint8_t lt[8];
  size_t lt_len;
  {
    compact_state_t st = {0, sizeof(lt), lt};
    lexkey_encode_uint(&st, 3);
    lt_len = st.start;
  }

  assert(cmp_buf(k1a, k1a_len, gt, gt_len) <= 0);
  assert(cmp_buf(k1b, k1b_len, gt, gt_len) <= 0);
  assert(cmp_buf(k2aa, k2aa_len, gt, gt_len) > 0);
  assert(cmp_buf(k2bb, k2bb_len, gt, gt_len) > 0);
  assert(cmp_buf(k2aa, k2aa_len, lt, lt_len) < 0);
  assert(cmp_buf(k2bb, k2bb_len, lt, lt_len) < 0);
  assert(cmp_buf(k3aaa, k3aaa_len, lt, lt_len) >= 0);
  assert(cmp_buf(k3bbb, k3bbb_len, lt, lt_len) >= 0);

  return 0;
}
