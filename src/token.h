#pragma once
#include "slice.h"

typedef enum {
    TOKEN_ERROR,
    TOKEN_UNKNOWN,

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
    TOKEN_I64,
    TOKEN_F64,

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

    TOKEN_STRING,
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

    TOKEN_PRINT_STMT,
    TOKEN_END_STMT,
    TOKEN_EOF,
} TokenType;



#define ALL_TOKENS(X)                       \
    X(TOKEN_UNKNOWN, SLICE_EMPTY)           \
                                            \
    X(TOKEN_LEFT_PAREN, SLICE("("))        \
    X(TOKEN_RIGHT_PAREN, SLICE(")"))       \
    X(TOKEN_LEFT_BRACKET, SLICE("["))      \
    X(TOKEN_RIGHT_BRACKET, SLICE("]"))     \
    X(TOKEN_LEFT_BRACE, SLICE("{"))        \
    X(TOKEN_RIGHT_BRACE, SLICE("}"))       \
                                            \
    X(TOKEN_MINUS, SLICE("-"))             \
    X(TOKEN_PLUS, SLICE("+"))              \
    X(TOKEN_SLASH, SLICE("/"))             \
    X(TOKEN_STAR, SLICE("*"))              \
    X(TOKEN_I64, SLICE_EMPTY)               \
    X(TOKEN_F64, SLICE_EMPTY)               \
                                            \
    X(TOKEN_FALSE, SLICE("false"))         \
    X(TOKEN_TRUE, SLICE("true"))           \
                                            \
    X(TOKEN_EQUAL, SLICE("="))             \
    X(TOKEN_BANG, SLICE("!"))              \
                                            \
    X(TOKEN_BANG_EQUAL, SLICE("!="))       \
    X(TOKEN_EQUAL_EQUAL, SLICE("=="))      \
    X(TOKEN_GREATER, SLICE(">"))           \
    X(TOKEN_GREATER_EQUAL, SLICE(">="))    \
    X(TOKEN_LESS, SLICE("<"))              \
    X(TOKEN_LESS_EQUAL, SLICE("<="))       \
                                            \
    X(TOKEN_AND, SLICE("and"))             \
    X(TOKEN_OR, SLICE("or"))               \
                                            \
    X(TOKEN_STRING, SLICE_EMPTY)            \
    X(TOKEN_IDENTIFIER, SLICE_EMPTY)        \
                                            \
    X(TOKEN_VAR, SLICE("var"))             \
    X(TOKEN_FUN, SLICE("fun"))             \
                                            \
    X(TOKEN_IF, SLICE("if"))               \
    X(TOKEN_ELSE, SLICE("else"))           \
                                            \
    X(TOKEN_WHILE, SLICE("while"))         \
    X(TOKEN_FOR, SLICE("for"))             \
    X(TOKEN_RETURN, SLICE("return"))       \
                                            \
    X(TOKEN_COMMA, SLICE(","))             \
    X(TOKEN_DOT, SLICE("."))               \
                                            \
    X(TOKEN_PRINT_STMT, SLICE("print"))    \
    X(TOKEN_END_STMT, SLICE(";"))          \
    X(TOKEN_EOF, SLICE_EMPTY)               \


//#define X(type, name) TokenType_##type,
//typedef enum { ALL_TOKENS(X) } TokenTypes;
//#undef X
//
//#define X(type, name) [type] = (name),
//const Slice TOKEN_NAMES[] = { ALL_TOKENS(X) };
//#undef X
//
//#define X(type, name) [type] = SLICE(#type),
//const Slice TOKEN_REPR[] = { ALL_TOKENS(X) };
//#undef X



typedef struct {
    int row;
    int col;
    int index;
} Location;


typedef struct {
    TokenType type;
    Location location;
    int count;
    int cols;
} Token;

Token token_make_empty();
