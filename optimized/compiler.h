#pragma once
#include "preamble.h"
#include "parser.h"
#include "array.h"
#include "opcodes.h"
#include "chunk.h"

declare_dynarray(OpCode)

#define COMPILER_MAX_ERRORS


typedef struct {
    Token name;
    int   depth;
} Local;

declare_dynarray(Local)
declare_dynarray(Chunk)

typedef struct {
    Parser* parser;

    DynArray_Chunk chunks;
    int current_chunk;

    Location current_location;

//    Error errors[COMPILER_MAX_ERRORS];
//    int error_count;

    int scope_depth;

    /* Compile-time storage for identifying locals. */
    DynArray_Local locals;

    bool had_error;
    bool in_panic_mode;
} Compiler;

Compiler compiler_make(Parser* parser);

void emit_byte(Compiler* comiler, u8 byte);

Ast* visit_identifier(Compiler* compiler, Ast_Identifier* node);
Ast* visit_literal(Compiler* compiler, Ast_Literal* node);
Ast* visit_bin_op(Compiler* compiler, Ast_BinOp* node);
Ast* visit_func_call(Compiler* compiler, Ast_FuncCall* node);
Ast* visit_var_assign(Compiler* compiler, Ast_VarAssign* node);
Ast* visit_var_decl(Compiler* compiler, Ast_VarDecl* node);
Ast* visit_func_decl(Compiler* compiler, Ast_FuncDecl* node);
Ast* visit_return_stmt(Compiler* compiler, Ast_ReturnStmt* node);
Ast* visit_if_stmt(Compiler* compiler, Ast_IfStmt* node);
Ast* visit_while_stmt(Compiler* compiler, Ast_WhileStmt* node);
Ast* visit_block(Compiler* compiler, Ast_Block* node);
Ast* visit_statement(Compiler* compiler, Ast* node);
Ast* visit_expression(Compiler* compiler, Ast* node);
