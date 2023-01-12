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
        .variables=make_dynarray_Variable(),
        .had_error=0,
        .in_panic_mode=0,
    };
}


static Chunk* current_chunk(Compiler* compiler) {
    return dynarray_Chunk_get(&compiler->chunks, compiler->current_chunk);
}

static void synchronize(Compiler* compiler) {

}


CompilerReturn type_check_identifier(Compiler* compiler, Ast_Identifier* node);
CompilerReturn type_check_literal(Compiler* compiler, Ast_Literal* node);
CompilerReturn type_check_bin_op(Compiler* compiler, Ast_BinOp* node);
CompilerReturn type_check_func_call(Compiler* compiler, Ast_FuncCall* node);
CompilerReturn type_check_var_assign(Compiler* compiler, Ast_VarAssign* node);
CompilerReturn type_check_var_decl(Compiler* compiler, Ast_VarDecl* node);
CompilerReturn type_check_func_decl(Compiler* compiler, Ast_FuncDecl* node);
CompilerReturn type_check_return_stmt(Compiler* compiler, Ast_ReturnStmt* node);
CompilerReturn type_check_if_stmt(Compiler* compiler, Ast_IfStmt* node);
CompilerReturn type_check_while_stmt(Compiler* compiler, Ast_WhileStmt* node);
CompilerReturn type_check_block(Compiler* compiler, Ast_Block* node);
CompilerReturn type_check_statement(Compiler* compiler, Ast* node);
CompilerReturn type_check_expression(Compiler* compiler, Ast* node);

//
//CompilerReturn type_check_identifier(Compiler* compiler, Ast_Identifier* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_literal(Compiler* compiler, Ast_Literal* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_bin_op(Compiler* compiler, Ast_BinOp* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_func_call(Compiler* compiler, Ast_FuncCall* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_var_assign(Compiler* compiler, Ast_VarAssign* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_var_decl(Compiler* compiler, Ast_VarDecl* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_func_decl(Compiler* compiler, Ast_FuncDecl* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_return_stmt(Compiler* compiler, Ast_ReturnStmt* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_if_stmt(Compiler* compiler, Ast_IfStmt* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_while_stmt(Compiler* compiler, Ast_WhileStmt* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_block(Compiler* compiler, Ast_Block* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_statement(Compiler* compiler, Ast* node) {
//    return make_primitive_type(0);
//}
//CompilerReturn type_check_expression(Compiler* compiler, Ast* node) {
//    return make_primitive_type(0);
//}



/* ---- Code Emission ---- */
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



CompilerReturn emit_identifier(Compiler* compiler, Ast_Identifier* node);
CompilerReturn emit_literal(Compiler* compiler, Ast_Literal* node);
CompilerReturn emit_bin_op(Compiler* compiler, Ast_BinOp* node);
CompilerReturn emit_func_call(Compiler* compiler, Ast_FuncCall* node);
CompilerReturn emit_expression(Compiler* compiler, Ast* node);

CompilerReturn emit_var_assign(Compiler* compiler, Ast_VarAssign* node);
CompilerReturn emit_var_decl(Compiler* compiler, Ast_VarDecl* node);
CompilerReturn emit_func_decl(Compiler* compiler, Ast_FuncDecl* node);
CompilerReturn emit_return_stmt(Compiler* compiler, Ast_ReturnStmt* node);
CompilerReturn emit_if_stmt(Compiler* compiler, Ast_IfStmt* node);
CompilerReturn emit_while_stmt(Compiler* compiler, Ast_WhileStmt* node);
CompilerReturn emit_block(Compiler* compiler, Ast_Block* node);

CompilerReturn emit_statement(Compiler* compiler, Ast* node);


CompilerReturn emit_identifier(Compiler* compiler, Ast_Identifier* node) {
    emit_bytes(compiler, OP_CONSTANT, (u8) node->scope_local_offset);
    return (CompilerReturn) { .next=(Ast*)(node+1), .type=make_primitive_type(PrimitiveType_i64) };
}



