#include "parser.h"


const char* ERROR_MESSAGE[] = {
        [COMPILE_ERROR_READING_VARIABLE_IN_OWN_INITIALIZER]      = "Can't read local variable '%.*s' in its own initializer",
        [COMPILE_ERROR_EXPECTED_PREFIX_TOKEN]                    = "Expected literal, unary operation or parenthesis, but got '%.*s'",
        [COMPILE_ERROR_EXPECTED_INFIX_TOKEN]                     = "Expected expression, but got '%.*s'",
        [COMPILE_ERROR_DECLARED_VARIABLE_TWICE]                  = "Already a variable with name '%.*s' in this scope",
        [COMPILE_ERROR_TRYING_TO_RETURN_FROM_SCRIPT]             = "Can't return from top-level code",
        [COMPILE_ERROR_TOO_MANY_PARAMETERS]                      = "Can't have more than 255 parameters",
        [COMPILE_ERROR_TOO_MANY_ARGUMENTS]                       = "Can't have more than 255 arguments",
        [COMPILE_ERROR_TOO_MANY_CONSTANTS]                       = "Too many constants in one chunk",
        [COMPILE_ERROR_TOO_MANY_LOCAL_VARIABLES]                 = "Too many local variables in function",
        [COMPILE_ERROR_LOOP_BODY_TO_LARGE]                       = "Loop body too large",
        [COMPILE_ERROR_TOO_LARGE_JUMP]                           = "Too much code to jump over",
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_EXPRESSION]  = "Expected ';' after expression, but got '%.*s'",
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_STMT]        = "Expected ';' after statement, but got '%.*s'",
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_RETURN]      = "Expected ';' after return expression, but got '%.*s'",
        [COMPILE_ERROR_EXPECTED_FUNCTION_NAME]                   = "Expected function name",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME]      = "Expected '(' after function name",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_GROUPING]           = "Expected ')' after starting expression with '('",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_PARAMETER]          = "Expected ')' after parameters",
        [COMPILE_ERROR_EXPECTED_PARAMETER_NAME]                  = "Expected parameter name",
        [COMPILE_ERROR_EXPECTED_VAR_DECL_NAME]                   = "Expected variable name",
        [COMPILE_ERROR_EXPECTED_BRACE_BEFORE_BODY]               = "Expected '{' before function body",
        [COMPILE_ERROR_EXPECTED_BRACE_AFTER_BLOCK]               = "Expected '}' after block",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_FOR]                = "Expected '(' after 'for'",
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_LOOP_COND]   = "Expected ';' after loop condition",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_FOR_CLAUSE]         = "Expected ')' after for clauses",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_WHILE]              = "Expected '(' after 'while'",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_COND]               = "Expected ')' after condition",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_ARGS]               = "Expected ')' after arguments",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_IF]                 = "Expected '(' after 'if'",
        [COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_DECL]            = "Must initialize variable! Expected '=' after variable declaration",
        [COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_ASSIGN]          = "Expected '=' after variable assignment",
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_DECL]    = "Expected ';' after variable declaration",
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_ASSIGN]  = "Expected ';' after variable assignment",
};



static void synchronize(Parser* parser);



void print_error(Error error) {
    Slice line_before = previous_line(error.source, error.start.offset);
    Slice line_on     = current_line(error.source,  error.start.offset);
    Slice line_after  = next_line(error.source,     error.start.offset);

    char arrow_buffer[1024] = { 0 };
    int n = (error.start.column-1 < 1022) ? error.start.column-1 : 1022;
    for (int j = 0; j < n; ++j) {
        arrow_buffer[j] = '-';  // (i % 5 == 4) ? '*' : '-';
    }
    int m = (n+error.count < 1023) ? n+error.count : 1023;
    for (int j = n; j < m; ++j) {
        arrow_buffer[j] = '^';  // (i % 5 == 4) ? '*' : '-';
    }
    arrow_buffer[m] = '\0';

    fprintf(
        stderr, "[PARSER] Error in '%.*s' at %s:%d:%d:\n    ",
        error.function.count, error.function.source,
        error.path,
        error.start.row, error.start.column
    );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wformat-security"
    if (!slice_is_empty(error.arg)) {
        fprintf(stderr, ERROR_MESSAGE[error.code], error.arg.count, error.arg.source);
    } else {
        fprintf(stderr, ERROR_MESSAGE[error.code]);
    }
#pragma clang diagnostic pop

    fprintf(stderr, ".\n");
    if (line_before.count > 0) {
        fprintf(stderr, "    %-4d| %.*s\n", error.start.row-1, line_before.count, line_before.source);
    }
    fprintf(stderr,
            "    %-4d| %.*s\n"
            "        |-%s\n",
            error.start.row, line_on.count, line_on.source,
            arrow_buffer
    );

    if (line_after.count > 0) {
        fprintf(stderr, "    %-4d| %.*s\n", error.start.row+1, line_after.count, line_after.source);
    }
}


