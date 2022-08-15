#include "slice.h"
#include "error.h"

Slice slice_make(const char* source, int count) {
    return (Slice) { .source=source, .count=count };
}

Slice slice_str_offset(const char* source, int start, int count) {
    return (Slice) { .source=&source[start], .count=count };
}


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
    if (index == 0)
        return slice_make_empty();

    const char* stop = source + index - 1;
    while (stop > source && *stop != '\n')
        --stop;

    if (stop == source)
        return slice_make_empty();

    const char* start = stop;
    while ((start-1) > source && *(start-1) != '\n')
        --start;

    if (start == stop)
        return (Slice) { .source=" ", .count=1 };
    else
        return (Slice) { .source=start, .count=(int)(stop-start) };
}


Slice current_line(const char* source, int index) {
    if (index == 0)
        return slice_make_empty();

    const char* start = source + index;
    while (start > source && *(start-1) != '\n')
        --start;

    const char* stop = source + index;
    while (*stop != '\0' && *stop != '\n')
        ++stop;

    return (Slice) { .source=start, .count=(int)(stop-start) };
}

Slice next_line(const char* source, int index) {
    if (index == 0)
        return slice_make_empty();

    const char* start = source + index;
    while (*start != '\0' && *start != '\n')
        ++start;

    if (*start == '\0')
        return (Slice) { .source=" ", .count=1 };
    ++start;  // @NOTE: Skip newline.

    const char* stop = start;
    while (*stop != '\0' && *stop != '\n')
        ++stop;

    if (start == stop)
        return (Slice) { .source=" ", .count=1 };
    else
        return (Slice) { .source=start, .count=(int)(stop-start) };
}
