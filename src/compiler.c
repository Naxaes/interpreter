#include "compiler.h"
#include "opcodes.h"
#include "object.h"
#include "value.h"


typedef enum {
    PRECEDENCE_NONE,
    PRECEDENCE_ASSIGNMENT,  // =
    PRECEDENCE_OR,          // or
    PRECEDENCE_AND,         // and
    PRECEDENCE_EQUALITY,    // == !=
    PRECEDENCE_COMPARISON,  // < > <= >=
    PRECEDENCE_TERM,        // + -
    PRECEDENCE_FACTOR,      // * /
    PRECEDENCE_UNARY,       // ! -
    PRECEDENCE_CALL,        // . ()
    PRECEDENCE_PRIMARY
} Precedence;


void store_error(Compiler* self, Location start, ErrorCode code, Token token);

static void parse_precedence(Compiler* compiler, Precedence precedence);
static void emit_constant(Compiler* compiler, Value value);
static u8 make_constant(Compiler* compiler, Value value);
static void synchronize(Compiler* self);

static void literal(Compiler* compiler, bool can_assign);
static void num_i64(Compiler* compiler, bool can_assign);
static void num_f64(Compiler* compiler, bool can_assign);
static void string(Compiler* compiler, bool can_assign);
static void variable(Compiler* compiler, bool can_assign);
static void grouping(Compiler* compiler, bool can_assign);
static void unary(Compiler* compiler, bool can_assign);
static void binary(Compiler* compiler, bool can_assign);
static void expression(Compiler* compiler);
static void variable_declaration(Compiler* self);
static void statement(Compiler* compiler);
static void declaration(Compiler* compiler);
static void begin_scope(Compiler* self);
static void end_scope(Compiler* self);
static void block(Compiler* self);
static void declare_variable(Compiler* self);
static void add_local(Compiler* self, Token param);
static bool identifiers_equal(Compiler* self, Token* a, Token* b);
static void if_statement(Compiler* self);
static void call(Compiler* self, bool can_assign);
static void patch_jump(Compiler* self, int jump);
static int emit_jump(Compiler* self, uint8_t instruction);
static void while_statement(Compiler* self);
static void emit_loop(Compiler* self, int start);
static void for_statement(Compiler* self);
static ObjFunction* compiler_end(Compiler* self);
static void function_declaration(Compiler* self);
static void function(Compiler* self, FunctionType type);
static uint8_t argument_list(Compiler* self);
static void return_statement(Compiler* self);


Compiler compiler_make(Parser* parser, FunctionType type) {
    Compiler compiler;
    compiler.function = function_make();
    compiler.type = type;

    memset(compiler.errors, 0, sizeof(compiler.errors));
    compiler.error_count = 0;

    compiler.parser = parser;
    compiler.local_count   = 0;
    compiler.scope_depth   = 0;
    compiler.const_ptr     = 0;
    compiler.had_error     = false;
    compiler.in_panic_mode = false;

    if (type != TYPE_SCRIPT) {
        Slice name = parser_token_repr(compiler.parser, compiler.parser->previous);
        compiler.function->name = string_make(name.source, name.count);
    }


    // @NOTE: The compiler implicitly claims stack slot zero
    //  for the VM’s own internal use
    Local* local = &compiler.locals[compiler.local_count++];
    local->depth = 0;
    local->name  =  token_make_empty();

    return compiler;
}


static void emit_byte(Compiler* self, u8 byte) {
    chunk_write(&self->function->chunk, byte, token_location(self->parser->previous));
}
static void emit_bytes(Compiler* self, u8 byte1, u8 byte2) {
    emit_byte(self, byte1);
    emit_byte(self, byte2);
}


static inline Token current_token(Compiler* self) {
    return self->parser->current;
}

static inline Token previous_token(Compiler* self) {
    return self->parser->previous;
}

static inline bool check(Compiler* self, TokenType type) {
    return self->parser->current.type == type;
}

static inline bool match(Compiler* self, TokenType type) {
    if (!check(self, type))
        return false;
    advance(self->parser);
    return true;
}

static inline void consume(Compiler* self, TokenType type, ErrorCode code) {
    if (!match(self, type)) {
        Token token = self->parser->current;
        store_error(self, token.location, code, token);
    }
}

