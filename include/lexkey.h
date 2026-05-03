#ifndef LEXKEY_H
#define LEXKEY_H

#include <compact.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// All encoders share libcompact's compact_state_t. Each type exposes the
// triplet:
//
//   lexkey_preencode_<type>(state, ...)  -- advances state->end by needed bytes
//   lexkey_encode_<type>(state, ...)     -- writes at state->buffer + state->start
//   lexkey_decode_<type>(state, ...)     -- reads at state->buffer + state->start
//
// All return 0 on success and a negative value on error. decode functions
// accept a NULL result pointer to skip the value.

// === BUFFER ===
//
// Wire format: 0x00 <body, with each 0x00 escaped as 0x00 0x02> 0x00 0x01.

static inline int
lexkey_preencode_buffer (compact_state_t *state, const uint8_t *buf, size_t len) {
  size_t extra = 3;
  for (size_t i = 0; i < len; i++) {
    if (buf[i] == 0x00) extra++;
  }
  state->end += len + extra;
  return 0;
}

static inline int
lexkey_encode_buffer (compact_state_t *state, const uint8_t *buf, size_t len) {
  state->buffer[state->start++] = 0x00;
  for (size_t i = 0; i < len; i++) {
    uint8_t b = buf[i];
    state->buffer[state->start++] = b;
    if (b == 0x00) state->buffer[state->start++] = 0x02;
  }
  state->buffer[state->start++] = 0x00;
  state->buffer[state->start++] = 0x01;
  return 0;
}

// Decodes into result (which must have capacity >= the encoded body length,
// at most state->end - state->start). Sets *result_len. If result is NULL,
// the value is consumed but not written.
static inline int
lexkey_decode_buffer (compact_state_t *state, uint8_t *result, size_t result_cap, size_t *result_len) {
  if (state->start >= state->end) return -1;
  if (state->buffer[state->start++] != 0x00) return -1;

  size_t n = 0;
  while (state->start < state->end) {
    uint8_t b = state->buffer[state->start++];
    if (b != 0x00) {
      if (result) {
        if (n >= result_cap) return -1;
        result[n] = b;
      }
      n++;
      continue;
    }
    if (state->start >= state->end) return -1;
    uint8_t next = state->buffer[state->start++];
    if (next == 0x01) {
      if (result_len) *result_len = n;
      return 0;
    }
    if (next == 0x02) {
      if (result) {
        if (n >= result_cap) return -1;
        result[n] = 0x00;
      }
      n++;
      continue;
    }
    return -1;
  }
  return -1;
}

// === STRING ===
//
// Identical wire format to BUFFER; convention is UTF-8.

static inline int
lexkey_preencode_string (compact_state_t *state, const char *str, size_t len) {
  return lexkey_preencode_buffer(state, (const uint8_t *) str, len);
}

static inline int
lexkey_encode_string (compact_state_t *state, const char *str, size_t len) {
  return lexkey_encode_buffer(state, (const uint8_t *) str, len);
}

static inline int
lexkey_decode_string (compact_state_t *state, char *result, size_t result_cap, size_t *result_len) {
  return lexkey_decode_buffer(state, (uint8_t *) result, result_cap, result_len);
}

// === UINT ===
//
// Wire format (lex-sortable, big-endian):
//   0x00..0xfb           : 1 byte, value as-is
//   0xfc + 2 BE bytes    : value in [0xfc..0xffff]
//   0xfd + 4 BE bytes    : value in [0x10000..0xffffffff]
//   0xfe + 8 BE bytes    : value in [0x100000000..UINT64_MAX]
//   0xff                 : max sentinel (Infinity in JS), use *_max helpers

static inline int
lexkey_preencode_uint (compact_state_t *state, uint64_t n) {
  if (n <= 0xfb) state->end += 1;
  else if (n <= 0xffff) state->end += 3;
  else if (n <= 0xffffffffULL) state->end += 5;
  else state->end += 9;
  return 0;
}

static inline int
lexkey_encode_uint (compact_state_t *state, uint64_t n) {
  if (n <= 0xfb) {
    state->buffer[state->start++] = (uint8_t) n;
    return 0;
  }
  if (n <= 0xffff) {
    state->buffer[state->start++] = 0xfc;
    state->buffer[state->start++] = (uint8_t) (n >> 8);
    state->buffer[state->start++] = (uint8_t) n;
    return 0;
  }
  if (n <= 0xffffffffULL) {
    state->buffer[state->start++] = 0xfd;
    state->buffer[state->start++] = (uint8_t) (n >> 24);
    state->buffer[state->start++] = (uint8_t) (n >> 16);
    state->buffer[state->start++] = (uint8_t) (n >> 8);
    state->buffer[state->start++] = (uint8_t) n;
    return 0;
  }
  state->buffer[state->start++] = 0xfe;
  state->buffer[state->start++] = (uint8_t) (n >> 56);
  state->buffer[state->start++] = (uint8_t) (n >> 48);
  state->buffer[state->start++] = (uint8_t) (n >> 40);
  state->buffer[state->start++] = (uint8_t) (n >> 32);
  state->buffer[state->start++] = (uint8_t) (n >> 24);
  state->buffer[state->start++] = (uint8_t) (n >> 16);
  state->buffer[state->start++] = (uint8_t) (n >> 8);
  state->buffer[state->start++] = (uint8_t) n;
  return 0;
}

