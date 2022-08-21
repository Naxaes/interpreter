#pragma once

typedef enum {
    OP_INVALID,
    OP_EXIT,
    OP_PRINT,

    OP_POP,

    OP_CONSTANT,
    OP_TRUE,
    OP_FALSE,

    OP_EQ,
    OP_GT,
    OP_LT,

    OP_NOT,
    OP_AND,
    OP_OR,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_NEGATE,

    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,

    OP_CALL,
    OP_RETURN,
    OP_NULL,

} OpCode;
