#pragma once

#include "slice.h"

typedef enum {
    OP_INVALID,
    OP_EXIT,
    OP_PRINT,

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
    OP_LOOP,

    OP_CALL,
    OP_RETURN,
    OP_NULL,
} OpCode;

typedef enum {
    TYPE_ANY = 0,
    TYPE_FLOAT,
    TYPE_SIGNED_INT,
    TYPE_UNSIGNED_INT,
    TYPE_MEM_ADDR,
    TYPE_INST_ADDR,
    TYPE_STACK_ADDR,
    TYPE_NATIVE_ID,
    TYPE_BOOL,
    COUNT_TYPES
} Type;

#define TYPE_LIST_CAPACITY 2
typedef struct {
    size_t size;
    Type types[];
} Type_List;

static Type_List x = { .size=4, .types={ TYPE_ANY, TYPE_ANY } };

#define TYPE_LIST(...) { .size = sizeof((Type[]){__VA_ARGS__}) / sizeof(Type), .types = {__VA_ARGS__} }
#define TYPE_EMPTY { .size = 0 .types = 0 }

// https://github.com/tsoding/bm/blob/master/bm/src/bm.c
// X(type, has_operand, input_type, output_type)
#define ALL_OPCODES(X)                                                                                              \
                                                                                                                    \
    X(INVALID, false)                                                                                                   \
    X(EXIT, false)                                                                                                      \
    X(PRINT, false, TYPE_LIST(TYPE_UNSIGNED_INT, TYPE_ANY), TYPE_EMPTY)                                                             \
    X(POP, false, TYPE_EMPTY, )                                                                                                       \
    X(CONSTANT, false)                                                                                                  \
    X(TRUE, false)                                                                                                      \
    X(FALSE, false)                                                                                                     \
    X(EQUAL, false)                                                                                                     \
    X(GREATER, false)                                                                                                   \
    X(LESS, false)                                                                                                      \
    X(NOT, false)                                                                                                       \
    X(AND, false)                                                                                                       \
    X(OR, false)                                                                                                        \
    X(ADD, false)                                                                                                       \
    X(SUBTRACT, false)                                                                                                  \
    X(MULTIPLY, false)                                                                                                  \
    X(DIVIDE, false)                                                                                                    \
    X(NEGATE, false)                                                                                                    \
    X(DEFINE_GLOBAL, false)                                                                                             \
    X(GET_GLOBAL, false)                                                                                                \
    X(SET_GLOBAL, false)                                                                                                \
    X(GET_LOCAL, false)                                                                                                 \
    X(SET_LOCAL, false)                                                                                                 \
    X(JUMP, false)                                                                                                      \
    X(JUMP_IF_FALSE, false)                                                                                             \
    X(LOOP, false)                                                                                                      \
    X(CALL, false)                                                                                                      \
    X(RETURN, false)                                                                                                    \
    X(NULL, false)                                                                                                      \




