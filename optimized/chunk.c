#include "memory.h"
#include "chunk.h"
#include "opcodes.h"


// Internal
static int instruction_constant(const char* name, Chunk* chunk, int offset);
static int instruction_simple(const char* name, int offset);
static int instruction_byte(const char* name, Chunk* chunk, int offset);
static int instruction_jump(const char* name, int sign, Chunk* chunk, int offset);
static int instruction_identifier(const char* name, Chunk* chunk, int offset);


Chunk chunk_make() {
    Chunk chunk = {
        .constants=make_dynarray_Value(),
        .code=make_dynarray_u8(),
        .locations=make_dynarray_Location(),
    };
    return chunk;
}

void chunk_write(Chunk* chunk, u8 byte, Location location) {
    dynarray_u8_append(&chunk->code, byte);
    dynarray_Location_append(&chunk->locations, location);
}

u8 chunk_peek(Chunk* chunk) {
    if (chunk->code.count > 0)
        return *dynarray_u8_get(&chunk->code, chunk->code.count-1);
    else
        return 0;
}


int chunk_add_constant(Chunk* chunk, Value constant) {
    int id = chunk->constants.count;
    dynarray_Value_append(&chunk->constants, constant);
    return id;
}

void chunk_free(Chunk* chunk) {
    // @TODO: Free!
    *chunk = chunk_make();
}

Location chunk_line(Chunk* chunk, int offset) {
    return *dynarray_Location_get(&chunk->locations, offset);
}


void chunk_disassemble(Chunk* chunk, const char* name) {
    printf("====== %s ========\n", name);

    for (int offset = 0; offset < (int) chunk->code.count;) {
        offset = chunk_instruction_disassemble(chunk, offset);
    }

    printf("======================\n\n");
}


int chunk_instruction_disassemble(Chunk* chunk, int offset) {
    printf("%04d", offset);

    if (offset > 0 && dynarray_Location_get(&chunk->locations, offset)->row == dynarray_Location_get(&chunk->locations, offset-1)->row) {
        printf("    | ");
    } else {
        printf(":%04d ",dynarray_Location_get(&chunk->locations, offset)->row);
    }

    u8 instruction = *dynarray_u8_get(&chunk->code, offset);
    switch (instruction) {
        case OP_PRINT:         return instruction_simple("OP_PRINT",    offset);
        case OP_EXIT:          return instruction_simple("OP_EXIT",     offset);
        case OP_POP:           return instruction_simple("OP_POP",      offset);
        case OP_TRUE:          return instruction_simple("OP_TRUE",     offset);
        case OP_FALSE:         return instruction_simple("OP_FALSE",    offset);
        case OP_NEGATE:        return instruction_simple("OP_NEGATE",   offset);
        case OP_NOT:           return instruction_simple("OP_NOT",      offset);
        case OP_EQ:            return instruction_simple("OP_EQ",       offset);
        case OP_GT:            return instruction_simple("OP_GT",       offset);
        case OP_LT:            return instruction_simple("OP_LT",       offset);
        case OP_AND:           return instruction_simple("OP_AND",      offset);
        case OP_OR:            return instruction_simple("OP_OR",       offset);
        case OP_ADD:           return instruction_simple("OP_ADD",      offset);
        case OP_SUB:           return instruction_simple("OP_SUB",      offset);
        case OP_MUL:           return instruction_simple("OP_MUL",      offset);
        case OP_DIV:           return instruction_simple("OP_DIV",      offset);
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
    uint16_t jump = (uint16_t)(*dynarray_u8_get(&chunk->code, offset + 1) << 8);
    jump |= *dynarray_u8_get(&chunk->code, offset + 2);
    printf("%-20s %04d -> %04d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int instruction_byte(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = *dynarray_u8_get(&chunk->code, offset + 1);
    printf("%-20s %-4d\n", name, slot);
    return offset + 2;
}

static int instruction_simple(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int instruction_constant(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = *dynarray_u8_get(&chunk->code, offset + 1);
    printf("%-20s %-4d '", name, constant);
    print_value(*dynarray_Value_get(&chunk->constants, constant));
    printf("' ");
    print_type(*dynarray_Value_get(&chunk->constants, constant));
    printf("\n");
    return offset + 2;
}

static int instruction_identifier(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = *dynarray_u8_get(&chunk->code, offset + 1);
    printf("%-20s %-4d '", name, constant);
    print_value(*dynarray_Value_get(&chunk->constants, constant));
    printf("' Identifier\n");
    return offset + 2;
}

