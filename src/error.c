#include "error.h"



const char* ERROR_MESSAGE[] = {
        [PARSER_ERROR_UNKNOWN_TOKEN]   = "Unknown token '%.*s'",
        [PARSER_ERROR_UNEXPECTED_EOF]  = "Unexpected EOF",

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
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_RETURN]      = "Expected ';' after return expression, but got '%.*s'",
        [COMPILE_ERROR_EXPECTED_FUNCTION_NAME]                   = "Expected function name",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME]      = "Expected '(' after function name",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_GROUPING]           = "Expected ')' after starting expression with '('",
        [COMPILE_ERROR_EXPECTED_PARENS_AFTER_PARAMETER]          = "Expected ')' after parameters",
        [COMPILE_ERROR_EXPECTED_PARAMETER_NAME]                  = "Expected parameter name",
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
        [COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_DECL]    = "Expected ';' after variable declaration",

        [RUNTIME_ERROR_UNKNOWN_OP_CODE]                 = "Unknown opcode %.*s",
        [RUNTIME_ERROR_TOO_MANY_ARGUMENTS]              = "Too many arguments! %.*s",
        [RUNTIME_ERROR_TOO_FEW_ARGUMENTS]               = "Too few arguments! %.*s",
        [RUNTIME_ERROR_STACK_OVERFLOW]                  = "Stack overflowed",
        [RUNTIME_ERROR_INVALID_CALL]                    = "Can only call functions and classes. %.*s",
        [RUNTIME_ERROR_UNDEFINED_VARIABLE]              = "Variable '%.*s' has not been defined",
        [RUNTIME_ERROR_UNSAFE_FLOAT_COMPARISON]         = "Comparison between floats is inaccurate",
        [RUNTIME_ERROR_REDEFINITION_OF_NATIVE_FUNCTION] = "Native function '%.*s' is already defined",
};



void print_error(Error error) {
    const char* type = 0;
    if (PARSER_ERROR_START_INDEX < error.code && error.code < PARSER_ERROR_STOP_INDEX)
        type = "Parser";
    else if (COMPILE_ERROR_START_INDEX < error.code && error.code < COMPILE_ERROR_STOP_INDEX)
        type = "Compiler";
    else if (RUNTIME_ERROR_START_INDEX < error.code && error.code < RUNTIME_ERROR_STOP_INDEX)
        type = "Interpreter";
    else
        PANIC("Non-valid code '%d'", (int) error.code);

    Slice line_before = previous_line(error.source, error.start.index);
    Slice line_on     = current_line(error.source,  error.start.index);
    Slice line_after  = next_line(error.source,     error.start.index);

    char arrow_buffer[1024] = { 0 };
    int n = (error.start.col < 1022) ? error.start.col : 1022;
    for (int j = 0; j < n; ++j) {
        arrow_buffer[j] = '-';  // (i % 5 == 4) ? '*' : '-';
    }
    int m = (n+error.count < 1023) ? n+error.count : 1023;
    for (int j = n; j < m; ++j) {
        arrow_buffer[j] = '^';  // (i % 5 == 4) ? '*' : '-';
    }
    arrow_buffer[m] = '\0';

    fprintf(
        stderr, "[%s] Error in '%.*s' at %s:%d:%d:\n    ",
        type,
        error.function.count, error.function.source,
        error.path,
        error.start.row, error.start.col
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
