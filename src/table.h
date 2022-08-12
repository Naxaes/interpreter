#pragma once

#include "preamble.h"
#include "slice.h"
#include "value.h"


#define TABLE_MAX_LOAD_FACTOR 0.75


typedef struct {
    Slice key;
    Value value;
} Entry;


typedef struct {
    Entry* entries;
    int    count;
    int    capacity;
} Table;


Table  table_make();
void   table_free(Table* table);
Entry* table_find(Table* table, Slice key);
void   table_copy(Table* from, Table* to);
void   table_grow(Table* table, int new_capacity);
bool   table_add(Table* table, Slice key, Value value);
Slice  table_delete(Table* table, Slice key);
bool   table_set(Table* table, Slice key, Value  value);
bool   table_get(Table* table, Slice key, Value* value);

