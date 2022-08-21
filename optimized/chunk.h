#pragma once
#include "preamble.h"
#include "value.h"
#include "location.h"
#include "array.h"

declare_dynarray(u8)
declare_dynarray(Value)
declare_dynarray(Location)


#define CHUNK_CONSTANTS_MAX 1024

/* A runtime object. */
typedef struct {
    /* Literals, objects, and run-time identifier strings */
    DynArray_Value    constants;
    DynArray_u8       code;
    DynArray_Location locations;
} Chunk;

Chunk chunk_make();
void chunk_write(Chunk* chunk, u8 byte, Location location);
u8   chunk_peek(Chunk* chunk);
void chunk_free(Chunk* chunk);
int  chunk_add_constant(Chunk* chunk, Value constant);
void chunk_disassemble(Chunk* chunk, const char* name);
int  chunk_instruction_disassemble(Chunk* chunk, int offset);

Location chunk_line(Chunk* chunk, int offset);
