#pragma once

#include "preamble.h"
#include "slice.h"
#include "location.h"


typedef enum {
    PRIMITIVE_TYPE_inferred = 0,

    PRIMITIVE_TYPE_number,
    PRIMITIVE_TYPE_u8,
    PRIMITIVE_TYPE_u16,
    PRIMITIVE_TYPE_u32,
    PRIMITIVE_TYPE_rune,
    PRIMITIVE_TYPE_u64,
    PRIMITIVE_TYPE_u128,

    PRIMITIVE_TYPE_int,
    PRIMITIVE_TYPE_bool,
    PRIMITIVE_TYPE_i8,
    // PRIMITIVE_TYPE_char,
    PRIMITIVE_TYPE_i16,
    PRIMITIVE_TYPE_i32,
    PRIMITIVE_TYPE_i64,
    PRIMITIVE_TYPE_i128,

    PRIMITIVE_TYPE_float,
    PRIMITIVE_TYPE_f16,
    PRIMITIVE_TYPE_f32,
    PRIMITIVE_TYPE_f64,
    PRIMITIVE_TYPE_f128,

    PRIMITIVE_TYPE_string,

    PRIMITIVE_TYPE_User,
} PrimitiveType;
STATIC_ASSERT(PRIMITIVE_TYPE_User < 255, primitive_as_u8);
#define PrimitiveType_bit_size 8
PrimitiveType get_primitive(Slice view);


typedef struct {
    u32 type;
    u16 a;
    PrimitiveType primitive : PrimitiveType_bit_size;
    u8 b;
} Type;
STATIC_ASSERT(sizeof(Type) == 8, waste);
Type make_primitive_type(PrimitiveType primitive);
Type make_user_type(u32 type);


typedef enum {
    None,

    Add,
    Sub,
    Mul,
    Div,
    Mod,

    Lt,
    Le,
    Eq,
    Ne,
    Ge,
    Gt,

    And,
    Or,

    OperationCount,
} Operation;
STATIC_ASSERT(OperationCount < 255, operation_as_u8);

const char* operation_view(Operation op);


typedef enum {
    AST_ERROR = 0,

    AST_IDENTIFIER,
    AST_LITERAL,
    AST_BIN_OP,
    AST_FUNC_CALL,
    AST_VAR_ASSIGN,
    AST_VAR_DECL,
    AST_FUNC_DECL,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_BLOCK,
    AST_MODULE,

    AST_INVALID = 0b11111110,
    AST_COUNT,
} AstType;
STATIC_ASSERT(AST_COUNT < 256, operation_as_u8);



typedef struct {
    AstType type : 8;
    u64 offset   : 24;
    u64 row      : 20;
    u64 column   : 12;
} Ast;
STATIC_ASSERT(sizeof(Ast) == 8 && sizeof(Ast) == sizeof(Location), ast_and_location_must_be_the_same);
Ast make_ast(AstType type, Location location);
Location ast_location(Ast ast);
Ast make_ast_error(Location location);


typedef struct {
    Ast ast;
    u32 name;
    u16 depth;
    u16 flags;
} Ast_Identifier;
STATIC_ASSERT(sizeof(Ast_Identifier) == 2*sizeof(Ast), waste);
Ast_Identifier make_identifier(Location location, u16 name, u8 depth, u8 flags);



typedef struct {
    Ast ast;
    union Value {
        i64 int_;
        u64 uint_;
        f64 float_;
        uintptr_t string;
    } value;
    u32 a;
    u16 b;
    u8  c;
    PrimitiveType type : PrimitiveType_bit_size;
} Ast_Literal;
STATIC_ASSERT(sizeof(Ast_Literal) == 3*sizeof(Ast), waste);
Ast_Literal make_literal(Location location, PrimitiveType type, union Value value);


typedef struct {
    Ast ast;
    u32 right;
    u16 b;
    u8  c;
    Operation op : 8;
} Ast_BinOp;
STATIC_ASSERT(sizeof(Ast_BinOp) == 2*sizeof(Ast), waste);
Ast_BinOp make_bin_op(Location location, Operation op, u32 right);


typedef struct {
    Ast ast;
    u32 name;
    u16 scope;
    u16 arg_count;
} Ast_FuncCall;
STATIC_ASSERT(sizeof(Ast_FuncCall) == 2*sizeof(Ast), waste);
Ast_FuncCall make_func_call(Location location, Ast_Identifier identifier, u16 arg_count);


typedef struct {
    Ast ast;
    u32 name;
    u16 scope;
    u16 a;
}  Ast_VarAssign;
STATIC_ASSERT(sizeof(Ast_VarAssign) == 2*sizeof(Ast), waste);
Ast_VarAssign make_var_assign(Location location, Ast_Identifier identifier);


typedef struct {
    Ast ast;
    u32 name;
    u16 scope;
    u16 a;
} Ast_VarDecl;
STATIC_ASSERT(sizeof(Ast_VarDecl) == 2*sizeof(Ast), waste);
Ast_VarDecl make_var_decl(Location location, Ast_Identifier identifier);


typedef struct {
    Ast ast;
    u32 name;
    u16 scope;
    u16 param_count;
    u32 return_type;
    PrimitiveType return_primitive : PrimitiveType_bit_size;
    u8  a;
    u16 b;
} Ast_FuncDecl;
STATIC_ASSERT(sizeof(Ast_FuncDecl) == 3*sizeof(Ast), waste);
Ast_FuncDecl make_func_decl(Location location, Ast_Identifier identifier, u16 param_count, Type return_type);


typedef struct {
    Ast ast;
} Ast_ReturnStmt;
Ast_ReturnStmt make_return_stmt(Location location);


typedef struct {
    Ast ast;
    u32 next;
    u32 end;
} Ast_IfStmt;
Ast_IfStmt make_if_stmt(Location location, u32 next, u32 end);


typedef struct {
    Ast ast;
    u32 end;
} Ast_WhileStmt;
Ast_WhileStmt make_while_stmt(Location location, u32 end);


typedef struct {
    Ast ast;
    u32 stmt_count;
} Ast_Block;
Ast_Block make_block(Location location, u32 stmt_count);


typedef struct {
    Ast ast;
    const char* name;
    u32 stmt_count;
} Ast_Module;
Ast_Module make_module(Location location, const char* name, u32 stmt_count);