//
//typedef struct {
//    bool exists;
//    Inst_Type inst;
//    Bang_Type ret;
//} Binary_Op_Of_Type;
//
//static const Binary_Op_Of_Type binary_op_of_type_table[COUNT_BANG_TYPES][COUNT_BANG_BINARY_OP_KINDS] = {
//    [BANG_TYPE_VOID] = {
//            [BANG_BINARY_OP_KIND_PLUS]  = {.exists = false},
//            [BANG_BINARY_OP_KIND_MINUS] = {.exists = false},
//            [BANG_BINARY_OP_KIND_MULT]  = {.exists = false},
//            [BANG_BINARY_OP_KIND_LT]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_GE]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_NE]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
//            [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_EQ]    = {.exists = false},
//    },
//    [BANG_TYPE_I64] = {
//            [BANG_BINARY_OP_KIND_PLUS]  = {.exists = true, .inst = INST_PLUSI,  .ret = BANG_TYPE_I64},
//            [BANG_BINARY_OP_KIND_MINUS] = {.exists = true, .inst = INST_MINUSI, .ret = BANG_TYPE_I64},
//            [BANG_BINARY_OP_KIND_MULT]  = {.exists = true, .inst = INST_MULTI,  .ret = BANG_TYPE_I64},
//            [BANG_BINARY_OP_KIND_LT]    = {.exists = true, .inst = INST_LTI,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_GE]    = {.exists = true, .inst = INST_GEI,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_NE]    = {.exists = true, .inst = INST_NEI,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
//            [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_EQ]    = {.exists = true, .inst = INST_EQI,    .ret = BANG_TYPE_BOOL},
//    },
//    [BANG_TYPE_U8] = {
//            [BANG_BINARY_OP_KIND_PLUS]  = {.exists = true, .inst = INST_PLUSI,  .ret = BANG_TYPE_U8},
//            [BANG_BINARY_OP_KIND_MINUS] = {.exists = true, .inst = INST_MINUSI, .ret = BANG_TYPE_U8},
//            [BANG_BINARY_OP_KIND_MULT]  = {.exists = true, .inst = INST_MULTU,  .ret = BANG_TYPE_U8},
//            [BANG_BINARY_OP_KIND_LT]    = {.exists = true, .inst = INST_LTU,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_GE]    = {.exists = true, .inst = INST_GEU,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_NE]    = {.exists = true, .inst = INST_NEU,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
//            [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_EQ]    = {.exists = true, .inst = INST_EQU,    .ret = BANG_TYPE_BOOL},
//    },
//    [BANG_TYPE_BOOL] = {
//            [BANG_BINARY_OP_KIND_PLUS]  = {.exists = false},
//            [BANG_BINARY_OP_KIND_MINUS] = {.exists = false},
//            [BANG_BINARY_OP_KIND_MULT]  = {.exists = false},
//            [BANG_BINARY_OP_KIND_LT]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_GE]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_NE]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_AND]   = {.exists = true, .inst = INST_ANDB, .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_OR]    = {.exists = true, .inst = INST_ORB,  .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_EQ]    = {.exists = false},
//    },
//    [BANG_TYPE_PTR] = {
//            [BANG_BINARY_OP_KIND_PLUS]  = {.exists = true, .inst = INST_PLUSI,  .ret = BANG_TYPE_PTR},
//            [BANG_BINARY_OP_KIND_MINUS] = {.exists = true, .inst = INST_MINUSI, .ret = BANG_TYPE_PTR},
//            [BANG_BINARY_OP_KIND_MULT]  = {.exists = true, .inst = INST_MULTU,  .ret = BANG_TYPE_PTR},
//            [BANG_BINARY_OP_KIND_LT]    = {.exists = true, .inst = INST_LTU,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_GE]    = {.exists = true, .inst = INST_GEU,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_NE]    = {.exists = true, .inst = INST_NEU,    .ret = BANG_TYPE_BOOL},
//            [BANG_BINARY_OP_KIND_AND]   = {.exists = false},
//            [BANG_BINARY_OP_KIND_OR]    = {.exists = false},
//            [BANG_BINARY_OP_KIND_EQ]    = {.exists = true, .inst = INST_EQU,    .ret = BANG_TYPE_BOOL},
//    },
//};
//static_assert(
//    COUNT_BANG_TYPES == 5,
//    "The amount of types have changed. "
//    "Please update the binary_op_of_type table accordingly. "
//    "Thanks!");
//static_assert(
//    COUNT_BANG_BINARY_OP_KINDS == 9,
//    "The amount of binary operations have changed. "
//    "Please update the binary_op_of_type table accordingly. "
//    "Thanks!");

//
//Bang_Type compile_binary_op_into_basm(Bang *bang, Basm *basm, Bang_Binary_Op binary_op)
//{
//    const Compiled_Expr compiled_lhs = compile_bang_expr_into_basm(bang, basm, binary_op.lhs);
//    const Compiled_Expr compiled_rhs = compile_bang_expr_into_basm(bang, basm, binary_op.rhs);
//
//    if (compiled_lhs.type != compiled_rhs.type) {
//        bang_diag_msg(binary_op.loc, BANG_DIAG_ERROR,
//                      "LHS of `%s` has type `%s` but RHS has type `%s`",
//                      bang_token_kind_name(bang_binary_op_def(binary_op.kind).token_kind),
//                      bang_type_def(compiled_lhs.type).name,
//                      bang_type_def(compiled_rhs.type).name);
//        exit(1);
//    }
//    const Bang_Type type = compiled_lhs.type;
//
//    Binary_Op_Of_Type boot = binary_op_of_type_table[type][binary_op.kind];
//    if (!boot.exists) {
//        bang_diag_msg(binary_op.loc, BANG_DIAG_ERROR,
//                      "binary operation `%s` does not exist for type `%s`",
//                      bang_token_kind_name(bang_binary_op_def(binary_op.kind).token_kind),
//                      bang_type_def(type).name);
//        exit(1);
//    }
//
//    basm_push_inst(basm, boot.inst, word_u64(0));
//
//    return boot.ret;
//}


