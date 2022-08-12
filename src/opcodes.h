#pragma once

typedef enum {
    STMT_EXIT,
    STMT_RETURN,
    STMT_PRINT,

    OP_POP,

    OP_CONSTANT,
    OP_TRUE,
    OP_FALSE,

    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    OP_NOT,
    OP_AND,
    OP_OR,

    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    OP_NEGATE,

    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP


} OpCode;