// Returns 0 on success (writes to *result), 1 if the encoded value is the
// max sentinel (*result is left untouched), or a negative value on error.
static inline int
lexkey_decode_uint (compact_state_t *state, uint64_t *result) {
  if (state->start >= state->end) return -1;
  uint8_t a = state->buffer[state->start++];
  if (a <= 0xfb) {
    if (result) *result = a;
    return 0;
  }
  if (a == 0xfc) {
    if (state->end - state->start < 2) return -1;
    if (result) *result = ((uint64_t) state->buffer[state->start] << 8) | (uint64_t) state->buffer[state->start + 1];
    state->start += 2;
    return 0;
  }
  if (a == 0xfd) {
    if (state->end - state->start < 4) return -1;
    if (result) *result = ((uint64_t) state->buffer[state->start] << 24) | ((uint64_t) state->buffer[state->start + 1] << 16) | ((uint64_t) state->buffer[state->start + 2] << 8) | (uint64_t) state->buffer[state->start + 3];
    state->start += 4;
    return 0;
  }
  if (a == 0xfe) {
    if (state->end - state->start < 8) return -1;
    if (result) *result = ((uint64_t) state->buffer[state->start] << 56) | ((uint64_t) state->buffer[state->start + 1] << 48) | ((uint64_t) state->buffer[state->start + 2] << 40) | ((uint64_t) state->buffer[state->start + 3] << 32) | ((uint64_t) state->buffer[state->start + 4] << 24) | ((uint64_t) state->buffer[state->start + 5] << 16) | ((uint64_t) state->buffer[state->start + 6] << 8) | (uint64_t) state->buffer[state->start + 7];
    state->start += 8;
    return 0;
  }
  return 1;
}

// Max sentinel for uint (sorts after every finite uint).

static inline int
lexkey_preencode_uint_max (compact_state_t *state) {
  state->end += 1;
  return 0;
}

static inline int
lexkey_encode_uint_max (compact_state_t *state) {
  state->buffer[state->start++] = 0xff;
  return 0;
}

// === INT ===
//
// Wire format (lex-sortable, big-endian, asymmetric so that all negative
// numbers sort before all non-negative numbers):
//   0x00                 : min sentinel (-Infinity in JS), use *_min helpers
//   0x01 + 8 BE bytes    : value in [-UINT64_MAX..-(0xffffffff + 1)] using
//                          one's-complement-style encoding of the magnitude
//   0x02 + 4 BE bytes    : value in [-0xffffffff..-0x10000]
//   0x03 + 2 BE bytes    : value in [-0xffff..-0x100]
//   0x04 + 1 byte        : value in [-0xff..-1]
//   n + 0x05             : value in [0..0xf6] (single byte 0x05..0xfb)
//   0xfc + 2 BE bytes    : value in [0xf7..0xffff]
//   0xfd + 4 BE bytes    : value in [0x10000..0xffffffff]
//   0xfe + 8 BE bytes    : value in [0x100000000..INT64_MAX]
//   0xff                 : max sentinel (+Infinity in JS), use *_max helpers

static inline int
lexkey_preencode_int (compact_state_t *state, int64_t n) {
  if (n < 0) {
    if (n >= -(int64_t) 0xff) state->end += 2;
    else if (n >= -(int64_t) 0xffff) state->end += 3;
    else if (n >= -(int64_t) 0xffffffffLL) state->end += 5;
    else state->end += 9;
    return 0;
  }
  if (n <= 0xf6) state->end += 1;
  else if (n <= 0xffff) state->end += 3;
  else if (n <= 0xffffffffLL) state->end += 5;
  else state->end += 9;
  return 0;
}

