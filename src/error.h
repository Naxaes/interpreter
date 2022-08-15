#pragma once

#include "token.h"
#include "slice.h"

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


typedef enum {
    INFO,
    WARNING,
    ERROR,
} LogLevel;


#define STREAM      "STREAM"
#define PARSER      "PARSER"
#define COMPILER    "COMPILER"
#define INTERPRETER "INTERPRETER"

#define log(level, group, ...) do {                             \
    switch (level) {                                            \
        case INFO:    info(group, __VA_ARGS__);    break;       \
        case WARNING: warning(group, __VA_ARGS__); break;       \
        case ERROR:   error(group, __VA_ARGS__);   break;       \
    }                                                           \
} while (false);

#define info(group, ...)     do { fprintf(stdout, "[" group "] %s:%d INFO:\n",    __FILE__, __LINE__); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); } while(0)
#define warning(group, ...)  do { fprintf(stdout, "[" group "] %s:%d WARNING:\n", __FILE__, __LINE__); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); fflush(stdout); } while(0)
#define error(group, ...)    do { fprintf(stderr, "[" group "] %s:%d ERROR:\n",   __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stdout); fflush(stderr); raise(SIGINT); exit(-1); } while(0)



typedef enum {
    NO_ERROR,

    PARSER_ERROR_START_INDEX,
    PARSER_ERROR_UNKNOWN_TOKEN,
    PARSER_ERROR_UNEXPECTED_EOF,
    PARSER_ERROR_STOP_INDEX,

    COMPILE_ERROR_START_INDEX,
    COMPILE_ERROR_READING_VARIABLE_IN_OWN_INITIALIZER,
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
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_RETURN,
    COMPILE_ERROR_EXPECTED_FUNCTION_NAME,
    COMPILE_ERROR_EXPECTED_PARENS_AFTER_FUNCTION_NAME,
    COMPILE_ERROR_EXPECTED_PARAMETER_NAME,
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
    COMPILE_ERROR_EXPECTED_END_STATEMENT_AFTER_VAR_DECL,
    COMPILE_ERROR_STOP_INDEX,

    RUNTIME_ERROR_START_INDEX,
    RUNTIME_ERROR_UNKNOWN_OP_CODE,
    RUNTIME_ERROR_TOO_MANY_ARGUMENTS,
    RUNTIME_ERROR_TOO_FEW_ARGUMENTS,
    RUNTIME_ERROR_STACK_OVERFLOW,
    RUNTIME_ERROR_INVALID_CALL,
    RUNTIME_ERROR_UNDEFINED_VARIABLE,
    RUNTIME_ERROR_UNSAFE_FLOAT_COMPARISON,
    RUNTIME_ERROR_REDEFINITION_OF_NATIVE_FUNCTION,
    RUNTIME_ERROR_STOP_INDEX,
} ErrorCode;

extern const char* ERROR_MESSAGE[];


typedef struct {
    ErrorCode   code;
    Location    start;
    int         count;
    Slice       arg;
    const char* path;
    const char* source;
    Slice function;
} Error;





#ifdef DEBUG
#define D_ASSERT(x)  do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed", __FILE__, __LINE__); raise(SIGINT); }}  while(0)
#else
#define D_ASSERT(x)
#endif

// @NOTE: Always active. Use D_ASSERT for assertions in just debug mode.
#define ASSERT(x)        do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed", __FILE__, __LINE__); raise(SIGINT); }}  while(0)
#define ASSERTF(x, ...)  do { if (!(x)) { fprintf(stderr, "%s:%d: Assertion '" #x "' failed. ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); raise(SIGINT); }}  while(0)

#define PANIC(...)  do { fprintf(stderr, "[PANIC] %s:%d:\n    ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); raise(SIGINT); exit(-1); }  while(0)




void print_error(Error error);