void store_error(Parser* parser, Location start, ErrorCode code, Token token) {
    ASSERT(code != INTERNAL_ERROR);
    if (parser->in_panic_mode || parser->error_count >= PARSER_MAX_ERRORS) {
        return;
    }
    parser->in_panic_mode = true;

    Location location = token_location(token);

    // @TODO: Calculate columns correctly (utf8 identifiers).
    Slice arg = token_view(parser->source, token);
    int count = location.offset + arg.count - start.offset;

    ASSERT(count > 0);

    parser->errors[parser->error_count++] = (Error) {
            .code=code,
            .start=start,
            .count=count,
            .arg=arg,
            .source=parser->source,
            .path=parser->path,
    };
}



Parser make_parser(const char* source, const char* path, Array_Token tokens, StackAllocator buffer) {
    return (Parser) {
            .path              = path,
            .source            = source,
            .location          = make_location(0, 1, 0),
            .tokens            = tokens,
            .token_it          = 0,
            .declarations      = table_ast_make(),
            .variables         = table_variable_make(),
            .types             = table_type_make(),
            .string_map        = table_u32_make(),
            .names             = make_array_Slice(malloc(1024 * sizeof(Slice)), 1024),
            .name_count        = 0,
            .strings           = make_array_Slice(malloc(1024 * sizeof(Slice)), 1024),
            .string_count      = 0,
            .scope_depth       = 0,
            .buffer            = buffer,
            .look_ahead_buffer = make_stack(malloc(4096), 4096),
            .error_count       = 0,
            .in_panic_mode     = 0,
    };
}

// @TODO: Add alignment also.
#define prepend(parser, node, snapshot) do { \
    ASSERTF((int)((parser)->look_ahead_buffer.ptr + sizeof(node)) < (int)(parser)->look_ahead_buffer.capacity, "Buffer overflow");      \
    memmove((snapshot).data+sizeof(node)+(snapshot).ptr, (snapshot).data+(snapshot).ptr, (parser)->look_ahead_buffer.ptr);  \
    memcpy((snapshot).data+(snapshot).ptr, &(node), sizeof(node));                            \
    (parser)->look_ahead_buffer.ptr += sizeof(node);                         \
} while(0)

#define copy_over(parser) do { \
    allocator_push_raw(&(parser)->buffer, (parser)->look_ahead_buffer.ptr, 1, (parser)->look_ahead_buffer.data); \
    (parser)->look_ahead_buffer.ptr = 0;  \
} while(0)


Token peek_token(Parser* parser) {
    Token token = *array_Token_get(&parser->tokens, parser->token_it);
    return token;
}

Token peek_past_next_token(Parser* parser) {
    if (parser->token_it + 1 < parser->tokens.count)
        return *array_Token_get(&parser->tokens, parser->token_it+1);
    else
        return *array_Token_get(&parser->tokens, parser->token_it);
}

Token next_token(Parser* parser) {
    if (parser->token_it + 1 < parser->tokens.count) {
        Token token = *array_Token_get(&parser->tokens, parser->token_it++);
        Token next  = *array_Token_get(&parser->tokens, parser->token_it);
        parser->location = token_location(next);
        return token;
    } else {
        return *array_Token_get(&parser->tokens, parser->token_it);
    }
}

