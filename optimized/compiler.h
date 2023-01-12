#pragma once
#include "c-preamble/nax_preamble.h"
#include "parser.h"
#include "array.h"
#include "opcodes.h"
#include "chunk.h"

declare_dynarray(OpCode)

#define COMPILER_MAX_ERRORS


typedef struct {
    Ast* next;
    Type type;
} CompilerReturn;


typedef struct {
    Token name;
    int   depth;
} Variable;

declare_dynarray(Variable)
declare_dynarray(Chunk)

typedef struct {
    Parser* parser;

    DynArray_Chunk chunks;
    int current_chunk;

    Location current_location;

//    Error errors[COMPILER_MAX_ERRORS];
//    int error_count;

    int scope_depth;

    /* Compile-time storage for identifying for variables,
     * as they are emitted in code at runtime.
     *
     *
     * */
    DynArray_Variable variables;

    bool had_error;
    bool in_panic_mode;
} Compiler;

Compiler compiler_make(Parser* parser);
void compile(Compiler* compiler, Ast_Module * node);
