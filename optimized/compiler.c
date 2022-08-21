#include "compiler.h"
#include "ast.h"
#include "parser.h"
#include "value.h"
#include "object.h"


Compiler compiler_make(Parser* parser) {
    DynArray_Chunk chunks = make_dynarray_Chunk();
    dynarray_Chunk_append(&chunks, chunk_make());
    return (Compiler) {
        .parser=parser,
        .chunks=chunks,
        .current_chunk=0,
        .current_location=make_location(0, 1, 1),
        .scope_depth=0,
        .locals=make_dynarray_Local(),
        .had_error=0,
        .in_panic_mode=0,
    };
}


static Chunk* current_chunk(Compiler* compiler) {
    return dynarray_Chunk_get(&compiler->chunks, compiler->current_chunk);
}

static void synchronize(Compiler* compiler) {

}

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


void write(Compiler* compiler, u8 byte) {
    Chunk* chunk = current_chunk(compiler);
    chunk_write(chunk, byte, compiler->current_location);
}

int add_constant(Compiler* compiler, Value value) {
    Chunk* chunk = current_chunk(compiler);
    dynarray_Value_append(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void emit_byte(Compiler* compiler, u8 byte) {
    write(compiler, byte);
}

static void emit_bytes(Compiler* self, u8 byte1, u8 byte2) {
    emit_byte(self, byte1);
    emit_byte(self, byte2);
}

Ast* visit_identifier(Compiler* compiler, Ast_Identifier* node) {
    emit_bytes(compiler, OP_CONSTANT, (u8) node->name);
    return (Ast*) node;
}

Ast* visit_literal(Compiler* compiler, Ast_Literal* node) {
    Value value = { .type=VALUE_I64, .as.val_i64=node->value.int_ };
    int id = add_constant(compiler, value);
    emit_bytes(compiler, OP_CONSTANT, (u8) id);
    return (Ast*) node;
}

Ast* visit_bin_op(Compiler* compiler, Ast_BinOp* node) {
    Ast* left  = (Ast*)(node+1);
    Ast* right = (Ast*) ((u8*) left + node->right);
    visit_expression(compiler, left);
    visit_expression(compiler, right);
    switch (node->op) {
        case Add: emit_byte(compiler, OP_ADD);  break;
        case Sub: emit_byte(compiler, OP_SUB);  break;
        case Mul: emit_byte(compiler, OP_MUL);  break;
        case Div: emit_byte(compiler, OP_DIV);  break;
        case Mod: emit_byte(compiler, OP_MOD);  break;
        case Lt:  emit_byte(compiler, OP_LT);  break;
//        case Le:  emit_byte(compiler, OP_);  break;
        case Eq:  emit_byte(compiler, OP_EQ);  break;
//        case Ne:  emit_byte(compiler, OP_);  break;
//        case Ge:  emit_byte(compiler, OP_);  break;
        case Gt:  emit_byte(compiler, OP_GT);  break;
        case And: emit_byte(compiler, OP_AND);  break;
        case Or:  emit_byte(compiler, OP_OR);  break;
    }
    return (Ast*) node;
}

Ast* visit_func_call(Compiler* compiler, Ast_FuncCall* node) {

    return (Ast*) node;
}

Ast* visit_expression(Compiler* compiler, Ast* node) {
    switch (node->type) {
        case AST_IDENTIFIER: return visit_identifier(compiler, (Ast_Identifier*) node);
        case AST_LITERAL:    return visit_literal(compiler,    (Ast_Literal*) node);
        case AST_BIN_OP:     return visit_bin_op(compiler,     (Ast_BinOp *) node);
        case AST_FUNC_CALL:  return visit_func_call(compiler,  (Ast_FuncCall *) node);
        default: {
            // Expression / func call
            break;
        }
    }
}

Ast* visit_var_assign(Compiler* compiler, Ast_VarAssign* node) {
//    emit_bytes(compiler, OP_GET_GLOBAL, (u8) node->name);
//    visit_expression(compiler, (Ast*) (node+1));
//    emit_bytes(compiler, OP_SET_GLOBAL, (u8) node->name);
    return (Ast*) node;
}

Ast* visit_var_decl(Compiler* compiler, Ast_VarDecl* node) {
//    visit_expression(compiler, (Ast*) (node+1));
//    emit_bytes(compiler, OP_DEFINE_GLOBAL, (u8) node->name);
    return (Ast*) node;
}

Ast* visit_func_decl(Compiler* compiler, Ast_FuncDecl* node) {
//    emit_bytes(compiler, OP_CONSTANT, make_constant(self, MAKE_OBJ(function)));
//    emit_bytes(compiler, OP_DEFINE_GLOBAL, (u8) node->name);
    return (Ast*) node;
}

Ast* visit_return_stmt(Compiler* compiler, Ast_ReturnStmt* node) {
    return (Ast*) node;
}

Ast* visit_if_stmt(Compiler* compiler, Ast_IfStmt* node) {
    return (Ast*) node;
}

Ast* visit_while_stmt(Compiler* compiler, Ast_WhileStmt* node) {
    return (Ast*) node;
}

Ast* visit_block(Compiler* compiler, Ast_Block* node) {
    return (Ast*) node;
}


Ast* visit_statement(Compiler* compiler, Ast* node) {
    switch (node->type) {
        case AST_VAR_ASSIGN: {
            node = visit_var_assign(compiler, (Ast_VarAssign*) node);
            break;
        }
        case AST_VAR_DECL: {
            node = visit_var_decl(compiler, (Ast_VarDecl*) node);
            break;
        }
        case AST_FUNC_DECL: {
            node = visit_func_decl(compiler, (Ast_FuncDecl*) node);
            break;
        }
        case AST_RETURN_STMT: {
            node = visit_return_stmt(compiler, (Ast_ReturnStmt*) node);
            break;
        }
        case AST_IF_STMT: {
            node = visit_if_stmt(compiler, (Ast_IfStmt*) node);
            break;
        }
        case AST_WHILE_STMT: {
            node = visit_while_stmt(compiler, (Ast_WhileStmt*) node);
            break;
        }
        case AST_BLOCK: {
            node = visit_block(compiler, (Ast_Block*) node);
            break;
        }
        default: {
            // Expression / func call
            break;
        }
    }

    if (compiler->had_error)
        synchronize(compiler);

    return node;
}


Ast* visit_module(Compiler* compiler, Ast_Module* module) {

    Ast* node = (Ast*) (module + 1);
    for (u32 i = 0; i < module->stmt_count; ++i) {
        node = visit_statement(compiler, node);
    }
    
}