Token consume(Parser* parser, TokenType type, ErrorCode code) {
    Token token = peek_token(parser);
    if (token.type != type) {
        /* Error */
        store_error(parser, token_location(token), code, token);
    }
    return next_token(parser);
}

Token consume_range(Parser* parser, TokenType begin, TokenType end, ErrorCode code) {
    Token token = next_token(parser);
    if (!(begin <= token.type && token.type <= end)) {
        /* Error */
        store_error(parser, token_location(token), code, token);
    }
    return token;
}


Ast_Identifier identifier(Parser* parser, ErrorCode code) {
    Token token = consume(parser, TOKEN_IDENTIFIER, code);
    if (parser->in_panic_mode)
        return (Ast_Identifier) { 0 };

    Slice view = token_view(parser->source, token);
    Ast_Identifier previous;
    if (table_variable_get(&parser->variables, view, &previous)) {
        if (previous.depth != parser->scope_depth) {
            previous.depth = parser->scope_depth;
            previous.flags = 0;
        }
        return previous;
    } else {
        Ast_Identifier node = make_identifier(
                token_location(token),
                (u16) parser->name_count,
                parser->scope_depth,
                0
        );
        array_Slice_set(&parser->names, parser->name_count++, view);
        ASSERT(table_variable_add(&parser->variables, view, node));
        return node;
    }
}


/* @NOTE: Not an ast! */
Type type(Parser* parser, ErrorCode code) {
    Token token = consume(parser, TOKEN_IDENTIFIER, code);
    if (parser->in_panic_mode)
        return (Type) { 0 };

    Slice view = token_view(parser->source, token);
    Type previous;
    if (table_type_get(&parser->types, view, &previous)) {
        return previous;
    } else {
        PrimitiveType primitive = get_primitive(view);

        if (primitive != PRIMITIVE_TYPE_User) {
            Type node = make_primitive_type(primitive);
            return node;
        } else {
            Type node = make_user_type(parser->name_count);
            array_Slice_set(&parser->names, parser->name_count++, view);
            ASSERT(table_type_add(&parser->types, view, node));
            return node;
        }
    }
}



Ast_Literal literal(Parser* parser, ErrorCode code) {
    Token token = consume_range(parser, TOKEN_LITERAL_BEGIN, TOKEN_LITERAL_END, code);
    if (parser->in_panic_mode)
        return (Ast_Literal) { 0 };

    Slice view = token_view(parser->source, token);
    union Value value;
    PrimitiveType type;
    switch (token.type) {
        case TOKEN_I64: {
            value = (union Value) { .int_ = strtoll(view.source,  NULL, 10) };
            type = PRIMITIVE_TYPE_i64;
            break;
        }
        case TOKEN_U64: {
            value = (union Value) { .uint_ = strtoull(view.source, NULL, 10) };
            type = PRIMITIVE_TYPE_u64;
            break;
        }
        case TOKEN_F64: {
            value = (union Value) { .float_ = strtod(view.source,   NULL) };
            type = PRIMITIVE_TYPE_f64;
            break;
        }
        case TOKEN_STRING: {
            u32 previous;
            if (table_u32_get(&parser->string_map, view, &previous)) {
                value = (union Value) { .string = previous };
            } else {
                int index = parser->string_count++;
                value = (union Value) { .string = index };
                ASSERT(table_u32_add(&parser->string_map, view, index));
                array_Slice_set(&parser->strings, index, view);
            }
            type = PRIMITIVE_TYPE_string;
            break;
        }
        default:
            PANIC("Should not happen! '%d'", token.type);
    }

    Ast_Literal node = make_literal(
            token_location(token),
            type,
            value
    );

    return node;
}


