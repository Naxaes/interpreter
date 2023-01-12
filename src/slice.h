#pragma once

#include <stdbool.h>

typedef struct {
    const char* source;
    int count;
} Slice;

#define SLICE(x)  (Slice) { .source=(x), .count=sizeof(x) - 1 }
#define SLICE_EMPTY (Slice) { .source=0, .count=0  }

Slice slice_make(const char* source, int count);
Slice slice_str_offset(const char* source, int start, int stop);

Slice slice_make_empty();

bool slice_equals(Slice a, Slice b);
bool slice_is_empty(Slice self);

Slice previous_line(const char* source, int index);
Slice current_line(const char* source, int index);
Slice next_line(const char* source, int index);
