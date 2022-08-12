#pragma once

#include "tokens.h"
#include <stdbool.h>


typedef struct {
    const char* source;
    int   index;
    int   row;
    int   col;
    Token prev;
    Token curr;
    bool  had_error;
    bool  in_panic_mode;
} Parser;

Parser parser_make(const char* source);
void   parser_error(Parser* self, const char* message);
void   advance(Parser* self);
bool   expect(Parser* self, TokenType type);