CompilerReturn emit_literal(Compiler* compiler, Ast_Literal* node) {
    Value value;
    switch (node->type) {
        case PrimitiveType_number:
        case PrimitiveType_u8:
        case PrimitiveType_u16:
        case PrimitiveType_u32:
        case PrimitiveType_rune:
        case PrimitiveType_u64:
        case PrimitiveType_u128:
            value = MAKE_I64(node->value.int_);  // @TODO: Fix.
            break;

        case PrimitiveType_int:
        case PrimitiveType_bool:
        case PrimitiveType_i8:
        case PrimitiveType_i16:
        case PrimitiveType_i32:
        case PrimitiveType_i64:
        case PrimitiveType_i128:
            value = MAKE_I64(node->value.int_);
            break;

        case PrimitiveType_float:
        case PrimitiveType_f16:
        case PrimitiveType_f32:
        case PrimitiveType_f64:
        case PrimitiveType_f128:
            value = MAKE_F64(node->value.float_);
            break;

        case PrimitiveType_string: {
            Slice view = *dynarray_Slice_get(&compiler->parser->strings, (u32) node->value.string);
            value = MAKE_OBJ(string_make(view.source, view.count));
            break;
        }
        default: unreachable();
        static_assert(PrimitiveTypeCount == 26, "Exhaustive");
    }

    int id = add_constant(compiler, value);
    emit_bytes(compiler, OP_CONSTANT, (u8) id);
    return (CompilerReturn) { .next=(Ast*)(node+1), .type=value.type };
}

CompilerReturn emit_bin_op(Compiler* compiler, Ast_BinOp* node) {
    Ast* left  = bin_op_left(node);
    Ast* right = bin_op_right(node);
    CompilerReturn left_type  = emit_expression(compiler, left);
    CompilerReturn right_type = emit_expression(compiler, right);

    if (!is_type(left_type.type, right_type.type))
        PANIC("Type error!");

    switch (node->op) {
        case Add: emit_byte(compiler, OP_ADD); break;
        case Sub: emit_byte(compiler, OP_SUB); break;
        case Mul: emit_byte(compiler, OP_MUL); break;
        case Div: emit_byte(compiler, OP_DIV); break;
        case Mod: emit_byte(compiler, OP_MOD); break;
        case Lt:  emit_byte(compiler, OP_LT);  break;
//        case Le:  emit_byte(compiler, OP_);  break;
        case Eq:  emit_byte(compiler, OP_EQ);  break;
//        case Ne:  emit_byte(compiler, OP_);  break;
//        case Ge:  emit_byte(compiler, OP_);  break;
        case Gt:  emit_byte(compiler, OP_GT);  break;
        case And: emit_byte(compiler, OP_AND); break;
        case Or:  emit_byte(compiler, OP_OR);  break;
        default: unreachable();
        static_assert(PrimitiveTypeCount == 26, "Exhaustive");
    }
    return (CompilerReturn) { .next=right_type.next, .type=left_type.type };
}



CompilerReturn emit_func_call(Compiler* self, Ast_FuncCall* node) {
    Ast* expression = (Ast*) (node + 1);
    for (u32 i = 0; i < node->arg_count; ++i) {
        CompilerReturn result = emit_expression(self, expression);
        expression = result.next;
    }
    emit_bytes(self, OP_CALL, (u8) node->arg_count);
    return (CompilerReturn) { .next=expression };
}

CompilerReturn emit_expression(Compiler* compiler, Ast* node) {
    switch (node->type) {
        case AST_IDENTIFIER: return emit_identifier(compiler, (Ast_Identifier*) node);
        case AST_LITERAL:    return emit_literal(compiler,    (Ast_Literal*) node);
        case AST_BIN_OP:     return emit_bin_op(compiler,     (Ast_BinOp *) node);
        case AST_FUNC_CALL:  return emit_func_call(compiler,  (Ast_FuncCall *) node);
        default: {
            // Expression / func call
            PANIC("Not implemented");
            return (CompilerReturn) { 0 };
        }
    }
}

