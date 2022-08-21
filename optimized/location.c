#include "location.h"

Location make_location(u64 offset, u64 row, u64 column) {
    Location location = { 0, offset, row, column };
    return location;
}