void store_error(Compiler* self, Location location, ErrorCode code, Token token) {
    if (self->in_panic_mode || self->error_count >= COMPILER_MAX_ERRORS)
        return;

    self->in_panic_mode = true;
    self->had_error     = true;

    self->errors[self->error_count++] = (Error) {
        .code=code,
        .location=location,
        .arg=parser_token_repr(self->parser, token),
        .source=self->parser->source,
        .path=self->parser->path,
        .function = (self->function->name) ? (Slice) { .source= self->function->name->data, .count=self->function->name->size } : SLICE("script")
    };
}

typedef void (*ParseFn)(Compiler* compiler, bool can_assign);
typedef struct {
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
} ParseRule;

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {   grouping, call,   PRECEDENCE_CALL    },
        [TOKEN_RIGHT_PAREN]   = {   NULL,     NULL,   PRECEDENCE_NONE    },
        [TOKEN_MINUS]         = {   unary,    binary, PRECEDENCE_TERM    },
        [TOKEN_PLUS]          = {   NULL,     binary, PRECEDENCE_TERM    },
        [TOKEN_SLASH]         = {   NULL,     binary, PRECEDENCE_FACTOR  },
        [TOKEN_STAR]          = {   NULL,     binary, PRECEDENCE_FACTOR  },
        [TOKEN_I64]           = {   num_i64,  NULL,   PRECEDENCE_NONE    },
        [TOKEN_F64]           = {   num_f64,  NULL,   PRECEDENCE_NONE    },
        [TOKEN_FALSE]         = {   literal,  NULL,   PRECEDENCE_NONE    },
        [TOKEN_TRUE]          = {   literal,  NULL,   PRECEDENCE_NONE    },
        [TOKEN_IDENTIFIER]    = {   variable, NULL,   PRECEDENCE_NONE    },

        [TOKEN_BANG]          = {   unary,    NULL,   PRECEDENCE_UNARY   },

        [TOKEN_BANG_EQUAL]    = {   NULL,     binary,  PRECEDENCE_EQUALITY    },
        [TOKEN_EQUAL_EQUAL]   = {   NULL,     binary,  PRECEDENCE_EQUALITY    },
        [TOKEN_GREATER]       = {   NULL,     binary,  PRECEDENCE_COMPARISON  },
        [TOKEN_GREATER_EQUAL] = {   NULL,     binary,  PRECEDENCE_COMPARISON  },
        [TOKEN_LESS]          = {   NULL,     binary,  PRECEDENCE_COMPARISON  },
        [TOKEN_LESS_EQUAL]    = {   NULL,     binary,  PRECEDENCE_COMPARISON  },

        [TOKEN_AND]           = {   NULL,     binary, PRECEDENCE_AND     },
        [TOKEN_OR]            = {   NULL,     binary, PRECEDENCE_OR      },

        [TOKEN_STRING]        = {   string,   NULL,   PRECEDENCE_NONE    },

        [TOKEN_RETURN]        = {   NULL,     NULL,   PRECEDENCE_NONE    },
        [TOKEN_END_STMT]      = {   NULL,     NULL,   PRECEDENCE_NONE    },
        [TOKEN_ERROR]         = {   NULL,     NULL,   PRECEDENCE_NONE    },
        [TOKEN_EOF]           = {   NULL,     NULL,   PRECEDENCE_NONE    },
};

static ParseRule* get_rule(TokenType type) {
    return &rules[type];
}


static void parse_precedence(Compiler* self, Precedence precedence) {
    advance(self->parser);
    Token token = previous_token(self);
    ParseFn prefix_rule = get_rule(token.type)->prefix;
    if (prefix_rule == NULL) {
        store_error(self, token.location, COMPILE_ERROR_EXPECTED_PREFIX_TOKEN, token);
        return;
    }

    bool can_assign = precedence <= PRECEDENCE_ASSIGNMENT;
    prefix_rule(self, can_assign);

    while (precedence <= get_rule(current_token(self).type)->precedence) {
        advance(self->parser);
        ParseFn infix_rule = get_rule(self->parser->previous.type)->infix;
        if (infix_rule == NULL) {
            store_error(self, token.location, COMPILE_ERROR_EXPECTED_INFIX_TOKEN, token);
            return;
        }
        infix_rule(self, can_assign);
    }
}

static uint8_t identifier_constant(Compiler* self, Token* name) {
    Slice repr = parser_token_repr(self->parser, *name);
    ObjString* string = string_make(repr.source, repr.count);
    return make_constant(self, MAKE_OBJ(string));
}

