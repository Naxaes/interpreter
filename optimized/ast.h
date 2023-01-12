#pragma once

#include "c-preamble/nax_preamble.h"
#include "slice.h"
#include "location.h"


#define ALL_PRIMITIVES(X) \
    X(user_defined)       \
    X(inferred)           \
    X(number)             \
    X(u8)                 \
    X(u16)                \
    X(u32)                \
    X(rune)               \
    X(u64)                \
    X(u128)               \
    X(int)                \
    X(bool)               \
    X(i8)                 \
    X(char)               \
    X(i16)                \
    X(i32)                \
    X(i64)                \
    X(i128)               \
    X(float)              \
    X(f16)                \
    X(f32)                \
    X(f64)                \
    X(f128)               \
    X(object)             \
    X(string)             \
    X(function)           \
    X(null)               \

#define X(x) PrimitiveType_##x,
typedef enum {
    ALL_PRIMITIVES(X)
    PrimitiveTypeCount
} PrimitiveType;
#undef X

extern Slice PrimitiveType_TYPE_NAMES[PrimitiveTypeCount];


static_assert(PrimitiveTypeCount < 255, "primitive_as_u8");
#define PrimitiveType_bit_size 8
PrimitiveType get_primitive(Slice view);


typedef struct {
    u32 type : 24;
    PrimitiveType primitive : PrimitiveType_bit_size;
} Type;
static_assert(sizeof(Type) == 4, "waste");
Type make_primitive_type(PrimitiveType primitive);
Type make_user_type(u32 type);
static inline bool is_type(Type a, Type b) {
    return a.type == b.type && a.primitive == b.primitive;
}


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
static_assert(OperationCount < 255, "operation_as_u8");

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
static_assert(AST_COUNT < 256, "operation_as_u8");



typedef struct {
    AstType type : 8;
    u64 offset   : 24;
    u64 row      : 20;
    u64 column   : 12;
} Ast;
static_assert(sizeof(Ast) == 8 && sizeof(Ast) == sizeof(Location), "ast_and_location_must_be_the_same");
Ast make_ast(AstType type, Location location);
Location ast_location(Ast ast);


/* An expression with a named value. */
typedef struct {
    Ast ast;
    // An index in parser->names to an unique identifier.
    u16 absolute_offset;
    u16 scope_local_offset;
    u16 scope_depth;
} Ast_Identifier;
static_assert(sizeof(Ast_Identifier) == 2*sizeof(Ast), "waste");
Ast_Identifier make_identifier(Location location, u16 absolute_offset, u16 scope_local_offset, u16 scope_depth);
static inline Ast* ast_identifier_next(Ast_Identifier* node) { return (Ast*) (node+1); }

/* A constant in source */
typedef struct {
    Ast ast;
    union Value {
        i64 int_;
        u64 uint_;
        f64 float_;
        uintptr_t string;
    } value;
    u32 _pad1;
    u16 _pad2;
    u8  _pad3;
    PrimitiveType type : PrimitiveType_bit_size;
} Ast_Literal;
static_assert(sizeof(Ast_Literal) == 3*sizeof(Ast), "waste");
Ast_Literal make_literal(Location location, PrimitiveType type, union Value value);
static inline Ast* ast_literal_next(Ast_Literal* node) { return (Ast*) (node+1); }

typedef struct {
    Ast ast;
    // Offset (in Ast* granularity) to the right node.
    u32 right;
    u16 _pad1;
    PrimitiveType type : PrimitiveType_bit_size;
    Operation op : 8;
} Ast_BinOp;
static_assert(sizeof(Ast_BinOp) == 2*sizeof(Ast), "waste");
static inline Ast_BinOp make_bin_op(Location location, Operation op, Ast* left, Ast* right, PrimitiveType type) {
    intptr_t offset = (intptr_t) (right - left);
    ASSERT(0 <= offset && offset <= 4294967296);
    return (Ast_BinOp) { .ast=make_ast(AST_BIN_OP, location), .op=op, .right=(u32) offset, .type=type };
}
static inline Ast* bin_op_left(Ast_BinOp* bin_op) {
    return (Ast*) (bin_op + 1);
}
static inline Ast* bin_op_right(Ast_BinOp* bin_op) {
    return (Ast*) (bin_op_left(bin_op) + bin_op->right);
}


typedef struct {
    Ast ast;
    // An index in parser->names to an unique identifier.
    u32 name;
    u32 arg_count;
} Ast_FuncCall;
static_assert(sizeof(Ast_FuncCall) == 2*sizeof(Ast), "waste");
Ast_FuncCall make_func_call(Location location, Ast_Identifier identifier, u16 arg_count);


typedef struct {
    Ast ast;
    // An index in parser->names to an unique identifier.
    u32 name;
    u16 _pad1;
}  Ast_VarAssign;
static_assert(sizeof(Ast_VarAssign) == 2*sizeof(Ast), "waste");
Ast_VarAssign make_var_assign(Location location, Ast_Identifier identifier);
Ast* var_assign_expr(Ast_VarAssign* node);

typedef struct {
    Ast ast;
    // An index in parser->names to an unique identifier.
    u32 name;
    u16 _pad1;
} Ast_VarDecl;
static_assert(sizeof(Ast_VarDecl) == 2*sizeof(Ast), "waste");
Ast_VarDecl make_var_decl(Location location, Ast_Identifier identifier);


typedef struct {
    Ast ast;
    // An index in parser->names to an unique identifier.
    u32 name;
    Type return_type;
    u16 param_count;
    u16 next;
    /* Followed by params and then a block. */
} Ast_FuncDecl;
static_assert(sizeof(Ast_FuncDecl) == 3*sizeof(Ast), "waste");
Ast_FuncDecl make_func_decl(Location location, Ast_Identifier identifier, u16 param_count, Type return_type, u16 next);


typedef struct {
    Ast ast;
} Ast_ReturnStmt;
Ast_ReturnStmt make_return_stmt(Location location);
Ast* return_stmt_expr(Ast_ReturnStmt* node);


typedef struct {
    Ast ast;
    // Offset (in Ast* granularity) to the node after the if-block.
    u32 next;
    // Offset (in Ast* granularity) to the node after whole conditional chain.
    u32 end;
} Ast_IfStmt;
Ast_IfStmt make_if_stmt(Location location, u32 next, u32 end);


typedef struct {
    Ast ast;
    // Offset (in Ast* granularity) to the node after the while-block.
    u32 end;
} Ast_WhileStmt;
Ast_WhileStmt make_while_stmt(Location location, u32 end);


typedef struct {
    Ast ast;
    u32 stmt_count;
    // Offset (in Ast* granularity) to the node after the block.
    u32 end;
} Ast_Block;
Ast_Block make_block(Location location, u32 stmt_count, u32 end);


typedef struct {
    Ast ast;
    const char* name;
    u32 stmt_count;
    // Offset (in Ast* granularity) to the node until the end of the module.
    u32 end;
} Ast_Module;
Ast_Module make_module(Location location, const char* name, u32 stmt_count, u32 end);

