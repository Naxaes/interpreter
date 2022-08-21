#include "utf8.h"

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


bool are_continuations(const utf8* chars, int count) {
    for (int i = 0; i < count; ++i)
        if (!is_continuation_byte(chars[i]))
            return false;
    return true;
}



/* ---------------- IMPLEMENTATION ----------------
`char` is annoying... It's implicit in both size and sign according to the spec,
and it's easy to make bugs with bitwise operators as they implicitly cast to int.
For example, this is a bug: `(char & 0b10111111) == char`. Because the left-hand
side will be converted to int and hence always positive, while the right-hand
side can still be negative. So always cast to unsigned first...
*/

// TODO(ted): Doesn't check if the following bytes are continuation bytes.
rune char_array_to_rune(char* bytes) {
    int count = multi_byte_count(*bytes);
    if (count == 1) return (rune) *bytes;
    if (count == 2) return PACK_CHAR_ARRAY_TO_RUNE_2(bytes);
    if (count == 3) return PACK_CHAR_ARRAY_TO_RUNE_3(bytes);
    if (count == 4) return PACK_CHAR_ARRAY_TO_RUNE_4(bytes);

    PANIC("INVALID PATH");
}


bool is_continuation_byte(utf8 byte) {
    return 0b10000000u <= to_u8(byte) && to_u8(byte) <= 0b10111111u;
}


bool is_valid_start_byte(utf8 byte) {
    u8 bits = to_u8(byte);
    if ((bits & 0b01111111u) == bits) return true;
    if ((bits & 0b11011111u) == bits) return true;
    if ((bits & 0b11101111u) == bits) return true;
    if ((bits & 0b11110111u) == bits) return true;
    return false;
}


int multi_byte_count(utf8 byte) {
    u8 bits = to_u8(byte);
    if ((bits & 0b01111111u) == bits) return 1;
    if ((bits & 0b10111111u) == bits) PANIC("Byte is continuation byte.");
    if ((bits & 0b11011111u) == bits) return 2;
    if ((bits & 0b11101111u) == bits) return 3;
    if ((bits & 0b11110111u) == bits) return 4;

    PANIC("INVALID PATH");
}


bool is_whitespace(rune c) {  // TODO(ted): Are there more?
    return c == '\n' ||
           c == '\t' ||
           c == ' ';
}

bool is_space(char c) { return (c == ' ' || c == '\t'); }
bool is_digit(char c) { return ('0' <= c && c <= '9');  }
bool is_alpha(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

bool matches(rune a, rune b, rune x, rune y) {
    return a == x && b == y;
}

bool is_ascii_alpha(rune c) {  // TODO(ted): FIX UNICODE!
    return  ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z'); // ||
    // c >= 0xc2;
}

bool is_ascii_digit(rune a) {
    return '0' <= a && a <= '9';
}

bool is_single_ascii_token(rune c) {
    return  ('!' <= c && c <= '/') ||
            (':' <= c && c <= '@') ||
            ('[' <= c && c <= '`') ||
            ('{' <= c && c <= '~');
}