static uint8_t parse_variable(Compiler* self, ErrorCode code) {
    consume(self, TOKEN_IDENTIFIER, code);

    declare_variable(self);
    if (self->scope_depth > 0)
        return 0;

    return identifier_constant(self, &self->parser->previous);
}

static void declare_variable(Compiler* self) {
    if (self->scope_depth == 0)
        return;

    Token name = previous_token(self);
    for (int i = self->local_count - 1; i >= 0; i--) {
        Local* local = &self->locals[i];
        if (local->depth != -1 && local->depth < self->scope_depth) {
            break;
        }

        if (identifiers_equal(self, &name, &local->name)) {
            store_error(self, name.location, COMPILE_ERROR_DECLARED_VARIABLE_TWICE, name);
            return;
        }
    }
    add_local(self, name);
}

static bool identifiers_equal(Compiler* self, Token* a, Token* b) {
    if (a->count != b->count)
        return false;

    Slice repr_a   = parser_token_repr(self->parser, *a);
    Slice repr_b   = parser_token_repr(self->parser, *b);
    bool  is_equal = memcmp(repr_a.source, repr_b.source, repr_a.count) == 0;

    return is_equal;
}

static void add_local(Compiler* self, Token name) {
    if (self->local_count == 256) {
        store_error(self, name.location, COMPILE_ERROR_TOO_MANY_LOCAL_VARIABLES, name);
        return;
    }
    Local* local = &self->locals[self->local_count++];
    local->name  = name;
    local->depth = -1;
}

static void mark_initialized(Compiler* self) {
    if (self->scope_depth == 0)
        return;

    self->locals[self->local_count - 1].depth = self->scope_depth;
}

static void define_variable(Compiler* self, uint8_t global) {
    if (self->scope_depth > 0) {
        mark_initialized(self);
        return;
    }
    emit_bytes(self, OP_DEFINE_GLOBAL, global);
}

static void literal(Compiler* self, bool can_assign) {
    Token token = previous_token(self);
    switch (token.type) {
        case TOKEN_FALSE: emit_byte(self, OP_FALSE); break;
        case TOKEN_TRUE:  emit_byte(self, OP_TRUE); break;
        default: {
            Slice repr = parser_token_repr(self->parser, token);
            PANIC("Unexpected token '%.*s'. Expected 'true' or 'false'", repr.count, repr.source);
        }
    }
}

static int resolve_local(Compiler* self, Token* name) {
    for (int i = self->local_count - 1; i >= 0; i--) {
        Local* local = &self->locals[i];
        if (identifiers_equal(self, name, &local->name)) {
            if (local->depth == -1) {
                Slice repr = parser_token_repr(self->parser, *name);
                store_error(self, name->location, COMPILE_ERROR_READING_VARIABLE_IN_OWN_INITIALIZER, *name);
            }
            return i;
        }
    }

    return -1;
}

static void named_variable(Compiler* self, Token name, bool can_assign) {
    uint8_t get_op, set_op;
    int arg = resolve_local(self, &name);
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(self, &name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && match(self, TOKEN_EQUAL)) {
        expression(self);
        emit_bytes(self, set_op, (uint8_t) arg);
    } else {
        emit_bytes(self, get_op, (uint8_t) arg);
    }
}

static void variable(Compiler* self, bool can_assign) {
    named_variable(self, self->parser->previous, can_assign);
}

static void num_i64(Compiler* self, bool can_assign) {
    Slice repr = parser_token_repr(self->parser, previous_token(self));
    Value value = MAKE_I64(strtoll(repr.source, NULL, 10));
    emit_constant(self, value);
}

static void num_f64(Compiler* self, bool can_assign) {
    Slice repr = parser_token_repr(self->parser, previous_token(self));
    Value value = MAKE_F64(strtod(repr.source, NULL));
    emit_constant(self, value);
}

static void string(Compiler* self, bool can_assign) {
    Slice repr = parser_token_repr(self->parser, previous_token(self));
    ObjString* string = string_make(repr.source, repr.count);
    Value constant = MAKE_OBJ(string);
    emit_constant(self, constant);
}

static void emit_constant(Compiler* compiler, Value value) {
    emit_bytes(compiler, OP_CONSTANT, make_constant(compiler, value));
    compiler->constants[compiler->const_ptr++] = value;
}

