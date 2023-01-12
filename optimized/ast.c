#include "ast.h"



#define X(x) SLICE(#x),
Slice PrimitiveType_TYPE_NAMES[] = {
        ALL_PRIMITIVES(X)
};
#undef X


#define is_word(s, x) (memcmp(s, x, sizeof(x)-1) == 0 && *((s) + sizeof(x)-1) == '\0')


PrimitiveType get_primitive(Slice view) {
    switch (*view.source) {
        case 'i':
            if      (is_word(view.source+1, "nt"))  return PrimitiveType_int;
            else if (is_word(view.source+1, "8"))   return PrimitiveType_i8;
            else if (is_word(view.source+1, "16"))  return PrimitiveType_i16;
            else if (is_word(view.source+1, "32"))  return PrimitiveType_i32;
            else if (is_word(view.source+1, "64"))  return PrimitiveType_i64;
            else if (is_word(view.source+1, "128")) return PrimitiveType_i128;
            break;
        case 'u':
            if      (is_word(view.source+1, "int"))  return PrimitiveType_number;
            else if (is_word(view.source+1, "8"))    return PrimitiveType_u8;
            else if (is_word(view.source+1, "16"))   return PrimitiveType_u16;
            else if (is_word(view.source+1, "32"))   return PrimitiveType_u32;
            else if (is_word(view.source+1, "64"))   return PrimitiveType_u64;
            else if (is_word(view.source+1, "128"))  return PrimitiveType_u128;
            break;
        case 'f':
            if      (is_word(view.source+1, "loat"))  return PrimitiveType_float;
            else if (is_word(view.source+1, "16"))    return PrimitiveType_f16;
            else if (is_word(view.source+1, "32"))    return PrimitiveType_f32;
            else if (is_word(view.source+1, "64"))    return PrimitiveType_f64;
            else if (is_word(view.source+1, "128"))   return PrimitiveType_f128;
            break;
        case 'b':
            if (is_word(view.source+1, "ool")) return PrimitiveType_bool; else break;
        case 'r':
            if (is_word(view.source+1, "une")) return PrimitiveType_rune; else break;
        case '"': return PrimitiveType_string;
    }
    return PrimitiveType_user_defined;
}


Type make_primitive_type(PrimitiveType primitive) {
    return (Type) { .primitive=primitive, .type=0 };
}

Type make_user_type(u32 type) {
    ASSERT(0 <= type && type < 16777216);  /* 2^24 == 16 777 216 */
    return (Type) { .primitive=PrimitiveType_user_defined, .type=type };
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

Ast_Identifier  make_identifier(Location location, u16 absolute_offset, u16 scope_local_offset, u16 scope_depth) {
    return (Ast_Identifier) {
        .ast=make_ast(AST_IDENTIFIER, location),
        .absolute_offset=absolute_offset,
        .scope_local_offset=scope_local_offset,
        .scope_depth=scope_depth
    };
}



Ast_Literal make_literal(Location location, PrimitiveType type, union Value value) {
    return (Ast_Literal) { .ast=make_ast(AST_LITERAL, location), .type=type, .value=value };
}


Ast_FuncCall make_func_call(Location location, Ast_Identifier identifier, u16 arg_count) {
    return (Ast_FuncCall) { .ast=make_ast(AST_FUNC_CALL, location), .name=identifier.absolute_offset, .arg_count=arg_count };
}


Ast_VarAssign make_var_assign(Location location, Ast_Identifier identifier) {
    return (Ast_VarAssign) { .ast=make_ast(AST_VAR_ASSIGN, location), .name=identifier.absolute_offset };
}


Ast_VarDecl make_var_decl(Location location, Ast_Identifier identifier) {
    return (Ast_VarDecl) { .ast=make_ast(AST_VAR_DECL, location), .name=identifier.absolute_offset };
}


Ast_FuncDecl make_func_decl(Location location, Ast_Identifier identifier, u16 param_count, Type return_type, u16 next) {
    return (Ast_FuncDecl) { .ast=make_ast(AST_FUNC_DECL, location), .name=identifier.absolute_offset, .param_count=param_count, .return_type=return_type, .next=next };
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


Ast_Block make_block(Location location, u32 stmt_count, u32 end) {
    return (Ast_Block) { .ast=make_ast(AST_BLOCK, location), .stmt_count=stmt_count, .end=end };
}


Ast_Module make_module(Location location, const char* name, u32 stmt_count, u32 end) {
    return (Ast_Module) { .ast=make_ast(AST_MODULE, location), .name=name, .stmt_count=stmt_count, .end=end };
}