Ast_FuncCall* func_call(Parser* parser, StackAllocator* buffer) {
    Ast_Identifier name = identifier(parser, COMPILE_ERROR_EXPECTED_FUNCTION_NAME);
    Ast_FuncCall   n    = make_func_call(parser->location, name, 0);
    Ast_FuncCall*  node = stack_push(buffer, Ast_FuncCall, n);

    consume(parser, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME);

    u32 arg_count = 0;

    Token token = peek_token(parser);
    while (token.type != TOKEN_RIGHT_PAREN && token.type != TOKEN_EOF) {
        expression_start(parser);
        ++arg_count;
        if (peek_token(parser).type == TOKEN_COMMA)
            token = next_token(parser);
        else
            break;
    }

    ASSERT(arg_count < 255);
    node->arg_count = (u8) arg_count;

    consume(parser, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_ARGS);

    return node;
}


Ast* factor(Parser* parser, StackAllocator* buffer) {
    Token token = peek_token(parser);
    switch (token.type) {
        case TOKEN_LEFT_PAREN: {
            token = next_token(parser);
            Ast* node = expression(parser);
            consume(parser, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_GROUPING);
            return node;
        }

        case TOKEN_IDENTIFIER: {
            if (peek_past_next_token(parser).type == TOKEN_LEFT_PAREN) {
                return (Ast*) func_call(parser, buffer);
            } else {
                Ast_Identifier n = identifier(parser, INTERNAL_ERROR);
                return stack_push(buffer, Ast_Identifier, n);
            }
        }

        case TOKEN_F64:
        case TOKEN_I64:
        case TOKEN_STRING: {
            Ast_Literal n = literal(parser, INTERNAL_ERROR);
            return stack_push(buffer, Ast_Literal, n);
        }
        default:
            return NULL;
    }
}
Ast* product(Parser* parser) {
    StackAllocator snapshot = parser->look_ahead_buffer;
    Ast* node = factor(parser, &parser->look_ahead_buffer);

    while (true) {
        Token token = peek_token(parser);
        Operation op;
        switch (token.type) {
            case TOKEN_STAR:    op = Mul; break;
            case TOKEN_SLASH:   op = Div; break;
            case TOKEN_PERCENT: op = Mod; break;
            default: return node;
        }
        next_token(parser);

        Ast* right = factor(parser, &parser->look_ahead_buffer);
        int offset = (int) ((u8*) right - (u8*) node);

        Ast_BinOp bin_op = make_bin_op(
            token_location(token),
            op,
            offset
        );
        prepend(parser, bin_op, snapshot);
    }
}
Ast* term(Parser* parser) {
    StackAllocator snapshot = parser->look_ahead_buffer;
    Ast* node = product(parser);

    while (true) {
        Token token = peek_token(parser);
        Operation op;
        switch (token.type) {
            case TOKEN_PLUS:    op = Add; break;
            case TOKEN_MINUS:   op = Sub; break;
            default: return node;
        }
        next_token(parser);

        Ast* right = product(parser);
        int offset = (int) ((u8*) right - (u8*) node);

        Ast_BinOp bin_op = make_bin_op(
            token_location(token),
            op,
            offset
        );
        prepend(parser, bin_op, snapshot);
    }
}
Ast* relation(Parser* parser) {
    StackAllocator snapshot = parser->look_ahead_buffer;
    Ast* node = term(parser);

    while (true) {
        Token token = peek_token(parser);
        Operation op;
        switch (token.type) {
            case TOKEN_LESS:          op = Lt; break;
            case TOKEN_LESS_EQUAL:    op = Le; break;
            case TOKEN_EQUAL_EQUAL:   op = Eq; break;
            case TOKEN_BANG_EQUAL:    op = Ne; break;
            case TOKEN_GREATER_EQUAL: op = Ge; break;
            case TOKEN_GREATER:       op = Gt; break;
            default: return node;
        }
        next_token(parser);

        Ast* right = term(parser);
        int offset = (int) ((u8*) right - (u8*) node);

        Ast_BinOp bin_op = make_bin_op(
            token_location(token),
            op,
            offset
        );
        prepend(parser, bin_op, snapshot);
    }
}
Ast* and(Parser* parser) {
    StackAllocator snapshot = parser->look_ahead_buffer;
    Ast* node = relation(parser);

    while (true) {
        Token token = peek_token(parser);
        Operation op;
        switch (token.type) {
            case TOKEN_AND: op = And; break;
            default: return node;
        }
        next_token(parser);

        Ast* right = relation(parser);
        int offset = (int) ((u8*) right - (u8*) node);

        Ast_BinOp bin_op = make_bin_op(
            token_location(token),
            op,
            offset
        );
        prepend(parser, bin_op, snapshot);
    }
}
Ast* or(Parser* parser) {
    StackAllocator snapshot = parser->look_ahead_buffer;
    Ast* node = and(parser);

    while (true) {
        Token token = peek_token(parser);
        Operation op;
        switch (token.type) {
            case TOKEN_OR: op = Or; break;
            default: return node;
        }
        next_token(parser);

        Ast* right = and(parser);
        int offset = (int) ((u8*) right - (u8*) node);

        Ast_BinOp bin_op = make_bin_op(
            token_location(token),
            op,
            offset
        );
        prepend(parser, bin_op, snapshot);
    }
}
Ast* expression(Parser* parser) {
    StackAllocator snapshot = parser->look_ahead_buffer;
    Ast* node = or(parser);
    return node;
}
Ast* expression_start(Parser* parser) {
    Ast* node = or(parser);
    if (parser->look_ahead_buffer.ptr != 0) {
        Ast* top = stack_top(parser->buffer, Ast);
        copy_over(parser);
        return top;
    }
    return node;
}