static u8 make_constant(Compiler* self, Value value) {
    int constant = chunk_add_constant(&self->function->chunk, value);
    if (constant > UINT8_MAX) {
        Token token = current_token(self);
        store_error(self, token.location, COMPILE_ERROR_TOO_MANY_CONSTANTS, token_make_empty());
    }

    return (u8) constant;
}

static void grouping(Compiler* self, bool can_assign) {
    expression(self);
    expect(self->parser, TOKEN_RIGHT_PAREN);
}

static void unary(Compiler* self, bool can_assign) {
    Token     token = previous_token(self);
    TokenType operation = token.type;

    // Compile the operand.
    parse_precedence(self, PRECEDENCE_UNARY);

    // Emit the operator instruction.
    switch (operation) {
        case TOKEN_MINUS:   emit_byte(self, OP_NEGATE);  break;
        case TOKEN_BANG:    emit_byte(self, OP_NOT);     break;
        default: {
            Slice repr = parser_token_repr(self->parser, token);
            PANIC("Unexpected unary operation '%.*s'. Expected '-' or '+'", repr.count, repr.source);
        }
    }
}



static void binary(Compiler* self, bool can_assign) {
    Token     token     = previous_token(self);
    TokenType operation = token.type;
    ParseRule* rule = get_rule(operation);
    parse_precedence(self, (Precedence)(rule->precedence + 1));

    switch (operation) {
        case TOKEN_PLUS:          emit_byte(self,  OP_ADD);             break;
        case TOKEN_MINUS:         emit_byte(self,  OP_SUBTRACT);        break;
        case TOKEN_STAR:          emit_byte(self,  OP_MULTIPLY);        break;
        case TOKEN_SLASH:         emit_byte(self,  OP_DIVIDE);          break;
        case TOKEN_AND:           emit_byte(self,  OP_AND);             break;
        case TOKEN_OR:            emit_byte(self,  OP_OR);              break;
        case TOKEN_BANG_EQUAL:    emit_bytes(self, OP_EQUAL, OP_NOT);   break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(self,  OP_EQUAL);           break;
        case TOKEN_GREATER:       emit_byte(self,  OP_GREATER);         break;
        case TOKEN_GREATER_EQUAL: emit_bytes(self, OP_LESS, OP_NOT);    break;
        case TOKEN_LESS:          emit_byte(self,  OP_LESS);            break;
        case TOKEN_LESS_EQUAL:    emit_bytes(self, OP_GREATER, OP_NOT); break;
        case TOKEN_END_STMT:      emit_byte(self,  OP_RETURN);          break;
        default: {
            Slice repr = parser_token_repr(self->parser, token);
            PANIC("Unexpected binary operation '%.*s'. Expected '+', '-', '*', '/', 'and', 'or', '!', '!=', '==', '<', '<=', '>', or, '>='", repr.count, repr.source);
        }
    }
}

static void expression(Compiler* self) {
    // We simply parse the lowest precedence level.
    // Except for PRECEDENCE_NONE.
    parse_precedence(self, PRECEDENCE_ASSIGNMENT);
}


static void expression_statement(Compiler* self) {
    expression(self);
    consume(self, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_EXPRESSION);
    emit_byte(self, OP_POP);
}


static void print_statement(Compiler* self) {
    expression(self);
    consume(self, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_EXPRESSION);
    emit_byte(self, OP_PRINT);
}

//static void statement(Compiler* compiler) {
//    print_statement(compiler);
//}


static void declaration(Compiler* self) {
    if (match(self, TOKEN_FUN)) {
        function_declaration(self);
    } else if  (match(self, TOKEN_VAR)) {
        variable_declaration(self);
    } else if (match(self, TOKEN_PRINT_STMT)) {
        print_statement(self);
    } else if (match(self, TOKEN_LEFT_BRACE)) {
        begin_scope(self);
        block(self);
        end_scope(self);
    } else if (match(self, TOKEN_IF)) {
        if_statement(self);
    } else if (match(self, TOKEN_WHILE)) {
        while_statement(self);
    } else if (match(self, TOKEN_FOR)) {
        for_statement(self);
    } else if (match(self, TOKEN_RETURN)) {
        return_statement(self);
    } else {
        expression_statement(self);
    }

    if (parser_had_error(self->parser) || self->had_error)
        synchronize(self);
}

