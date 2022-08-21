#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>


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



#ifdef DEBUG
#define D_ASSERT(x)  do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed", __FILE__, __LINE__); raise(SIGINT); }}  while(0)
#else
#define D_ASSERT(x)
#endif

#define ASSERT(x)        do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed", __FILE__, __LINE__); raise(SIGINT); }}  while(0)
#define ASSERTF(x, ...)  do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed. ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); raise(SIGINT); }}  while(0)

#define PANIC(...)  do { fprintf(stderr, "[PANIC] %s:%d:\n    ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); raise(SIGINT); exit(-1); }  while(0)

#define NOT_IMPLEMENTED  do { fprintf(stderr, "[NOT IMPLEMENTED] %s:%d:\n    ", __FILE__, __LINE__); raise(SIGINT); exit(EXIT_FAILURE); }  while(0)
#define STATIC_ASSERT(x, name) extern int static_assert_##name[(x) ? 1 : -1]

#define align_of(type) offsetof(struct { char c; type member; }, member)
