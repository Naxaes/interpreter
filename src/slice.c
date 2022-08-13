#include "slice.h"
#include "error.h"


Slice slice_make_empty() {
    return (Slice) { .source=0, .count=0 };
}

bool slice_equals(Slice a, Slice b) {
    if (a.count != b.count)
        return false;
    for (int i = 0; i < a.count; ++i) {
        if (a.source[i] != b.source[i])
            return false;
    }
    return true;
}

bool slice_is_empty(Slice self) {
    return self.count == 0;
}


Slice previous_line(const char* source, int index) {
    const char* start = &source[index-1];
    if (start < source)
        return slice_make_empty();

    while (start != source && *start != '\n')
        --start;

    if (start == source)
        return slice_make_empty();

    if (start != source && *start == '\n') {
        --start;
        while (start != source && *start != '\n')
            --start;
    }

    const char* stop = start;
    while (*stop != '\0' && *stop != '\n')
        ++stop;

    return (Slice) { .source=start, .count=(int)(stop-start) };
}


Slice current_line(const char* source, int index) {
    const char* start = &source[index];
    if (start < source)
        return slice_make_empty();

    const char* stop  = &source[index];

    while (start != source && *(start-1) != '\n')
        --start;
    while (*stop != '\0' && *stop != '\n')
        ++stop;

    return (Slice) { .source=start, .count=(int)(stop-start) };
}

Slice next_line(const char* source, int index) {
    const char* start = &source[index];

    while (*start != '\0' && *start != '\n')
        ++start;
    if (*start == '\n') start++;

    const char* stop = start;
    if (*start != '\0' && *start != '\n') {
        stop = start+1;
        while (*stop != '\0' && *stop != '\n')
            ++stop;
    }

    return (Slice) { .source=start, .count=(int)(stop-start) };
}
