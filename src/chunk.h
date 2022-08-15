#pragma once
#include "value.h"
#include "token.h"

#define CHUNK_CONSTANTS_MAX 1024

typedef struct {
    Token name;
    int   depth;
} Local;


typedef struct {
    Value constants[CHUNK_CONSTANTS_MAX];
    int constant_count;

    Local locals[256];
    int   local_count;
    int   scope_depth;

    Location* lines;

    int* locations;
    int  location_count;
    int  location_capacity;
    Location location_previous;

    u8* code;
    int count;
    int capacity;
} Chunk;


Chunk chunk_make();
void  chunk_write(Chunk* chunk, u8 byte, Location location);
uint8_t chunk_peek(Chunk* chunk);
void  chunk_free(Chunk* chunk);
int   chunk_add_constant(Chunk* chunk, Value constant);
void  chunk_disassemble(Chunk* chunk, const char* name);
int   chunk_instruction_disassemble(Chunk* chunk, int offset);

Location chunk_line(const Chunk* chunk, int offset);
