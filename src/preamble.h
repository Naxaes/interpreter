#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* ---- TYPES ----
Concise and specific sizes, as well as some meaningful names
for certain types like Unicode and booleans.
*/
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef float     f32;
typedef double    f64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef u32       rune;   // Unicode code point.
typedef char      utf8;   // Unicode byte.
