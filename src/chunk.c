#include "chunk.h"
#include "opcodes.h"
#include "array.h"
#include "slice.h"


// Internal
static int instruction_constant(const char* name, Chunk* chunk, int offset);
static int instruction_simple(const char* name, int offset);
int instruction_byte(const char* name, Chunk* chunk, int offset);
int instruction_jump(const char* name, int sign, Chunk* chunk, int offset);
static void chunk_print_line(Chunk* chunk, int offset);



Chunk chunk_make(const char* source) {
    Chunk chunk;
    memset(chunk.constants, 0, CHUNK_CONSTANTS_MAX);
    chunk.constant_count = 0;

    chunk.source = source;
    chunk.lines  = NULL;

    chunk.code     = NULL;
    chunk.count    = 0;
    chunk.capacity = 0;

    return chunk;
}

void chunk_write(Chunk* chunk, u8 byte, Location location) {
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity  = GROW_CAPACITY(old_capacity);
        chunk->code      = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines     = GROW_ARRAY(Location, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count]  = byte;
    chunk->lines[chunk->count] = location;
    chunk->count++;
}

int chunk_add_constant(Chunk* chunk, Value constant) {
    chunk->constants[chunk->constant_count] = constant;
    return chunk->constant_count++;
}

void chunk_free(Chunk* chunk) {
    FREE_ARRAY(u8,  chunk->code,  chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    *chunk = chunk_make(chunk->source);
}


void chunk_disassemble(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = chunk_instruction_disassemble(chunk, offset);
    }
}


//static void chunk_print_line(Chunk* chunk, int offset) {
//    Location location = chunk->lines[offset];
//    Slice line = current_line(chunk->source, location.index);
//    printf("'%.*s'\n", line.count, line.source);
//}


int chunk_instruction_disassemble(Chunk* chunk, int offset) {
    printf("%04d", offset);

    if (offset > 0 && chunk->lines[offset].row == chunk->lines[offset - 1].row) {
        printf("    | ");
    } else {
        printf(":%04d ", chunk->lines[offset].row);
    }

    u8 instruction = chunk->code[offset];
    switch (instruction) {
        case STMT_RETURN:      return instruction_simple("STMT_RETURN", offset);
        case STMT_PRINT:       return instruction_simple("STMT_PRINT",  offset);
        case STMT_EXIT:        return instruction_simple("STMT_EXIT",   offset);
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
        case OP_CONSTANT:      return instruction_constant("OP_CONSTANT",      chunk, offset);
        case OP_DEFINE_GLOBAL: return instruction_constant("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:    return instruction_constant("OP_GET_GLOBAL",    chunk, offset);
        case OP_SET_GLOBAL:    return instruction_constant("OP_SET_GLOBAL",    chunk, offset);
        case OP_GET_LOCAL:     return instruction_byte("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:     return instruction_byte("OP_SET_LOCAL", chunk, offset);
        case OP_JUMP_IF_FALSE: return instruction_jump("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_JUMP:          return instruction_jump("OP_JUMP",  1, chunk, offset);
        case OP_LOOP:          return instruction_jump("OP_LOOP", -1, chunk, offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

int instruction_jump(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-20s %04d -> %04d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int instruction_byte(const char* name, Chunk* chunk, int offset) {
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

