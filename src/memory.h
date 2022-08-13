#pragma once
#include <stddef.h>


#define ALLOCATE(type)              ((type*) reallocate(0, 0, sizeof(type)))
#define ALLOCATE_RAW(type, size)    ((type*) reallocate(0, 0, size))
#define ALLOCATE_ARRAY(type, count) ((type*) reallocate(0, 0, sizeof(type) * (count)))

#define FREE(type, ptr)                  reallocate(ptr, sizeof(type), 0)
#define FREE_RAW(ptr, old_size)          reallocate(ptr, old_size, 0)
#define FREE_ARRAY(type, ptr, old_count) reallocate(ptr, sizeof(type) * (old_count), 0)

#define RESIZE_RAW(type, ptr, old_size, new_size)      ((type*) reallocate(ptr, old_size, new_size))
#define RESIZE_ARRAY(type, ptr, old_count, new_count)  ((type*) reallocate(ptr, sizeof(type) * (old_count), sizeof(type) * (new_count)))

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)


void* reallocate(void* pointer, size_t old_size, size_t new_size);

