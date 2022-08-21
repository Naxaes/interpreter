#include "ast.h"


#define is_word(s, x) (memcmp(s, x, sizeof(x)-1) == 0 && *((s) + sizeof(x)-1) == '\0')


PrimitiveType get_primitive(Slice view) {
    switch (*view.source) {
        case 'i':
            if      (is_word(view.source+1, "nt"))  return PRIMITIVE_TYPE_int;
            else if (is_word(view.source+1, "8"))   return PRIMITIVE_TYPE_i8;
            else if (is_word(view.source+1, "16"))  return PRIMITIVE_TYPE_i16;
            else if (is_word(view.source+1, "32"))  return PRIMITIVE_TYPE_i32;
            else if (is_word(view.source+1, "64"))  return PRIMITIVE_TYPE_i64;
            else if (is_word(view.source+1, "128")) return PRIMITIVE_TYPE_i128;
            break;
        case 'u':
            if      (is_word(view.source+1, "int"))  return PRIMITIVE_TYPE_number;
            else if (is_word(view.source+1, "8"))    return PRIMITIVE_TYPE_u8;
            else if (is_word(view.source+1, "16"))   return PRIMITIVE_TYPE_u16;
            else if (is_word(view.source+1, "32"))   return PRIMITIVE_TYPE_u32;
            else if (is_word(view.source+1, "64"))   return PRIMITIVE_TYPE_u64;
            else if (is_word(view.source+1, "128"))  return PRIMITIVE_TYPE_u128;
            break;
        case 'f':
            if      (is_word(view.source+1, "loat"))  return PRIMITIVE_TYPE_float;
            else if (is_word(view.source+1, "16"))    return PRIMITIVE_TYPE_f16;
            else if (is_word(view.source+1, "32"))    return PRIMITIVE_TYPE_f32;
            else if (is_word(view.source+1, "64"))    return PRIMITIVE_TYPE_f64;
            else if (is_word(view.source+1, "128"))   return PRIMITIVE_TYPE_f128;
            break;
        case 'b':
            if (is_word(view.source+1, "ool")) return PRIMITIVE_TYPE_bool; else break;
        case 'r':
            if (is_word(view.source+1, "une")) return PRIMITIVE_TYPE_rune; else break;
        case '"': return PRIMITIVE_TYPE_string;
    }
    return PRIMITIVE_TYPE_User;
}


Type make_primitive_type(PrimitiveType primitive) {
    return (Type) { .primitive=primitive, .type=-1 };
}

Type make_user_type(u32 type) {
    return (Type) { .primitive=PRIMITIVE_TYPE_User, .type=type };
}


const char* operation_view(Operation op) {
    switch (op) {
        case None: return "<error>";
        case Add:  return "+";
        case Sub:  return "-";
        case Mul:  return "*";
        case Div:  return "/";
        case Mod:  return "%";
        case Lt:   return "<";
        case Le:   return "<=";
        case Eq:   return "==";
        case Ne:   return "!=";
        case Ge:   return ">=";
        case Gt:   return ">";
        case And:  return "and";
        case Or:   return "or";
        default:
            NOT_IMPLEMENTED;
    }
}


Ast make_ast(AstType type, Location location) {
    Ast ast;
    memcpy(&ast, &location, sizeof(Ast));
    ast.type = type;
    return ast;
}

Location ast_location(Ast ast) {
    Location location;
    memcpy(&location, &ast, sizeof(Location));
    return location;
}

Ast make_ast_error(Location location) {
    Ast ast;
    memcpy(&ast, &location, sizeof(Ast));
    ast.type = AST_ERROR;
    return ast;
}


Ast_Identifier make_identifier(Location location, u16 name, u8 depth, u8 flags) {
    ASSERT(name < 65535);
    return (Ast_Identifier) { .ast=make_ast(AST_IDENTIFIER, location), .name=name, .depth=depth, .flags=flags };
}



Ast_Literal make_literal(Location location, PrimitiveType type, union Value value) {
    return (Ast_Literal) { .ast=make_ast(AST_LITERAL, location), .type=type, .value=value };
}


Ast_BinOp make_bin_op(Location location, Operation op, u32 right) {
    return (Ast_BinOp) { .ast=make_ast(AST_BIN_OP, location), .op=op, .right=right };
}


Ast_FuncCall make_func_call(Location location, Ast_Identifier identifier, u16 arg_count) {
    return (Ast_FuncCall) { .ast=make_ast(AST_FUNC_CALL, location), .name=identifier.name, .scope=identifier.depth, .arg_count=arg_count };
}


Ast_VarAssign make_var_assign(Location location, Ast_Identifier identifier) {
    return (Ast_VarAssign) { .ast=make_ast(AST_VAR_ASSIGN, location), .name=identifier.name, .scope=identifier.depth };
}


Ast_VarDecl make_var_decl(Location location, Ast_Identifier identifier) {
    return (Ast_VarDecl) { .ast=make_ast(AST_VAR_DECL, location), .name=identifier.name, .scope=identifier.depth };
}


Ast_FuncDecl make_func_decl(Location location, Ast_Identifier identifier, u16 param_count, Type return_type) {
    return (Ast_FuncDecl) { .ast=make_ast(AST_FUNC_DECL, location), .name=identifier.name, .scope=identifier.depth, .param_count=param_count, .return_type=return_type.type, .return_primitive=return_type.primitive };
}


Ast_ReturnStmt make_return_stmt(Location location) {
    return (Ast_ReturnStmt) { .ast=make_ast(AST_RETURN_STMT, location) };
}


Ast_IfStmt make_if_stmt(Location location, u32 next, u32 end) {
    return (Ast_IfStmt) { .ast=make_ast(AST_IF_STMT, location), .next=next, .end=end };
}


Ast_WhileStmt make_while_stmt(Location location, u32 end) {
    return (Ast_WhileStmt) { .ast=make_ast(AST_WHILE_STMT, location), .end=end };
}


Ast_Block make_block(Location location, u32 stmt_count) {
    return (Ast_Block) { .ast=make_ast(AST_BLOCK, location), .stmt_count=stmt_count };
}


Ast_Module make_module(Location location, const char* name, u32 stmt_count) {
    return (Ast_Module) { .ast=make_ast(AST_MODULE, location), .name=name, .stmt_count=stmt_count };
}
