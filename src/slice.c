#include "slice.h"
#include "debug.h"


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
    ASSERT((self.source == 0 && self.count == 0) || (self.source != 0 && self.count != 0));
    return self.source == 0;
}


Slice previous_line(const char* source, int index) {
    const char* start = &source[index-1];
    if (start < source)
        return slice_make_empty();

    while (*start != '\n' && start != source)
        --start;
    if (*start == '\n') {
        --start;
        while (*start != '\n' && start != source)
            --start;
    }

    const char* stop = start+1;
    while (*stop != '\n' && *stop != '\0')
        ++stop;

    return (Slice) { .source=start, .count=(int)(stop-start) };
}


Slice current_line(const char* source, int index) {
    const char* start = &source[index-1];
    if (start < source)
        return slice_make_empty();

    const char* stop  = &source[index];

    while (start != source && *(start-1) != '\n')
        --start;
    while (*stop != '\n' && *stop != '\0')
        ++stop;

    return (Slice) { .source=start, .count=(int)(stop-start) };
}

Slice next_line(const char* source, int index) {
    const char* start = &source[index];

    while (*start != '\n' && *start != '\0')
        ++start;
    if (*start == '\n') start++;

    const char* stop = start+1;
    while (*stop != '\0' && *stop != '\n')
        ++stop;

    return (Slice) { .source=start, .count=(int)(stop-start) };
}
