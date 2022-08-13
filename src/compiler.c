#include "compiler.h"
#include "opcodes.h"
#include "object.h"
#include "value.h"


typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;


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
static void call(Compiler* self);
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

    compiler.parser = parser;
    compiler.local_count = 0;
    compiler.scope_depth = 0;
    compiler.const_ptr   = 0;

    if (type != TYPE_SCRIPT) {
        compiler.function->name = string_make(
            &parser->source[parser->prev.index], parser->prev.count
        );
    }


    // @NOTE: The compiler implicitly claims stack slot zero
    //  for the VM’s own internal use
    Local* local = &compiler.locals[compiler.local_count++];
    local->depth = 0;
    local->name  = (Token) {.type=TOKEN_ERROR, .row=0, .col=0, .index=0, .count=0 };

    return compiler;
}


static void emit_byte(Compiler* self, u8 byte) {
    chunk_write(&self->function->chunk, byte, (Location) { self->parser->prev.row, self->parser->prev.index } );
}
static void emit_bytes(Compiler* self, u8 byte1, u8 byte2) {
    emit_byte(self, byte1);
    emit_byte(self, byte2);
}


static bool check(Compiler* self, TokenType type) {
    return self->parser->curr.type == type;
}

static bool match(Compiler* self, TokenType type) {
    if (!check(self, type))
        return false;
    advance(self->parser);
    return true;
}

static void consume(Compiler* self, TokenType type, const char* message) {
    if (!match(self, type))
        parser_error(self->parser, message);
}


typedef void (*ParseFn)(Compiler* compiler, bool can_assign);
typedef struct {
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
} ParseRule;

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {   grouping, call,   PREC_CALL    },
        [TOKEN_RIGHT_PAREN]   = {   NULL,     NULL,   PREC_NONE    },
        [TOKEN_MINUS]         = {   unary,    binary, PREC_TERM    },
        [TOKEN_PLUS]          = {   NULL,     binary, PREC_TERM    },
        [TOKEN_SLASH]         = {   NULL,     binary, PREC_FACTOR  },
        [TOKEN_STAR]          = {   NULL,     binary, PREC_FACTOR  },
        [TOKEN_I64]           = {   num_i64,  NULL,   PREC_NONE    },
        [TOKEN_F64]           = {   num_f64,  NULL,   PREC_NONE    },
        [TOKEN_FALSE]         = {   literal,  NULL,   PREC_NONE    },
        [TOKEN_TRUE]          = {   literal,  NULL,   PREC_NONE    },
        [TOKEN_IDENTIFIER]    = {   variable, NULL,   PREC_NONE    },

        [TOKEN_BANG]          = {   unary,    NULL,   PREC_UNARY   },

        [TOKEN_BANG_EQUAL]    = {   NULL,     binary,  PREC_EQUALITY    },
        [TOKEN_EQUAL_EQUAL]   = {   NULL,     binary,  PREC_EQUALITY    },
        [TOKEN_GREATER]       = {   NULL,     binary,  PREC_COMPARISON  },
        [TOKEN_GREATER_EQUAL] = {   NULL,     binary,  PREC_COMPARISON  },
        [TOKEN_LESS]          = {   NULL,     binary,  PREC_COMPARISON  },
        [TOKEN_LESS_EQUAL]    = {   NULL,     binary,  PREC_COMPARISON  },

        [TOKEN_AND]           = {   NULL,     binary, PREC_AND     },
        [TOKEN_OR]            = {   NULL,     binary, PREC_OR      },

        [TOKEN_STRING]        = {   string,   NULL,   PREC_NONE    },

        [TOKEN_RETURN]        = {   NULL,     NULL,   PREC_NONE    },
        [TOKEN_END_STMT]      = {   NULL,     NULL,   PREC_NONE    },
        [TOKEN_ERROR]         = {   NULL,     NULL,   PREC_NONE    },
        [TOKEN_EOF]           = {   NULL,     NULL,   PREC_NONE    },
};

static ParseRule* get_rule(TokenType type) {
    return &rules[type];
}


static void parse_precedence(Compiler* self, Precedence precedence) {
    advance(self->parser);
    ParseFn prefix_rule = get_rule(self->parser->prev.type)->prefix;
    if (prefix_rule == NULL) {
        parser_error(self->parser, "Expect expression.");
    }


    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(self, can_assign);

    while (precedence <= get_rule(self->parser->curr.type)->precedence) {
        advance(self->parser);
        ParseFn infix_rule = get_rule(self->parser->prev.type)->infix;
        infix_rule(self, can_assign);
    }
}

static uint8_t identifier_constant(Compiler* self, Token* name) {
    ObjString* string = string_make(&self->parser->source[name->index], name->count);
    return make_constant(self, OBJ_VALUE(string));
}

