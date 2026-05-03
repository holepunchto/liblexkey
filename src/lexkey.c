#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <compact.h>
#include "../include/lexkey.h"

extern int lexkey_preencode_buffer (compact_state_t *state, const uint8_t *buf, size_t len);
extern int lexkey_encode_buffer (compact_state_t *state, const uint8_t *buf, size_t len);
extern int lexkey_decode_buffer (compact_state_t *state, uint8_t *result, size_t result_cap, size_t *result_len);

extern int lexkey_preencode_string (compact_state_t *state, const char *str, size_t len);
extern int lexkey_encode_string (compact_state_t *state, const char *str, size_t len);
extern int lexkey_decode_string (compact_state_t *state, char *result, size_t result_cap, size_t *result_len);

extern int lexkey_preencode_uint (compact_state_t *state, uint64_t n);
extern int lexkey_encode_uint (compact_state_t *state, uint64_t n);
extern int lexkey_decode_uint (compact_state_t *state, uint64_t *result);
extern int lexkey_preencode_uint_max (compact_state_t *state);
extern int lexkey_encode_uint_max (compact_state_t *state);

extern int lexkey_preencode_int (compact_state_t *state, int64_t n);
extern int lexkey_encode_int (compact_state_t *state, int64_t n);
extern int lexkey_decode_int (compact_state_t *state, int64_t *result);
extern int lexkey_preencode_int_min (compact_state_t *state);
extern int lexkey_encode_int_min (compact_state_t *state);
extern int lexkey_preencode_int_max (compact_state_t *state);
extern int lexkey_encode_int_max (compact_state_t *state);

extern int lexkey_preencode_date (compact_state_t *state, int64_t ms);
extern int lexkey_encode_date (compact_state_t *state, int64_t ms);
extern int lexkey_decode_date (compact_state_t *state, int64_t *result);

extern int lexkey_preencode_bool (compact_state_t *state, bool value);
extern int lexkey_encode_bool (compact_state_t *state, bool value);
extern int lexkey_decode_bool (compact_state_t *state, bool *result);

extern int lexkey_preencode_terminate (compact_state_t *state);
extern int lexkey_encode_terminate (compact_state_t *state);
