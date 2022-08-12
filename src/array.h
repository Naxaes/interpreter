#pragma once

#include "memory.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define ALLOCATE_ARRAY(type, new_count) (type*) reallocate(0, 0, sizeof(type) * (new_count))
#define GROW_ARRAY(type, pointer, old_count, new_count) (type*) reallocate(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))
#define FREE_ARRAY(type, pointer, old_count) reallocate(pointer, sizeof(type) * (old_count), 0)
