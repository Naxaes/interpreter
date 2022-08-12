#pragma once
#include <stddef.h>

#define ALLOCATE(type) (type*) reallocate(0, 0, sizeof(type))
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)
#define FREE_RAW(pointer, size) reallocate(pointer, size, 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size);