static inline int
lexkey_encode_int (compact_state_t *state, int64_t n) {
  if (n < 0) {
    // Compute absolute magnitude as uint64_t, safe for INT64_MIN.
    uint64_t mag = (uint64_t) (-(n + 1)) + 1;

    if (mag > 0xffffffffULL) {
      state->buffer[state->start++] = 0x01;
      uint64_t r = mag >> 32;
      uint64_t rem = mag & 0xffffffffULL;
      uint32_t hi = (uint32_t) (0xffffffffULL - r);
      uint32_t lo = (uint32_t) (0xffffffffULL - rem);
      state->buffer[state->start++] = (uint8_t) (hi >> 24);
      state->buffer[state->start++] = (uint8_t) (hi >> 16);
      state->buffer[state->start++] = (uint8_t) (hi >> 8);
      state->buffer[state->start++] = (uint8_t) hi;
      state->buffer[state->start++] = (uint8_t) (lo >> 24);
      state->buffer[state->start++] = (uint8_t) (lo >> 16);
      state->buffer[state->start++] = (uint8_t) (lo >> 8);
      state->buffer[state->start++] = (uint8_t) lo;
      return 0;
    }
    if (mag > 0xffff) {
      state->buffer[state->start++] = 0x02;
      uint32_t v = (uint32_t) (0xffffffffULL - mag);
      state->buffer[state->start++] = (uint8_t) (v >> 24);
      state->buffer[state->start++] = (uint8_t) (v >> 16);
      state->buffer[state->start++] = (uint8_t) (v >> 8);
      state->buffer[state->start++] = (uint8_t) v;
      return 0;
    }
    if (mag > 0xff) {
      state->buffer[state->start++] = 0x03;
      uint16_t v = (uint16_t) (0xffff - mag);
      state->buffer[state->start++] = (uint8_t) (v >> 8);
      state->buffer[state->start++] = (uint8_t) v;
      return 0;
    }
    state->buffer[state->start++] = 0x04;
    state->buffer[state->start++] = (uint8_t) (0xff - mag);
    return 0;
  }

  if (n <= 0xf6) {
    state->buffer[state->start++] = (uint8_t) (n + 0x05);
    return 0;
  }
  if (n <= 0xffff) {
    state->buffer[state->start++] = 0xfc;
    state->buffer[state->start++] = (uint8_t) (n >> 8);
    state->buffer[state->start++] = (uint8_t) n;
    return 0;
  }
  if (n <= 0xffffffffLL) {
    state->buffer[state->start++] = 0xfd;
    state->buffer[state->start++] = (uint8_t) (n >> 24);
    state->buffer[state->start++] = (uint8_t) (n >> 16);
    state->buffer[state->start++] = (uint8_t) (n >> 8);
    state->buffer[state->start++] = (uint8_t) n;
    return 0;
  }
  state->buffer[state->start++] = 0xfe;
  state->buffer[state->start++] = (uint8_t) (n >> 56);
  state->buffer[state->start++] = (uint8_t) (n >> 48);
  state->buffer[state->start++] = (uint8_t) (n >> 40);
  state->buffer[state->start++] = (uint8_t) (n >> 32);
  state->buffer[state->start++] = (uint8_t) (n >> 24);
  state->buffer[state->start++] = (uint8_t) (n >> 16);
  state->buffer[state->start++] = (uint8_t) (n >> 8);
  state->buffer[state->start++] = (uint8_t) n;
  return 0;
}

