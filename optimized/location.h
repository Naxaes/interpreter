#pragma once

#include "c-preamble/nax_preamble.h"


typedef struct {
    u64 _unused : 8;
    u64 offset  : 24;
    u64 row     : 20;
    u64 column  : 12;
} Location;

Location make_location(u64 offset, u64 row, u64 column);
