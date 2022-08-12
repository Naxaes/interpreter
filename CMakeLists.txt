cmake_minimum_required(VERSION 3.22)
project(chain C)

set(CMAKE_C_STANDARD 99)


set(CMAKE_C_FLAGS "-Wno-declaration-after-statement -Wno-missing-variable-declarations -Wno-unused-function -Wno-unused-parameter -Wno-strict-prototypes -Wno-missing-prototypes -Wno-padded -Wno-unused-parameter -Wno-sign-conversion -Wall -Wextra -Wpedantic -Weverything -O0 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
include_directories(src/)

add_executable(
    chain src/chain.c
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
target_compile_definitions(chain PRIVATE -DVM_DEBUG_TRACE_EXECUTION)

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