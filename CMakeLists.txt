cmake_minimum_required(VERSION 3.22)
project(chain C)

set(CMAKE_C_STANDARD 99)


set(WARNINGS "-Wall -Wextra -Wpedantic -Weverything -Wno-declaration-after-statement -Wno-missing-variable-declarations -Wno-unused-function -Wno-unused-parameter -Wno-strict-prototypes -Wno-missing-prototypes -Wno-padded -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-variable -Wno-switch-enum")
set(DEBUG_FLAGS "-O0 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
set(FLAGS -O2)

set(CMAKE_C_FLAGS "${WARNINGS} ${DEBUG_FLAGS}")
include_directories(src/)

add_executable(
    chain src/chain.c
    src/chunk.c
    src/compiler.c
    src/error.c
    src/interpreter.c
    src/memory.c
    src/object.c
    src/parser.c
    src/slice.c
    src/table.c
    src/token.c
    src/value.c
)
target_compile_definitions(chain PRIVATE -DVM_DEBUG_TRACE_EXECUTION -DDEBUG -DCOMPILER_OUTPUT_DISASSEMBLY)

add_executable(
    tests tests/main.c
    src/chunk.c
    src/compiler.c
    src/interpreter.c
    src/memory.c
    src/object.c
    src/parser.c
    src/slice.c
    src/table.c
    src/value.c
)