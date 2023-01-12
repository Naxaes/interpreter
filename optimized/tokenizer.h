#pragma once

#include "c-preamble/nax_preamble.h"
#include "slice.h"
#include "array.h"
#include "memory.h"
#include "location.h"


typedef enum {
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,

    TOKEN_MINUS,
    TOKEN_PLUS,

    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_PERCENT,

    TOKEN_I64,
    TOKEN_LITERAL_BEGIN = TOKEN_I64,
    TOKEN_U64,
    TOKEN_F64,
    TOKEN_STRING,
    TOKEN_LITERAL_END = TOKEN_STRING,

    TOKEN_FALSE,
    TOKEN_TRUE,

    TOKEN_EQUAL,
    TOKEN_BANG,

    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    TOKEN_AND,
    TOKEN_OR,

    TOKEN_IDENTIFIER,

    TOKEN_VAR,
    TOKEN_FUN,

    TOKEN_IF,
    TOKEN_ELSE,

    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,

    TOKEN_COMMA,
    TOKEN_DOT,

    TOKEN_END_STMT,
    TOKEN_EOF,
    TOKEN_COUNT,
} TokenType;
static_assert(TOKEN_COUNT < 256, "invariant");


typedef struct {
    TokenType type : 8;
    u64 offset : 24;
    u64 row    : 20;
    u64 column : 12;
} Token;
static_assert(sizeof(Token) == sizeof(Location), "Token and Location must be the same");

Location token_location(Token token);

typedef enum {
    PrefixNone,
    PrefixHex,
    PrefixOct,
    PrefixBin
} LiteralPrefix;

typedef enum {
    PostfixNone,
    PostfixF64,
    PostfixI64,
    PostfixU64,
} LiteralPostfix;


Slice token_view(const char* source, Token token);

declare_array(Token)
Array_Token tokenize(const char* source, const char* path, StackAllocator* allocator);


