cmake_minimum_required(VERSION 3.22)
project(sorted_array C)

set(CMAKE_C_STANDARD 99)

set(WARNINGS "-Wall -Wextra -Wpedantic -Weverything -Wno-declaration-after-statement -Wno-missing-variable-declarations -Wno-unused-function -Wno-unused-parameter -Wno-strict-prototypes -Wno-missing-prototypes -Wno-padded -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-variable -Wno-switch-enum -Wno-gnu-binary-literal -Wno-unused-macros")
set(DEBUG_FLAGS "-O0 -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
set(FLAGS -O2)


if (CMAKE_BUILD_TYPE MATCHES Debug)
    message("Compiling in debug with ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} and flags ${WARNINGS} ${DEBUG_FLAGS}")
    set(CMAKE_C_FLAGS "${WARNINGS} ${DEBUG_FLAGS}")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message("Compiling in release with ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} and flags ${WARNINGS} ${FLAGS}")
    set(CMAKE_C_FLAGS "${WARNINGS} ${FLAGS}")
endif ()


add_executable(sorted_array main.c)
target_include_directories(sorted_array PRIVATE .)
target_compile_definitions(sorted_array PRIVATE -DSORTED_ARRAY_IMPLEMENTATION)
