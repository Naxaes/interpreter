#include "memory.h"
#include <stdlib.h>


void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    (void) old_size;
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) 
        exit(1);
    return result;
}




bool is_power_of_two(uintptr_t n) {
    return n && ((n & (n - 1)) == 0);
}

void* to_nearest_power_of_two(const void* const data, uintptr_t alignment) {
    ASSERT(is_power_of_two(alignment));
    const u8* const ptr = data;
    return (void*) (((uintptr_t)(ptr + alignment - 1)) & -alignment);
}


StackAllocator make_stack(void* data, int capacity) {
    return (StackAllocator) {
            .data=data,
            .ptr=0,
            .capacity=capacity
    };
}

StackAllocator split_stack(StackAllocator allocator) {
    return (StackAllocator) {
            .data=allocator.data+allocator.ptr,
            .ptr=0,
            .capacity=allocator.capacity-allocator.ptr
    };
}

void* allocate_raw(StackAllocator* allocator, int size, int alignment) {
    u8* data = to_nearest_power_of_two(allocator->data + allocator->ptr, alignment);
    int to_align = (int) (data - (allocator->data + allocator->ptr));

    if (allocator->ptr + size + to_align > allocator->capacity)
        return NULL;

    allocator->ptr += size + to_align;
    return data;
}

void* allocator_push_raw(StackAllocator* allocator, int size, int alignment, void* value) {
    void* data = allocate_raw(allocator, size, alignment);
    memcpy(data, value, size);
    return data;
}

