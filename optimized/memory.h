#pragma once

#include "preamble.h"


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


bool is_power_of_two(uintptr_t n);
void* to_nearest_power_of_two(const void* data, uintptr_t alignment);


typedef struct {
    u8* data;
    int ptr;
    int capacity;
} StackAllocator;

StackAllocator make_stack(void* data, int capacity);
StackAllocator split_stack(StackAllocator allocator);
void* allocate_raw(StackAllocator* allocator, int size, int alignment);
void* allocator_push_raw(StackAllocator* allocator, int size, int alignment, void* value);

// @TODO: Fix!
#define allocate(allocator, type, count) (type*) allocate_raw(allocator, (int)(count)*sizeof(type), align_of(type))
#define stack_top(allocator, type)   (type*) to_nearest_power_of_two((allocator).data + (allocator).ptr, align_of(type))
#define stack_push(allocator, type, value)  allocator_push_raw(allocator, sizeof(value), align_of(type), &(value))


