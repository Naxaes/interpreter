#pragma once

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>


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