static void return_statement(Compiler* self) {
    if (self->type == TYPE_SCRIPT) {
        Token token = current_token(self);
        store_error(self, token.location, COMPILE_ERROR_TRYING_TO_RETURN_FROM_SCRIPT, token_make_empty());
    }

    if (match(self, TOKEN_END_STMT)) {
        emit_byte(self, OP_NULL);
        emit_byte(self, OP_RETURN);
    } else {
        expression(self);
        consume(self, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_RETURN);
        emit_byte(self, OP_RETURN);
    }
}

static void function_declaration(Compiler* self) {
    uint8_t global = parse_variable(self, COMPILE_ERROR_EXPECTED_FUNCTION_NAME);
    mark_initialized(self);
    function(self, TYPE_FUNCTION);
    define_variable(self, global);
}

static void function(Compiler* self, FunctionType type) {
    Compiler compiler = compiler_make(self->parser, type);
    begin_scope(&compiler);

    consume(&compiler, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME);
    if (!check(&compiler, TOKEN_RIGHT_PAREN)) {
        do {
            compiler.function->arity++;
            if (compiler.function->arity > 255) {
                store_error(&compiler, current_token(self).location, COMPILE_ERROR_TOO_MANY_PARAMETERS, token_make_empty());
            }
            uint8_t constant = parse_variable(&compiler, COMPILE_ERROR_EXPECTED_PARAMETER_NAME);
            define_variable(&compiler, constant);
        } while (match(&compiler, TOKEN_COMMA));
    }

    consume(&compiler, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_PARAMETER);
    consume(&compiler, TOKEN_LEFT_BRACE, COMPILE_ERROR_EXPECTED_BRACE_BEFORE_BODY);
    block(&compiler);

    if (chunk_peek(&compiler.function->chunk) != OP_RETURN) {
        emit_byte(&compiler, OP_NULL);
        emit_byte(&compiler, OP_RETURN);
    }


    ObjFunction* function = compiler_end(&compiler);
    emit_bytes(self, OP_CONSTANT, make_constant(self, MAKE_OBJ(function)));
}

static void for_statement(Compiler* self) {
    begin_scope(self);

    consume(self, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_FOR);

    // Check initializer.
    if (match(self, TOKEN_VAR)) {
        variable_declaration(self);
    } else if (!match(self, TOKEN_END_STMT)) {
        expression_statement(self);
    }

    // Condition
    int loop_start = self->function->chunk.count;
    int exit_jump = -1;
    if (!match(self, TOKEN_END_STMT)) {
        expression(self);
        consume(self, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_LOOP_COND);

        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(self, OP_JUMP_IF_FALSE);
        emit_byte(self, OP_POP); // Condition.
    }

    // Increment
    if (!match(self, TOKEN_RIGHT_PAREN)) {
        int body_jump = emit_jump(self, OP_JUMP);
        int increment_start = self->function->chunk.count;
        expression(self);
        emit_byte(self, OP_POP);
        consume(self, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_FOR_CLAUSE);

        emit_loop(self, loop_start);
        loop_start = increment_start;
        patch_jump(self, body_jump);
    }


    // Body
    declaration(self);
    emit_loop(self, loop_start);

    if (exit_jump != -1) {
        patch_jump(self, exit_jump);
        emit_byte(self, OP_POP);   // Condition.
    }

    end_scope(self);
}

static void while_statement(Compiler* self) {
    int loop_start = self->function->chunk.count;

    consume(self, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_WHILE);
    expression(self);
    consume(self, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_COND);

    int exit_jump = emit_jump(self, OP_JUMP_IF_FALSE);
    emit_byte(self, OP_POP);
    declaration(self);
    emit_loop(self, loop_start);

    patch_jump(self, exit_jump);
    emit_byte(self, OP_POP);
}

static void emit_loop(Compiler* self, int start) {
    emit_byte(self, OP_LOOP);

    int offset = self->function->chunk.count - start + 2;
    if (offset > UINT16_MAX) {
        store_error(self, current_token(self).location, COMPILE_ERROR_LOOP_BODY_TO_LARGE, token_make_empty());
        return;
    }

    emit_byte(self, (offset >> 8) & 0xff);
    emit_byte(self, offset & 0xff);
}

static int emit_jump(Compiler* self, uint8_t instruction) {
    emit_byte(self, instruction);
    emit_byte(self, 0xff);  // Placeholders.
    emit_byte(self, 0xff);  // Placeholders.
    return self->function->chunk.count - 2;
}

