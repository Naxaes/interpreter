#include "preamble.h"


#ifndef STRING_BOUNDS_CHECK
#define STRING_BOUNDS_CHECK
#endif

// NOTE(ted): This is undefined behaviour:
//      string.data[0] << 0  |
//      string.data[1] << 8  |
//      string.data[2] << 16 |
//      string.data[3] << 24;
// because char might be signed and can't be shifted
// left 24 times.
static inline u8 to_u8(utf8 x) { return (u8) x; }
#define PACK_CHAR_ARRAY_TO_RUNE_2(x) (rune) ((to_u8((x)[0]) << 0u) | (to_u8((x)[1]) << 8u))
#define PACK_CHAR_ARRAY_TO_RUNE_3(x) (rune) ((to_u8((x)[0]) << 0u) | (to_u8((x)[1]) << 8u) | (to_u8((x)[2]) << 16u))
#define PACK_CHAR_ARRAY_TO_RUNE_4(x) (rune) ((to_u8((x)[0]) << 0u) | (to_u8((x)[1]) << 8u) | (to_u8((x)[2]) << 16u) | (to_u8((x)[3]) << 24u))
#define RUNE_AS_CHAR_ARRAY(x) (utf8[]) { (utf8)((x) >> 0), (utf8)((x) >> 8), (utf8)((x) >> 16), (utf8)((x) >> 24), 0 }

rune char_array_to_rune(char* bytes);
bool is_continuation_byte(utf8 byte);
bool is_valid_start_byte(utf8 byte);
int  multi_byte_count(utf8 byte);
bool rune_matches(const utf8* a, rune b);
bool matches(rune a, rune b, rune x, rune y);
bool is_ascii_alpha(rune c);
bool is_ascii_digit(rune a);
bool is_single_ascii_token(rune c);
bool are_continuations(const utf8* chars, int count);


bool is_whitespace(rune c);
bool is_space(char c);
bool is_digit(char c);
bool is_alpha(char c);