CompilerReturn emit_var_assign(Compiler* compiler, Ast_VarAssign* node) {
    emit_bytes(compiler, OP_GET_GLOBAL, (u8) node->name);
    CompilerReturn result = emit_expression(compiler, (Ast*) (node+1));
    emit_bytes(compiler, OP_SET_GLOBAL, (u8) node->name);
    return (CompilerReturn) { .next=result.next, .type=make_primitive_type(PrimitiveType_null) };
}

CompilerReturn emit_var_decl(Compiler* compiler, Ast_VarDecl* node) {
    CompilerReturn result = emit_expression(compiler, (Ast*) (node+1));
    emit_bytes(compiler, OP_DEFINE_GLOBAL, (u8) node->name);
    return (CompilerReturn) { .next=result.next, .type=make_primitive_type(PrimitiveType_null) };
}

CompilerReturn emit_func_decl(Compiler* compiler, Ast_FuncDecl* node) {
//    emit_bytes(compiler, OP_CONSTANT, make_constant(self, MAKE_OBJ(function)));
//    emit_bytes(compiler, OP_DEFINE_GLOBAL, (u8) node->name);
    return (CompilerReturn) { .next=(Ast*) node, .type=make_primitive_type(PrimitiveType_null) };
}

CompilerReturn emit_return_stmt(Compiler* compiler, Ast_ReturnStmt* node) {
    return (CompilerReturn) { .next=(Ast*) node, .type=make_primitive_type(PrimitiveType_null) };
}

CompilerReturn emit_if_stmt(Compiler* compiler, Ast_IfStmt* node) {
    return (CompilerReturn) { .next=(Ast*) node, .type=make_primitive_type(PrimitiveType_null) };
}

CompilerReturn emit_while_stmt(Compiler* compiler, Ast_WhileStmt* node) {
    return (CompilerReturn) { .next=(Ast*) node, .type=make_primitive_type(PrimitiveType_null) };
}

CompilerReturn emit_block(Compiler* self, Ast_Block* node) {
    Ast* statement = (Ast*) (node + 1);
    CompilerReturn result = { 0 };
    for (u32 i = 0; i < node->stmt_count; ++i) {
        result = emit_statement(self, statement);
        statement = result.next;
    }
    return (CompilerReturn) { .next=result.next, .type=make_primitive_type(PrimitiveType_null) };
}


CompilerReturn emit_statement(Compiler* compiler, Ast* node) {
    CompilerReturn result = { 0 };
    switch (node->type) {
        case AST_VAR_ASSIGN: {
            result = emit_var_assign(compiler, (Ast_VarAssign*) node);
            break;
        }
        case AST_VAR_DECL: {
            result = emit_var_decl(compiler, (Ast_VarDecl*) node);
            break;
        }
        case AST_FUNC_DECL: {
            result = emit_func_decl(compiler, (Ast_FuncDecl*) node);
            break;
        }
        case AST_RETURN_STMT: {
            result = emit_return_stmt(compiler, (Ast_ReturnStmt*) node);
            break;
        }
        case AST_IF_STMT: {
            result = emit_if_stmt(compiler, (Ast_IfStmt*) node);
            break;
        }
        case AST_WHILE_STMT: {
            result = emit_while_stmt(compiler, (Ast_WhileStmt*) node);
            break;
        }
        case AST_BLOCK: {
            result = emit_block(compiler, (Ast_Block*) node);
            break;
        }
        default: {
            result = emit_expression(compiler, node);
            break;
        }
    }

    if (compiler->had_error)
        synchronize(compiler);

    return result;
}


void emit_module(Compiler* compiler, Ast_Module* module) {
    Ast* node = (Ast*) (module + 1);
    for (u32 i = 0; i < module->stmt_count; ++i) {
        CompilerReturn result = emit_statement(compiler, node);
        node = result.next;
    }
}


void compile(Compiler* compiler, Ast_Module* node) {
    emit_module(compiler, node);
    emit_byte(compiler, OP_EXIT);
}
