cmake_minimum_required(VERSION 3.22)
project(chain2 C)

set(CMAKE_C_STANDARD 11)

set(WARNINGS "-Wall -Wextra -Wpedantic -Weverything -Wno-declaration-after-statement -Wno-missing-variable-declarations -Wno-unused-function -Wno-unused-parameter -Wno-strict-prototypes -Wno-missing-prototypes -Wno-padded -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-variable -Wno-switch-enum -Wno-gnu-binary-literal -Wno-unused-macros")
set(DEBUG_FLAGS "-O0 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
set(FLAGS -O2)


if (CMAKE_BUILD_TYPE MATCHES Debug)
    message("Compiling in debug...")
    set(CMAKE_C_FLAGS "${WARNINGS} ${DEBUG_FLAGS}")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message("Compiling in release...")
    set(CMAKE_C_FLAGS "${WARNINGS} ${FLAGS}")
endif ()



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
target_include_directories(chain2 PRIVATE libraries/)
target_compile_definitions(chain2 PRIVATE -DVM_DEBUG_TRACE_EXECUTION -DDEBUG -DCOMPILER_OUTPUT_DISASSEMBLY)



add_subdirectory(table)
add_subdirectory(sorted_array)


add_executable(c_target c_target.c)