Ast_Block* block(Parser* parser) {
    Token token = consume(parser, TOKEN_LEFT_BRACE, COMPILE_ERROR_EXPECTED_BRACE_BEFORE_BODY);

    Ast_Block  n     = make_block(token_location(token), 0);
    Ast_Block* block = stack_push(&parser->buffer, Ast_Block, n);

    u32 stmt_count = 0;
    token = peek_token(parser);
    while (token.type != TOKEN_RIGHT_BRACE && token.type != TOKEN_EOF) {
        statement(parser);
        token = peek_token(parser);
        ++stmt_count;
    }

    consume(parser, TOKEN_RIGHT_BRACE, COMPILE_ERROR_EXPECTED_BRACE_AFTER_BLOCK);

    block->stmt_count = stmt_count;

    return block;
}


Ast_VarAssign* var_assign(Parser* parser) {
    Ast_Identifier name = identifier(parser, INTERNAL_ERROR);

    Ast_VarAssign  n    = make_var_assign(ast_location(name.ast), name);
    Ast_VarAssign* node = stack_push(&parser->buffer, Ast_VarAssign, n);

    consume(parser, TOKEN_EQUAL, COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_ASSIGN);
    expression_start(parser);
    consume(parser, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_ASSIGN);

    return node;
}

Ast_VarDecl* var_decl(Parser* parser) {
    Token var = consume(parser, TOKEN_VAR, INTERNAL_ERROR);
    Ast_Identifier name = identifier(parser, COMPILE_ERROR_EXPECTED_VAR_DECL_NAME);

    Ast_VarDecl  n    = make_var_decl(token_location(var), name);
    Ast_VarDecl* node = stack_push(&parser->buffer, Ast_VarDecl, n);

    Type type = make_primitive_type(PRIMITIVE_TYPE_inferred);

    consume(parser, TOKEN_EQUAL, COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_DECL);
    expression_start(parser);
    consume(parser, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_DECL);

    return node;
}

Ast_FuncDecl* func_decl(Parser* parser) {
    Token var = consume(parser, TOKEN_FUN, INTERNAL_ERROR);

    Ast_Identifier name = identifier(parser, COMPILE_ERROR_EXPECTED_FUNCTION_NAME);
    Type type = make_primitive_type(PRIMITIVE_TYPE_inferred);

    Ast_FuncDecl  n    = make_func_decl(token_location(var), name, 0, type);
    Ast_FuncDecl* node = stack_push(&parser->buffer, Ast_FuncDecl, n);

//    if (peek_token(parser).type == TOKEN_ARROW) {
//        consume(parser, TOKEN_ARROW);
//    }

    consume(parser, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME);

    u32 param_count = 0;

    Token token = peek_token(parser);
    while (token.type != TOKEN_RIGHT_PAREN && token.type != TOKEN_EOF) {
        Ast_Identifier param = identifier(parser, COMPILE_ERROR_EXPECTED_PARAMETER_NAME);
        stack_push(&parser->buffer, Ast_Identifier, param);
        ++param_count;
        if (peek_token(parser).type == TOKEN_COMMA)
            token = next_token(parser);
        else
            break;
    }

    ASSERT(param_count < 65356);
    node->param_count = (u16) param_count;

    consume(parser, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_PARAMETER);

    block(parser);

    return node;
}

