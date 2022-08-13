#pragma once

#include "token.h"
#include "slice.h"
#include "error.h"

#include <stdbool.h>


typedef struct {
    const char* path;
    const char* source;
    Location location;
    Token previous;
    Token current;
    Error errors[32];
    int   error_count;
    bool  in_panic_mode;
} Parser;

Parser parser_make(const char* path, const char* source);
void   advance(Parser* self);
bool   expect(Parser* self, TokenType type);

bool parser_had_error(const Parser* self);
void parser_flush_errors(Parser* self);

Slice parser_token_repr(Parser* self, Token token);

