cmake_minimum_required(VERSION 3.22)
project(chain2 C)

set(CMAKE_C_STANDARD 99)

set(WARNINGS "-Wall -Wextra -Wpedantic -Weverything -Wno-declaration-after-statement -Wno-missing-variable-declarations -Wno-unused-function -Wno-unused-parameter -Wno-strict-prototypes -Wno-missing-prototypes -Wno-padded -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-variable -Wno-switch-enum -Wno-gnu-binary-literal -Wno-unused-macros")
set(DEBUG_FLAGS "-O0 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
set(FLAGS -O2)

set(CMAKE_C_FLAGS "${WARNINGS} ${DEBUG_FLAGS}")

add_executable(
    chain2 main.c
    ast.c
    chunk.c
    compiler.c
    interpreter.c
    location.c
    memory.c
    object.c
    parser.c
    slice.c
    tokenizer.c
    utf8.c
    value.c
)
target_include_directories(chain2 PRIVATE .)
target_compile_definitions(chain2 PRIVATE -DVM_DEBUG_TRACE_EXECUTION -DDEBUG -DCOMPILER_OUTPUT_DISASSEMBLY)