Ast_IfStmt* if_stmt(Parser* parser) {
    Token if_ = consume(parser, TOKEN_IF, INTERNAL_ERROR);

    Ast_IfStmt  n    = make_if_stmt(token_location(if_), 0, 0);
    Ast_IfStmt* node = stack_push(&parser->buffer, Ast_IfStmt, n);

    consume(parser, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_IF);
    expression_start(parser);
    consume(parser, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_EXPRESSION);

    Ast_Block* body = block(parser);

    node->next = parser->buffer.ptr;

    if (peek_token(parser).type == TOKEN_ELSE) {
        next_token(parser);
        if (peek_token(parser).type == TOKEN_IF) {
            if_stmt(parser);
        } else {
            Ast_IfStmt* else_ = stack_push(&parser->buffer, Ast_IfStmt, n);
            block(parser);
            else_->end = else_->next = parser->buffer.ptr;
        }
    }

    node->end = parser->buffer.ptr;
    return node;
}

Ast_WhileStmt* while_stmt(Parser* parser) {
    Token while_ = consume(parser, TOKEN_WHILE, INTERNAL_ERROR);

    Ast_WhileStmt  n    = make_while_stmt(token_location(while_), 0);
    Ast_WhileStmt* node = stack_push(&parser->buffer, Ast_WhileStmt, n);

    consume(parser, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_WHILE);
    expression_start(parser);
    consume(parser, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_LOOP_COND);

    Ast_Block* body = block(parser);

    node->end = parser->buffer.ptr;
    return node;
}

Ast_ReturnStmt* return_stmt(Parser* parser) {
    if (parser->scope_depth == 0) {
        Token token = peek_token(parser);
        store_error(parser, token_location(token), COMPILE_ERROR_TRYING_TO_RETURN_FROM_SCRIPT, token);
        return NULL;
    }

    Token return_ = consume(parser, TOKEN_RETURN, INTERNAL_ERROR);

    Ast_ReturnStmt  n    = make_return_stmt(token_location(return_));
    Ast_ReturnStmt* node = stack_push(&parser->buffer, Ast_ReturnStmt, n);

    expression_start(parser);

    consume(parser, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_RETURN);
    return node;
}


Ast* statement(Parser* parser) {
    retry:;
    Token token = peek_token(parser);
    switch (token.type) {
        case TOKEN_LEFT_BRACE: {
            return (Ast*) block(parser);
        } case TOKEN_IDENTIFIER: {
            if (peek_past_next_token(parser).type == TOKEN_LEFT_PAREN) {
                Ast* node = (Ast*) func_call(parser, &parser->buffer);
                consume(parser, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_STMT);
                return node;
            } else {
                return (Ast*) var_assign(parser);
            }
        } case TOKEN_VAR: {
            return (Ast*) var_decl(parser);
        } case TOKEN_FUN: {
            return (Ast*) func_decl(parser);
        } case TOKEN_IF: {
            return (Ast*) if_stmt(parser);
        } case TOKEN_WHILE: {
            return (Ast*) while_stmt(parser);
        } case TOKEN_FOR: {
//            return (Ast*) for_stmt(parser);
            NOT_IMPLEMENTED;
            return NULL;
        } case TOKEN_RETURN: {
            return (Ast*) return_stmt(parser);
        } case TOKEN_END_STMT: {
            next_token(parser);
            goto retry;
        } default: {
            Ast* node = expression_start(parser);
            consume(parser, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_EXPRESSION);
            return node;
        }
    }
}



void print_indent(int n) {
    if (n == 0) return;
    for (int i = 0; i < 2*n-2; i += 2) {
        printf("  |  ");
    }
    if (n > 0) {
        printf("  |--");
    }
}


