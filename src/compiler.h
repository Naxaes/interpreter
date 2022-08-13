#pragma once
#include "parser.h"
#include "chunk.h"
#include "value.h"
#include "object.h"
#include "error.h"

#define COMPILER_MAX_ERRORS 32



typedef struct {
    Token name;
    int   depth;
} Local;


typedef struct {
    Parser* parser;

    Error errors[COMPILER_MAX_ERRORS];
    int error_count;

    ObjFunction* function;
    FunctionType type;

    Local locals[256];
    int   local_count;
    int   scope_depth;

    Value  constants[1024];
    int    const_ptr;

    bool   had_error;
    bool   in_panic_mode;
} Compiler;


ObjFunction* compile(const char* path, const char* source);
