#pragma once

typedef enum {
    TOKEN_ERROR,
    TOKEN_UNKNOWN,

    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
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

    TOKEN_PRINT_STMT,
    TOKEN_END_STMT,
    TOKEN_EOF,
} TokenType;


typedef struct {
    int row;
    int col;
    int index;
} Location;


typedef struct {
    TokenType type;
    Location location;
    int count;
} Token;

Location token_location(Token token);
Token token_make_empty();
Token token_make(TokenType type, Location location, int count);
