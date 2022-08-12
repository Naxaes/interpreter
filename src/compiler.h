#pragma once
#include "parser.h"
#include "chunk.h"
#include "value.h"


typedef struct {
    Token name;
    int   depth;
} Local;


typedef struct {
    Parser parser;
    Chunk  chunk;

    Local locals[256];
    int   local_count;
    int   scope_depth;

    Value  constants[1024];
    int    const_ptr;
} Compiler;


Compiler compiler_make(Chunk chunk, Parser parser);
bool compile(Compiler* compiler);
