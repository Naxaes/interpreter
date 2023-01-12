#pragma once

#include "ast.h"
#include "slice.h"
#include "location.h"
#include "table.h"
#include "array.h"
#include "tokenizer.h"


typedef enum {
    NO_ERROR = 0,
    INTERNAL_ERROR,

    COMPILE_ERROR_READING_VARIABLE_IN_OWN_INITIALIZER,
    COMPILE_ERROR_BEGIN = COMPILE_ERROR_READING_VARIABLE_IN_OWN_INITIALIZER,
    COMPILE_ERROR_EXPECTED_PREFIX_TOKEN,
    COMPILE_ERROR_EXPECTED_INFIX_TOKEN,
    COMPILE_ERROR_DECLARED_VARIABLE_TWICE,
    COMPILE_ERROR_TRYING_TO_RETURN_FROM_SCRIPT,
    COMPILE_ERROR_TOO_MANY_PARAMETERS,
    COMPILE_ERROR_TOO_MANY_ARGUMENTS,
    COMPILE_ERROR_TOO_MANY_CONSTANTS,
    COMPILE_ERROR_TOO_MANY_LOCAL_VARIABLES,
    COMPILE_ERROR_LOOP_BODY_TO_LARGE,
    COMPILE_ERROR_TOO_LARGE_JUMP,
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_EXPRESSION,
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_STMT,
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_RETURN,
    COMPILE_ERROR_EXPECTED_FUNCTION_NAME,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME,
    COMPILE_ERROR_EXPECTED_PARAMETER_NAME,
    COMPILE_ERROR_EXPECTED_VAR_DECL_NAME,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_PARAMETER,
    COMPILE_ERROR_EXPECTED_BRACE_BEFORE_BODY,
    COMPILE_ERROR_EXPECTED_BRACE_AFTER_BLOCK,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_FOR,
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_LOOP_COND,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_FOR_CLAUSE,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_WHILE,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_COND,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_IF,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_ARGS,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_GROUPING,
    COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_DECL,
    COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_ASSIGN,
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_DECL,
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_ASSIGN,
    COMPILE_ERROR_END = COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_ASSIGN,
} ErrorCode;



typedef struct {
    ErrorCode   code;
    Location    start;
    int         count;
    Slice       arg;
    const char* path;
    const char* source;
    Slice function;
} Error;

extern const char* ERROR_MESSAGE[];
void print_error(Error error);


declare_table(Ast_Identifier, variable)
declare_table(Type, type)
declare_table(u32, u32)

declare_dynarray(Ast_Identifier)
declare_dynarray(Slice)


#define PARSER_MAX_ERRORS 32
typedef struct {
    const char* path;
    const char* source;
    Location    location;

    Array_Token tokens;
    u32 token_it;

    u32 depth;

//    Table_ast declarations;

    Table_type types;

    // An lookup table of unique identifier names.
    Table_variable variable_cache;
    // An array of unique identifier names. Identical names
    // in different scopes are different identifiers.
    DynArray_Slice variables;
    // Used for the local offset of the identifier.
    u32 name_count_for_current_depth;

    // An array of unique string literals.
    Table_u32      string_map;
    DynArray_Slice strings;


    StackAllocator buffer;
    StackAllocator look_ahead_buffer;

    Error errors[PARSER_MAX_ERRORS];
    int   error_count;
    bool  in_panic_mode;
} Parser;

Parser make_parser(const char* source,  const char* path, Array_Token tokens, StackAllocator buffer);


Ast* expression_start(Parser* parser);
Ast* expression(Parser* parser);
Ast* statement(Parser* parser);
Ast_Block* block(Parser* parser);
Ast_VarAssign* var_assign(Parser* parser);
Ast_VarDecl* var_decl(Parser* parser);
Ast_FuncDecl* func_decl(Parser* parser);
Ast_IfStmt* if_stmt(Parser* parser);
Ast_WhileStmt* while_stmt(Parser* parser);
//Ast_ForStmt* for_stmt(Parser* parser);
Ast_ReturnStmt* return_stmt(Parser* parser);
Ast_Module* module(Parser* parser, const char* name);


Ast* visit(Ast* ast, Parser* parser, int indent);