static uint8_t parse_variable(Compiler* self, const char* message) {
    consume(self, TOKEN_IDENTIFIER, message);

    declare_variable(self);
    if (self->scope_depth > 0)
        return 0;

    return identifier_constant(self, &self->parser->prev);
}

static void declare_variable(Compiler* self) {
    if (self->scope_depth == 0)
        return;

    Token* name = &self->parser->prev;
    for (int i = self->local_count - 1; i >= 0; i--) {
        Local* local = &self->locals[i];
        if (local->depth != -1 && local->depth < self->scope_depth) {
            break;
        }

        if (identifiers_equal(self, name, &local->name)) {
            parser_error(self->parser, "Already a variable with this name in this scope.");
        }
    }
    add_local(self, *name);
}

static bool identifiers_equal(Compiler* self, Token* a, Token* b) {
    if (a->count != b->count) return false;
    return memcmp(&self->parser->source[a->index], &self->parser->source[b->index], a->count) == 0;
}

static void add_local(Compiler* self, Token name) {
    if (self->local_count == 256) {
        parser_error(self->parser, "Too many local variables in function.");
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

static void literal(Compiler* compiler, bool can_assign) {
    switch (compiler->parser->prev.type) {
        case TOKEN_FALSE: emit_byte(compiler, OP_FALSE); break;
        case TOKEN_TRUE:  emit_byte(compiler, OP_TRUE); break;
        default: parser_error(compiler->parser, "Unexpected token."); // Unreachable.
    }
}

static int resolve_local(Compiler* self, Token* name) {
    for (int i = self->local_count - 1; i >= 0; i--) {
        Local* local = &self->locals[i];
        if (identifiers_equal(self, name, &local->name)) {
            if (local->depth == -1) {
                parser_error(self->parser, "Can't read local variable in its own initializer.");
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
    named_variable(self, self->parser->prev, can_assign);
}

static void num_i64(Compiler* self, bool can_assign) {
    Token token = self->parser->prev;
    Value value = I64_VALUE(strtoll(&self->parser->source[token.index], NULL, 10));
    emit_constant(self, value);
}

static void num_f64(Compiler* self, bool can_assign) {
    Token token = self->parser->prev;
    Value value = F64_VALUE(strtod(&self->parser->source[token.index], NULL));
    emit_constant(self, value);
}

static void string(Compiler* self, bool can_assign) {
    Token token = self->parser->prev;
    ObjString* string = string_make(&self->parser->source[token.index], token.count);
    Value constant = OBJ_VALUE(string);
    emit_constant(self, constant);
}

static void emit_constant(Compiler* compiler, Value value) {
    emit_bytes(compiler, OP_CONSTANT, make_constant(compiler, value));
    compiler->constants[compiler->const_ptr++] = value;
}

static u8 make_constant(Compiler* self, Value value) {
    int constant = chunk_add_constant(&self->function->chunk, value);
    if (constant > UINT8_MAX) {
        parser_error(self->parser, "Too many constants in one chunk.");
    }

    return (u8) constant;
}

static void grouping(Compiler* self, bool can_assign) {
    expression(self);
    expect(self->parser, TOKEN_RIGHT_PAREN);
}

static void unary(Compiler* self, bool can_assign) {
    TokenType operation = self->parser->prev.type;

    // Compile the operand.
    parse_precedence(self, PREC_UNARY);

    // Emit the operator instruction.
    switch (operation) {
        case TOKEN_MINUS:   emit_byte(self, OP_NEGATE);  break;
        case TOKEN_BANG:    emit_byte(self, OP_NOT);     break;
        default: parser_error(self->parser, "Unexpected unary operation token."); // Unreachable.
    }
}



static void binary(Compiler* compiler, bool can_assign) {
    TokenType operation = compiler->parser->prev.type;
    ParseRule* rule = get_rule(operation);
    parse_precedence(compiler, (Precedence)(rule->precedence + 1));

    switch (operation) {
        case TOKEN_PLUS:          emit_byte(compiler,  OP_ADD);             break;
        case TOKEN_MINUS:         emit_byte(compiler,  OP_SUBTRACT);        break;
        case TOKEN_STAR:          emit_byte(compiler,  OP_MULTIPLY);        break;
        case TOKEN_SLASH:         emit_byte(compiler,  OP_DIVIDE);          break;
        case TOKEN_AND:           emit_byte(compiler,  OP_AND);             break;
        case TOKEN_OR:            emit_byte(compiler,  OP_OR);              break;
        case TOKEN_BANG_EQUAL:    emit_bytes(compiler, OP_EQUAL, OP_NOT);   break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(compiler,  OP_EQUAL);           break;
        case TOKEN_GREATER:       emit_byte(compiler,  OP_GREATER);         break;
        case TOKEN_GREATER_EQUAL: emit_bytes(compiler, OP_LESS, OP_NOT);    break;
        case TOKEN_LESS:          emit_byte(compiler,  OP_LESS);            break;
        case TOKEN_LESS_EQUAL:    emit_bytes(compiler, OP_GREATER, OP_NOT); break;
        case TOKEN_END_STMT:      emit_byte(compiler,  OP_RETURN);          break;
        default: parser_error(compiler->parser, "Unexpected binary operation token."); // Unreachable.
    }
}

static void expression(Compiler* compiler) {
    // We simply parse the lowest precedence level.
    // Except for PREC_NONE.
    parse_precedence(compiler, PREC_ASSIGNMENT);
}


static void expression_statement(Compiler* compiler) {
    expression(compiler);
    consume(compiler, TOKEN_END_STMT, "Expect END_STMT after expression.");
    emit_byte(compiler, OP_POP);
}


static void print_statement(Compiler* compiler) {
    expression(compiler);
    consume(compiler, TOKEN_END_STMT, "Expected END_STMT after expression.");
    emit_byte(compiler, OP_PRINT);
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

    if (self->parser->had_error)
        synchronize(self);
}

static void return_statement(Compiler* self) {
    if (self->type == TYPE_SCRIPT) {
        parser_error(self->parser, "Can't return from top-level code.");
    }

    if (match(self, TOKEN_END_STMT)) {
        emit_byte(self, OP_NULL);
        emit_byte(self, OP_RETURN);
    } else {
        expression(self);
        consume(self, TOKEN_END_STMT, "Expect ';' after return value.");
        emit_byte(self, OP_RETURN);
    }
}

static void function_declaration(Compiler* self) {
    uint8_t global = parse_variable(self, "Expect function name.");
    mark_initialized(self);
    function(self, TYPE_FUNCTION);
    define_variable(self, global);
}

static void function(Compiler* self, FunctionType type) {
    Compiler compiler = compiler_make(self->parser, type);
    begin_scope(&compiler);

    consume(&compiler, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(&compiler, TOKEN_RIGHT_PAREN)) {
        do {
            compiler.function->arity++;
            if (compiler.function->arity > 255) {
                parser_error(compiler.parser, "Can't have more than 255 parameters.");
            }
            uint8_t constant = parse_variable(&compiler, "Expect parameter name.");
            define_variable(&compiler, constant);
        } while (match(&compiler, TOKEN_COMMA));
    }

    consume(&compiler, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(&compiler, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(&compiler);

    if (chunk_peek(&compiler.function->chunk) != OP_RETURN) {
        emit_byte(&compiler, OP_NULL);
        emit_byte(&compiler, OP_RETURN);
    }


    ObjFunction* function = compiler_end(&compiler);
    emit_bytes(self, OP_CONSTANT, make_constant(self, OBJ_VALUE(function)));
}

static void for_statement(Compiler* self) {
    begin_scope(self);

    consume(self, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

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
        consume(self, TOKEN_END_STMT, "Expect ';' after loop condition.");

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
        consume(self, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

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

    consume(self, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression(self);
    consume(self, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

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
    if (offset > UINT16_MAX) parser_error(self->parser, "Loop body too large.");

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
    consume(self, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(self);
    consume(self, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

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
        parser_error(self->parser, "Too much code to jump over.");
    }

    self->function->chunk.code[offset]     = (jump >> 8) & 0xff;
    self->function->chunk.code[offset + 1] = jump & 0xff;
}

static void block(Compiler* self) {
    while (!check(self, TOKEN_RIGHT_BRACE) && !check(self, TOKEN_EOF)) {
        declaration(self);
    }

    consume(self, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
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

static void call(Compiler* self) {
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
        parser_error(self->parser, "Can't have more than 255 arguments.");
    }
    consume(self, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void variable_declaration(Compiler* self) {
    uint8_t global = parse_variable(self, "Expect variable name.");

    consume(self, TOKEN_EQUAL, "Must initialize variable!");
    expression(self);
    consume(self, TOKEN_END_STMT, "Expect ';' after variable declaration.");

    define_variable(self, global);
}

static void synchronize(Compiler* self) {
    self->parser->in_panic_mode = false;

    while (self->parser->curr.type != TOKEN_EOF) {
        if (self->parser->prev.type == TOKEN_END_STMT) return;
        switch (self->parser->curr.type) {
//            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT_STMT:
            case TOKEN_RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advance(self->parser);
    }
}


static ObjFunction* compiler_end(Compiler* self) {
    if (!self->parser->had_error) {
        chunk_disassemble(
            &self->function->chunk,
            (self->function->name != NULL) ?
            self->function->name->data: "<script>"
        );
        return self->function;
    } else {
        return NULL;
    }
}


ObjFunction* compile(const char* source) {
    Parser   parser   = parser_make(source);
    Compiler compiler = compiler_make(&parser, TYPE_SCRIPT);

    advance(compiler.parser);
    while (!match(&compiler, TOKEN_EOF)) {
        declaration(&compiler);
    }

    emit_byte(&compiler, OP_EXIT);
    return compiler_end(&compiler);
}


