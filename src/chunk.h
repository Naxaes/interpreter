#pragma once
#include "value.h"

#define CHUNK_CONSTANTS_MAX 1024


typedef struct {
    int row;
    int index;
} Location;


typedef struct {
    Value constants[CHUNK_CONSTANTS_MAX];
    int constant_count;

    const char* source;
    Location*   lines;

    u8* code;
    int count;
    int capacity;
} Chunk;


Chunk chunk_make(const char* source);
void  chunk_write(Chunk* chunk, u8 byte, Location location);
void  chunk_free(Chunk* chunk);
int   chunk_add_constant(Chunk* chunk, Value constant);
void  chunk_disassemble(Chunk* chunk, const char* name);
int   chunk_instruction_disassemble(Chunk* chunk, int offset);