Ast* visit(Ast* ast, Parser* parser, int indent) {
    print_indent(indent);
    printf("[%04d] ", (int) ((u8*)(ast) - parser->buffer.data));
    switch (ast->type) {
        case AST_ERROR: {
            printf("AST_ERROR\n");
            break;
        }
        case AST_IDENTIFIER: {
            Ast_Identifier* n = (Ast_Identifier*) ast;
            Slice view = *array_Slice_get(&parser->names, n->name);
            printf("AST_IDENTIFIER: %.*s (%d)\n", view.count, view.source, n->name);
            return (Ast*)(n+1);
        }
        case AST_LITERAL: {
            Ast_Literal* n = (Ast_Literal*) ast;
            printf("AST_LITERAL: ");
            switch (n->type) {
                case PRIMITIVE_TYPE_inferred:
                case PRIMITIVE_TYPE_number:
                case PRIMITIVE_TYPE_u8:
                case PRIMITIVE_TYPE_u16:
                case PRIMITIVE_TYPE_u32:
                case PRIMITIVE_TYPE_rune:
                case PRIMITIVE_TYPE_u64:
                case PRIMITIVE_TYPE_u128: printf("%llu\n", n->value.uint_); break;
                case PRIMITIVE_TYPE_int:
                case PRIMITIVE_TYPE_bool:
                case PRIMITIVE_TYPE_i8:
                case PRIMITIVE_TYPE_i16:
                case PRIMITIVE_TYPE_i32:
                case PRIMITIVE_TYPE_i64:
                case PRIMITIVE_TYPE_i128: printf("%lld\n", n->value.int_); break;
                case PRIMITIVE_TYPE_float:
                case PRIMITIVE_TYPE_f16:
                case PRIMITIVE_TYPE_f32:
                case PRIMITIVE_TYPE_f64:
                case PRIMITIVE_TYPE_f128: printf("%.2f\n", n->value.float_); break;
                case PRIMITIVE_TYPE_string: {
                    Slice name = *array_Slice_get(&parser->strings, (int) n->value.string);
                    printf("%.*s\n", name.count, name.source);
                    break;
                } case PRIMITIVE_TYPE_User:
                    break;
            }
            return (Ast*)(n+1);
        } case AST_FUNC_CALL: {
            Ast_FuncCall* n = (Ast_FuncCall*) ast;
            Slice view = *array_Slice_get(&parser->names, n->name);
            printf("AST_FUNC_CALL: %.*s (%d)\n", view.count, view.source, n->name);
            Ast* next = (Ast*)(n+1);
            for (int i = 0; i < n->arg_count; ++i) {
                next = visit(next, parser, indent+1);
            }
            return next;
        } case AST_BIN_OP:{
            Ast_BinOp* n = (Ast_BinOp*) ast;
            printf("AST_BIN_OP: %s\n", operation_view(n->op));
            Ast* next = visit((Ast*)(n+1), parser, indent+1);
            next = visit(next, parser, indent+1);
            return next;
        } case AST_VAR_ASSIGN: {
            Ast_VarAssign* n = (Ast_VarAssign*) ast;
            Slice view = *array_Slice_get(&parser->names, n->name);
            printf("AST_VAR_ASSIGN: %.*s (%d)\n", view.count, view.source, n->name);
            Ast* next = visit((Ast*)(n+1), parser, indent+1);
            return next;
        }
        case AST_VAR_DECL: {
            Ast_VarDecl* n = (Ast_VarDecl*) ast;
            Slice view = *array_Slice_get(&parser->names, n->name);
            printf("AST_VAR_DECL: %.*s (%d)\n", view.count, view.source, n->name);
            Ast* next = visit((Ast*)(n+1), parser, indent+1);
            return next;
        }
        case AST_FUNC_DECL: {
            Ast_FuncDecl* n = (Ast_FuncDecl*) ast;
            Slice view = *array_Slice_get(&parser->names, n->name);
            printf("AST_FUNC_DECL: %.*s (%d)\n", view.count, view.source, n->name);
            Ast* next = (Ast*)(n+1);
            for (int i = 0; i < n->param_count; ++i) {
                next = visit(next, parser, indent+1);
            }
            next = visit(next, parser, indent+1);
            return next;
        }
        case AST_RETURN_STMT: {
            Ast_ReturnStmt* n = (Ast_ReturnStmt*) ast;
            printf("AST_RETURN_STMT:\n");
            Ast* next = visit((Ast*) (n+1), parser, indent+1);
            return next;
        }
        case AST_IF_STMT: {
            Ast_IfStmt* n = (Ast_IfStmt*) ast;
            Ast* next = (Ast*) (n+1);
            printf("AST_IF_STMT: next=%d, end=%d\n", n->next, n->end);
            next = visit(next, parser, indent+1);
            next = visit(next, parser, indent+1);
            ast  = next;
            n = (Ast_IfStmt*) ast;

            while (ast->type == AST_IF_STMT && n->next != n->end) {
                print_indent(indent+1);
                printf("[%04d] ", (int) ((u8*)(ast) - parser->buffer.data));
                printf("AST_IF_STMT: next=%d, end=%d\n", n->next, n->end);
                next = visit((Ast*) (n+1), parser, indent+2);
                next = visit(next, parser, indent+2);
                ast  = next;
                n = (Ast_IfStmt*) ast;
            }
            if (ast->type == AST_IF_STMT) {
                print_indent(indent+1);
                printf("[%04d] ", (int) ((u8*)(ast) - parser->buffer.data));
                printf("AST_ELSE_STMT: end=%d\n", n->end);
                next = visit((Ast*) (n+1), parser, indent+2);
            }
            return next;
        }
        case AST_WHILE_STMT: {
            Ast_WhileStmt* n = (Ast_WhileStmt*) ast;
            printf("AST_WHILE_STMT: end=%d\n", n->end);
            Ast* next = visit((Ast*) (n+1), parser, indent+1);
            next = visit(next, parser, indent+1);
            return next;
        }
        case AST_BLOCK: {
            Ast_Block* n = (Ast_Block*) ast;
            printf("AST_BLOCK: stmt_count=%d\n", n->stmt_count);
            Ast* next = (Ast*) (n+1);
            for (u32 i = 0; i < n->stmt_count; ++i) {
                next = visit(next, parser, indent+1);
            }
            return next;
        }
        case AST_MODULE: {
            Ast_Module* n = (Ast_Module*) ast;
            printf("AST_MODULE: name=%s, stmt_count=%d\n", n->name, n->stmt_count);
            Ast* next = (Ast*) (n + 1);
            for (u32 i = 0; i < n->stmt_count; ++i) {
                next = visit(next, parser, indent+1);
            }
            printf("[%04d] EOF\n", (int) ((u8*)(next) - parser->buffer.data));
        }
        case AST_COUNT: {
            return NULL;
        }
        case AST_INVALID: {
            return NULL;
        }
    }
    return NULL;
}


Ast_Module* module(Parser* parser, const char* name) {
    Token token = peek_token(parser);

    Ast_Module  n = make_module(parser->location, name, 0);
    Ast_Module* node = stack_push(&parser->buffer, Ast_Module, n);

    u32 stmt_count = 0;
    while (token.type != TOKEN_EOF) {
        Ast* stmt = statement(parser);
        if (parser->error_count > 0) {
            synchronize(parser);
            if (parser->error_count == PARSER_MAX_ERRORS)
                return NULL;
        }

        ++stmt_count;
        token = peek_token(parser);
    }

    node->stmt_count = stmt_count;

    return node;
}



static void synchronize(Parser* parser) {
    parser->in_panic_mode = false;
    parser->look_ahead_buffer.ptr = 0;

    // @NOTE: Skip over all tokens until we hit the start
    //  of a new statement.
    while (peek_past_next_token(parser).type != TOKEN_EOF) {
        if (peek_token(parser).type == TOKEN_END_STMT)
            return;

        switch (peek_past_next_token(parser).type) {
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
                return;
            default:
                next_token(parser);
        }
    }
}



