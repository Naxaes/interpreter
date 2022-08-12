#pragma once
#include "parser.h"
#include "chunk.h"
#include "value.h"
#include "object.h"


typedef struct {
    Token name;
    int   depth;
} Local;


typedef struct {
    Parser* parser;

    ObjFunction* function;
    FunctionType type;

    Local locals[256];
    int   local_count;
    int   scope_depth;

    Value  constants[1024];
    int    const_ptr;
} Compiler;


ObjFunction* compile(const char* source);
