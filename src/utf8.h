#include "preamble.h"
#include "error.h"

/*
Useful resources:
* https://www.rfc-editor.org/rfc/rfc3629
* https://www.utf8-chartable.de/
* https://stackoverflow.com/questions/66715611/check-for-valid-utf-8-encoding-in-c/66723102#66723102

UTF-8 is a multi-byte encoding. The first byte of a character tells the
character's size in bytes.
    * 0b0xxxxxxx - start of one byte character.
    * 0b10xxxxxx - not a valid start byte! See continuation byte below.
    * 0b110xxxxx - start of two byte character.
    * 0b1110xxxx - start of three byte character.
    * 0b11110xxx - start of four byte character.
A character can have max 4 bytes.

If a character contain multiple bytes, then the following bytes of that
character must start with 0b10xxxxxx. This is called a continuation byte.

Each character is assigned an id, which is just an incremented digit called
code point. Due to the first bits being reserved for meta data, it'll not map
exactly to the hex representation (except for the first 127, ASCII, characters).
The first character (NULL character) is 0b00000000 and has code point U+0000. The
last ASCII character (DELETE character) is 0b01111111 has code point U+007F.
Thereafter comes U+0080 (Padding Character) as 0b11000010 0b10000000 (0xC280).

Due to the meta data bits, there might be several ways to represent the same code
point (this is only a problem between single and double byte code points).
    * 0b0000_0000 <=> 0b1100_0000 0b1000_0000 (xxx0_0000 xx00_0000) <=> U+0000
    * 0b0100_0000 <=> 0b1000_0001 0b1000_0000 (xxx0_0001 xx00_0000 <=> 000_0100_0000) <=> U+0040
The shortest encoding is the valid one.

TODO(ted): Verify this!
Due to a character having max 4 bytes, the largest code point is U+10FFFF, as the
information actually being stored (disregarding the meta data bits) is, for a four
byte number, 2^3 * (2^6) * (2^6) * (2^6).

Another thing to consider is that a character can also be multi code point. For
example, è can be a single code point or e + ` combined. If Unicode is used as
identifiers, equivalent characters should be normalized to some standard form.
*/

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
bool is_whitespace(rune c);
bool matches(rune a, rune b, rune x, rune y);
bool is_ascii_alpha(rune c);
bool is_ascii_digit(rune a);
bool is_single_ascii_token(rune c);
static inline bool are_continuations(const utf8* chars, int count) ;