// Returns 0 on success (writes to *result), 1 if max sentinel, 2 if min
// sentinel, or a negative value on error.
static inline int
lexkey_decode_int (compact_state_t *state, int64_t *result) {
  if (state->start >= state->end) return -1;
  uint8_t a = state->buffer[state->start++];

  if (a == 0x00) return 2;
  if (a == 0xff) return 1;

  if (a == 0x01) {
    if (state->end - state->start < 8) return -1;
    uint32_t hi = ((uint32_t) state->buffer[state->start] << 24) | ((uint32_t) state->buffer[state->start + 1] << 16) | ((uint32_t) state->buffer[state->start + 2] << 8) | (uint32_t) state->buffer[state->start + 3];
    uint32_t lo = ((uint32_t) state->buffer[state->start + 4] << 24) | ((uint32_t) state->buffer[state->start + 5] << 16) | ((uint32_t) state->buffer[state->start + 6] << 8) | (uint32_t) state->buffer[state->start + 7];
    state->start += 8;
    uint64_t r = 0xffffffffULL - (uint64_t) hi;
    uint64_t rem = 0xffffffffULL - (uint64_t) lo;
    uint64_t mag = (r << 32) | rem;
    if (mag > (uint64_t) INT64_MAX + 1) return -1;
    if (result) {
      if (mag == (uint64_t) INT64_MAX + 1) *result = INT64_MIN;
      else *result = -(int64_t) mag;
    }
    return 0;
  }

  if (a == 0x02) {
    if (state->end - state->start < 4) return -1;
    uint32_t v = ((uint32_t) state->buffer[state->start] << 24) | ((uint32_t) state->buffer[state->start + 1] << 16) | ((uint32_t) state->buffer[state->start + 2] << 8) | (uint32_t) state->buffer[state->start + 3];
    state->start += 4;
    if (result) *result = (int64_t) v - (int64_t) 0xffffffffLL;
    return 0;
  }

  if (a == 0x03) {
    if (state->end - state->start < 2) return -1;
    uint16_t v = (uint16_t) (((uint16_t) state->buffer[state->start] << 8) | (uint16_t) state->buffer[state->start + 1]);
    state->start += 2;
    if (result) *result = (int64_t) v - 0xffff;
    return 0;
  }

  if (a == 0x04) {
    if (state->start >= state->end) return -1;
    uint8_t v = state->buffer[state->start++];
    if (result) *result = (int64_t) v - 0xff;
    return 0;
  }

  if (a <= 0xfb) {
    if (result) *result = (int64_t) a - 0x05;
    return 0;
  }

  if (a == 0xfc) {
    if (state->end - state->start < 2) return -1;
    uint16_t v = (uint16_t) (((uint16_t) state->buffer[state->start] << 8) | (uint16_t) state->buffer[state->start + 1]);
    state->start += 2;
    if (result) *result = v;
    return 0;
  }

  if (a == 0xfd) {
    if (state->end - state->start < 4) return -1;
    uint32_t v = ((uint32_t) state->buffer[state->start] << 24) | ((uint32_t) state->buffer[state->start + 1] << 16) | ((uint32_t) state->buffer[state->start + 2] << 8) | (uint32_t) state->buffer[state->start + 3];
    state->start += 4;
    if (result) *result = v;
    return 0;
  }

  // a == 0xfe
  if (state->end - state->start < 8) return -1;
  uint64_t hi = ((uint64_t) state->buffer[state->start] << 24) | ((uint64_t) state->buffer[state->start + 1] << 16) | ((uint64_t) state->buffer[state->start + 2] << 8) | (uint64_t) state->buffer[state->start + 3];
  uint64_t lo = ((uint64_t) state->buffer[state->start + 4] << 24) | ((uint64_t) state->buffer[state->start + 5] << 16) | ((uint64_t) state->buffer[state->start + 6] << 8) | (uint64_t) state->buffer[state->start + 7];
  state->start += 8;
  uint64_t v = (hi << 32) | lo;
  if (v > (uint64_t) INT64_MAX) return -1;
  if (result) *result = (int64_t) v;
  return 0;
}

// Min/max sentinels for int.

static inline int
lexkey_preencode_int_min (compact_state_t *state) {
  state->end += 1;
  return 0;
}

static inline int
lexkey_encode_int_min (compact_state_t *state) {
  state->buffer[state->start++] = 0x00;
  return 0;
}

static inline int
lexkey_preencode_int_max (compact_state_t *state) {
  state->end += 1;
  return 0;
}

static inline int
lexkey_encode_int_max (compact_state_t *state) {
  state->buffer[state->start++] = 0xff;
  return 0;
}

// === DATE ===
//
// Encoded as INT of milliseconds since the Unix epoch.

static inline int
lexkey_preencode_date (compact_state_t *state, int64_t ms) {
  return lexkey_preencode_int(state, ms);
}

static inline int
lexkey_encode_date (compact_state_t *state, int64_t ms) {
  return lexkey_encode_int(state, ms);
}

static inline int
lexkey_decode_date (compact_state_t *state, int64_t *result) {
  return lexkey_decode_int(state, result);
}

// === BOOL ===
//
// Encoded as UINT 0 or 1.

static inline int
lexkey_preencode_bool (compact_state_t *state, bool value) {
  (void) value;
  state->end += 1;
  return 0;
}

static inline int
lexkey_encode_bool (compact_state_t *state, bool value) {
  return lexkey_encode_uint(state, value ? 1 : 0);
}

static inline int
lexkey_decode_bool (compact_state_t *state, bool *result) {
  uint64_t n;
  int err = lexkey_decode_uint(state, &n);
  if (err != 0) return err < 0 ? err : -1;
  if (result) *result = n != 0;
  return 0;
}

// === Range terminator ===
//
// Append a 0xff byte to a partial key to form an inclusive upper bound that
// sorts after any extension of that key. Used when building gt/lte ranges
// over a subset of the encoder's fields.

static inline int
lexkey_preencode_terminate (compact_state_t *state) {
  state->end += 1;
  return 0;
}

static inline int
lexkey_encode_terminate (compact_state_t *state) {
  state->buffer[state->start++] = 0xff;
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif // LEXKEY_H