static void if_statement(Compiler* self) {
    consume(self, TOKEN_LEFT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_IF);
    expression(self);
    consume(self, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_COND);

    int then_jump = emit_jump(self, OP_JUMP_IF_FALSE);
    emit_byte(self, OP_POP);
    declaration(self);

    int else_jump = emit_jump(self, OP_JUMP);
    patch_jump(self, then_jump);

    emit_byte(self, OP_POP);
    if (match(self, TOKEN_ELSE))
        declaration(self);
    patch_jump(self, else_jump);

}

static void patch_jump(Compiler* self, int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = self->function->chunk.count - offset - 2;

    if (jump > UINT16_MAX) {
        store_error(self, current_token(self).location, COMPILE_ERROR_TOO_LARGE_JUMP, token_make_empty());
    }

    self->function->chunk.code[offset]     = (jump >> 8) & 0xff;
    self->function->chunk.code[offset + 1] = jump & 0xff;
}

static void block(Compiler* self) {
    while (!check(self, TOKEN_RIGHT_BRACE) && !check(self, TOKEN_EOF)) {
        declaration(self);
    }

    consume(self, TOKEN_RIGHT_BRACE, COMPILE_ERROR_EXPECTED_BRACE_AFTER_BLOCK);
}

static void begin_scope(Compiler* self) {
    self->scope_depth += 1;
}

static void end_scope(Compiler* self) {
    self->scope_depth -= 1;
    while (self->local_count > 0 && self->locals[self->local_count - 1].depth > self->scope_depth) {
        emit_byte(self, OP_POP);
        self->local_count--;
    }
}

static void call(Compiler* self, bool can_assign) {
    uint8_t arg_count = argument_list(self);
    emit_bytes(self, OP_CALL, arg_count);
}

static uint8_t argument_list(Compiler* self) {
    uint8_t arg_count = 0;
    if (!check(self, TOKEN_RIGHT_PAREN)) {
        do {
            expression(self);
            arg_count++;
        } while (match(self, TOKEN_COMMA));
    }
    if (arg_count == 255) {
        store_error(self, current_token(self).location, COMPILE_ERROR_TOO_MANY_ARGUMENTS, token_make_empty());
    }
    consume(self, TOKEN_RIGHT_PAREN, COMPILE_ERROR_EXPECTED_PARENS_AFTER_ARGS);
    return arg_count;
}

static void variable_declaration(Compiler* self) {
    uint8_t global = parse_variable(self, COMPILE_ERROR_EXPECTED_PARENS_AFTER_ARGS);

    consume(self, TOKEN_EQUAL, COMPILE_ERROR_EXPECTED_EQUAL_AFTER_VAR_DECL);
    expression(self);
    consume(self, TOKEN_END_STMT, COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_DECL);

    define_variable(self, global);
}


static void synchronize(Compiler* self) {
    self->parser->in_panic_mode = false;
    self->in_panic_mode = false;

    // @NOTE: Skip over all tokens until we hit the start
    //  of a new statement.
    while (current_token(self).type != TOKEN_EOF) {
        if (previous_token(self).type == TOKEN_END_STMT)
            return;

        switch (current_token(self).type) {
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT_STMT:
            case TOKEN_RETURN:
                return;
            default:
                advance(self->parser);
        }

    }
}


static ObjFunction* compiler_end(Compiler* self) {
    if (parser_had_error(self->parser)) {
        for (int i = 0; i < self->parser->error_count; ++i) {
            print_error(self->parser->errors[i]);
        }
    }

    if (self->had_error) {
        for (int i = 0; i < self->error_count; ++i) {
            print_error(self->errors[i]);
        }
    }

    if (!parser_had_error(self->parser) && !self->had_error) {
        #ifdef COMPILER_OUTPUT_DISASSEMBLY
            chunk_disassemble(
                &self->function->chunk,
                (self->function->name != NULL) ?
                self->function->name->data: "<script>"
            );
        #endif
        return self->function;
    }

    return NULL;
}


ObjFunction* compile(const char* path, const char* source) {
    Parser   parser   = parser_make(path, source);
    Compiler compiler = compiler_make(&parser, TYPE_SCRIPT);

    advance(compiler.parser);
    while (!match(&compiler, TOKEN_EOF)) {
        declaration(&compiler);
    }

    emit_byte(&compiler, OP_EXIT);
    return compiler_end(&compiler);
}


