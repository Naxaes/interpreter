#include "memory.h"
#include "chunk.h"
#include "opcodes.h"
#include "error.h"


// Internal
static int instruction_constant(const char* name, Chunk* chunk, int offset);
static int instruction_simple(const char* name, int offset);
static int instruction_byte(const char* name, Chunk* chunk, int offset);
static int instruction_jump(const char* name, int sign, Chunk* chunk, int offset);
static int instruction_identifier(const char* name, Chunk* chunk, int offset);
static void chunk_add_line(Chunk* chunk, Location location);

Chunk chunk_make() {
    Chunk chunk;
    memset(chunk.constants, 0, CHUNK_CONSTANTS_MAX);
    chunk.constant_count = 0;

    chunk.location_previous = (Location) { .row=0, .col=0, .index=0 };
    chunk.location_count = 1;
    chunk.location_capacity  = GROW_CAPACITY(0);
    chunk.locations = RESIZE_ARRAY(int, NULL, 0, chunk.location_capacity);
    chunk.locations[0] = 0;

    chunk.lines  = NULL;

    chunk.local_count = 0;
    Local* local = &chunk.locals[chunk.local_count++];
    local->depth = 0;
    local->name  =  token_make_empty();

    chunk.code     = NULL;
    chunk.count    = 0;
    chunk.capacity = 0;

    return chunk;
}

void chunk_write(Chunk* chunk, u8 byte, Location location) {
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity  = GROW_CAPACITY(old_capacity);
        chunk->code      = RESIZE_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines     = RESIZE_ARRAY(Location, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk_add_line(chunk, location);
    chunk->location_previous = location;

    chunk->code[chunk->count]  = byte;
    chunk->lines[chunk->count] = location;
    chunk->count++;
}

uint8_t chunk_peek(Chunk* chunk) {
    if (chunk->count > 0)
        return chunk->code[chunk->count-1];
    else
        return 0;
}


int chunk_add_constant(Chunk* chunk, Value constant) {
    chunk->constants[chunk->constant_count] = constant;
    return chunk->constant_count++;
}

void chunk_free(Chunk* chunk) {
    FREE_ARRAY(u8,  chunk->code,  chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    *chunk = chunk_make();
}

Location chunk_line(const Chunk* chunk, int offset) {
    return chunk->lines[offset];
    int row = 0;
    int col = 0;
    int index = 0;


    for (int i = 0; i < chunk->location_count && index < offset; ++i) {
        int x = chunk->locations[i];
        if (x > 0) {
            col   += x;
            index += x;
        } else {
            col  = 0;
            row -= x;
            index -= x;
        }
    }

    if (index == offset)
        return (Location) { .row=row, .col=col, .index=index };
    else
        return (Location) { .row=row, .col=col, .index=index };
}

static void chunk_add_line(Chunk* chunk, Location location) {
    int* previous = &chunk->locations[chunk->location_count-1];
    int rows  = location.row - chunk->location_previous.row;
    int cols  = location.col - chunk->location_previous.col;

    if (*previous <= 0 && rows > 0) {
        // Add to row count.
        *previous -= rows;
    } else if (*previous > 0 && rows == 0) {
        // Add to column count.
        *previous += cols;
        return;
    }

    if (*previous > 0 && rows > 0) {
        if (chunk->location_count + 1 >= chunk->location_capacity) {
            int old_location_capacity = chunk->location_capacity;
            chunk->location_capacity  = GROW_CAPACITY(old_location_capacity);
            chunk->locations = RESIZE_ARRAY(int, chunk->locations, old_location_capacity, chunk->location_capacity);
        }
        // Append a new row count.
        chunk->locations[chunk->location_count++] = -rows;
    }
    if (*previous < 0 && cols > 0) {
        if (chunk->location_count + 1 >= chunk->location_capacity) {
            int old_location_capacity = chunk->location_capacity;
            chunk->location_capacity  = GROW_CAPACITY(old_location_capacity);
            chunk->locations = RESIZE_ARRAY(int, chunk->locations, old_location_capacity, chunk->location_capacity);
        }
        // Append a new column count.
        chunk->locations[chunk->location_count++] = cols;
    }
}


void chunk_disassemble(Chunk* chunk, const char* name) {
    printf("====== %s ========\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = chunk_instruction_disassemble(chunk, offset);
    }

    printf("======================\n\n");
}


int chunk_instruction_disassemble(Chunk* chunk, int offset) {
    printf("%04d", offset);

    if (offset > 0 && chunk->lines[offset].row == chunk->lines[offset - 1].row) {
        printf("    | ");
    } else {
        printf(":%04d ", chunk->lines[offset].row);
    }

    u8 instruction = chunk->code[offset];
    switch (instruction) {
        case OP_PRINT:         return instruction_simple("OP_PRINT",    offset);
        case OP_EXIT:          return instruction_simple("OP_EXIT",     offset);
        case OP_POP:           return instruction_simple("OP_POP",      offset);
        case OP_TRUE:          return instruction_simple("OP_TRUE",     offset);
        case OP_FALSE:         return instruction_simple("OP_FALSE",    offset);
        case OP_NEGATE:        return instruction_simple("OP_NEGATE",   offset);
        case OP_NOT:           return instruction_simple("OP_NOT",      offset);
        case OP_EQUAL:         return instruction_simple("OP_EQUAL",    offset);
        case OP_GREATER:       return instruction_simple("OP_GREATER",  offset);
        case OP_LESS:          return instruction_simple("OP_LESS",     offset);
        case OP_AND:           return instruction_simple("OP_AND",      offset);
        case OP_OR:            return instruction_simple("OP_OR",       offset);
        case OP_ADD:           return instruction_simple("OP_ADD",      offset);
        case OP_SUBTRACT:      return instruction_simple("OP_SUBTRACT", offset);
        case OP_MULTIPLY:      return instruction_simple("OP_MULTIPLY", offset);
        case OP_DIVIDE:        return instruction_simple("OP_DIVIDE",   offset);
        case OP_NULL:          return instruction_simple("OP_NULL",     offset);
        case OP_RETURN:        return instruction_simple("OP_RETURN",   offset);
        case OP_CONSTANT:      return instruction_constant("OP_CONSTANT",      chunk, offset);
        case OP_DEFINE_GLOBAL: return instruction_identifier("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:    return instruction_identifier("OP_GET_GLOBAL",    chunk, offset);
        case OP_SET_GLOBAL:    return instruction_identifier("OP_SET_GLOBAL",    chunk, offset);
        case OP_GET_LOCAL:     return instruction_byte("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:     return instruction_byte("OP_SET_LOCAL", chunk, offset);
        case OP_CALL:          return instruction_byte("OP_CALL",      chunk, offset);
        case OP_JUMP_IF_FALSE: return instruction_jump("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_JUMP:          return instruction_jump("OP_JUMP",  1, chunk, offset);
        case OP_LOOP:          return instruction_jump("OP_LOOP", -1, chunk, offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

static int instruction_jump(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-20s %04d -> %04d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int instruction_byte(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-20s %-4d\n", name, slot);
    return offset + 2;
}

static int instruction_simple(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int instruction_constant(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-20s %-4d '", name, constant);
    print_value(chunk->constants[constant]);
    printf("' ");
    print_type(chunk->constants[constant]);
    printf("\n");
    return offset + 2;
}

static int instruction_identifier(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-20s %-4d '", name, constant);
    print_value(chunk->constants[constant]);
    printf("' Identifier\n");
    return offset + 2;
}

