#pragma once

#include "preamble.h"
#include "slice.h"


// ---------------- TABLE ----------------
// Interesting hash functions:
//      * http://www.cse.yorku.ca/~oz/hash.html
//      * https://stackoverflow.com/a/57960443/6486738
//      * https://stackoverflow.com/a/69812981/6486738




#define TABLE_MAX_LOAD_FACTOR       0.75
#define TABLE_ENTRY_EMPTY_VALUE     0x00
#define TABLE_ENTRY_TOMBSTONE_VALUE 0xFF

static u32 hash(Slice slice);

#define declare_table(V0, V1)                                                                                                                                                                \
                                                                                                                                                                                        \
typedef struct {                                                                                                                                                                        \
    Slice key;                                                                                                                                                                          \
    V0 value;                                                                                                                                                                            \
} Entry_##V1;                                                                                                                                                                            \
                                                                                                                                                                                        \
typedef struct {                                                                                                                                                                        \
    Entry_##V1* entries;                                                                                                                                                                 \
    u16* slots; /* @TODO: Use! */                                                                                                                                                       \
    int count;                                                                                                                                                                          \
    int capacity;                                                                                                                                                                       \
} Table_##V1;                                                                                                                                                                            \
                                                                                                                                                                                        \
static inline bool entry_##V1##_value_compare(V0* value, u8 x);                                                                                                                          \
static inline bool entry_##V1##_is_empty(Entry_##V1 entry);                 \
static inline bool entry_##V1##_is_tombstone(Entry_##V1 entry);                 \
static inline bool entry_##V1##_is_occupied(Entry_##V1 entry);                  \
static inline bool entry_##V1##_is_available(Entry_##V1 entry);                 \
                                                                                                                                                                                        \
static inline Entry_##V1 entry_##V1##_make_free();                                                                                                                                         \
static inline Entry_##V1 entry_##V1##_make_tombstone();                                                                                                                                    \
                                                                                                                                                                                        \
Table_##V1 table_##V1##_make();                                                                                                                                                          \
void table_##V1##_free(Table_##V1* table);                                                                                                                                                         \
Entry_##V1* table_##V1##_find(Table_##V1* table, Slice key);                                                                                                                                              \
Slice table_##V1##_delete(Table_##V1* table, Slice key);                                                                                                                                                      \
bool table_##V1##_set(Table_##V1* table, Slice key, V0 value);                                                                                                                                    \
bool table_##V1##_get(Table_##V1* table, Slice key, V0* value);                                                                                                                                  \
bool table_##V1##_add(Table_##V1* table, Slice key, V0 value);                                                                                                                             \
void table_##V1##_copy(Table_##V1* from, Table_##V1* to) ;                                                                                                                                     \
void table_##V1##_grow(Table_##V1* table, int new_capacity);                                                                                                                                              \




#define define_table(V0, V1)                                                                                                                                                                \
static inline bool entry_##V1##_value_compare(V0* value, u8 x) {                                                                                                                    \
    u8* data = (u8*) value;                                                                                                                                                 \
    for (int i = 0; i < (int) sizeof(V0); ++i) {                                                                                                                                         \
        if (data[i] != x)                                                                                                                                                               \
            return false;                                                                                                                                                               \
    }                                                                                                                                                                                   \
    return true;                                                                                                                                                                        \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
static inline bool entry_##V1##_is_empty(Entry_##V1 entry)      { return  slice_is_empty(entry.key) &&  entry_##V1##_value_compare(&entry.value, TABLE_ENTRY_EMPTY_VALUE);     }           \
static inline bool entry_##V1##_is_tombstone(Entry_##V1 entry)  { return  slice_is_empty(entry.key) &&  entry_##V1##_value_compare(&entry.value, TABLE_ENTRY_TOMBSTONE_VALUE); }           \
static inline bool entry_##V1##_is_occupied(Entry_##V1 entry)   { return !slice_is_empty(entry.key); }                                                                                    \
static inline bool entry_##V1##_is_available(Entry_##V1 entry)  { return !entry_##V1##_is_occupied(entry); }                                                                               \
                                                                                                                                                                                        \
static inline Entry_##V1 entry_##V1##_make_free() {                                                                                                                                       \
    Entry_##V1 entry = { 0 };                                                                                                                                                            \
    memset(&entry.value, TABLE_ENTRY_EMPTY_VALUE, sizeof(V0));                                                                                                                           \
    return entry;                                                                                                                                                                       \
}                                                                                                                                                                                       \
static inline Entry_##V1 entry_##V1##_make_tombstone() {                                                                                                                                  \
    Entry_##V1 entry = { 0 };                                                                                                                                                            \
    memset(&entry.value, TABLE_ENTRY_TOMBSTONE_VALUE, sizeof(V0));                                                                                                                       \
    return entry;                                                                                                                                                                       \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
                                                                                                                                                                                        \
Table_##V1 table_##V1##_make() {                                                                                                                                                          \
    return (Table_##V1) { .entries=NULL, .count=0, .capacity=0 };                                                                                                                        \
}                                                                                                                                                                                       \
void table_##V1##_free(Table_##V1* table) {                                                                                                                                           \
    FREE_ARRAY(Entry_##V1 , table->entries, table->capacity);                                                                                                                            \
    *table = table_##V1##_make();                                                                                                                                                        \
}                                                                                                                                                                                       \
Entry_##V1* table_##V1##_find(Table_##V1* table, Slice key) {                                                                                                                              \
    u32 index = hash(key) % table->capacity;                                                                                                                                            \
                                                                                                                                                                                        \
    Entry_##V1* tombstone = NULL;                                                                                                                                                        \
                                                                                                                                                                                        \
    while (true) {                                                                                                                                                                      \
        Entry_##V1 * entry = &table->entries[index];                                                                                                                                     \
        if (entry_##V1##_is_empty(*entry)) {                                                                                                                                 \
            /* Empty entry.                                                                                                                                                             \
               If we found a tombstone, reuse it.                                                                                                                                       \
               Otherwise, return the empty entry. */                                                                                                                                    \
            return tombstone != NULL ? tombstone : entry;                                                                                                                               \
        } else if (entry_##V1##_is_tombstone(*entry)) {                                                                                                                    \
            /* We found a tombstone. Store and continue.                                                                                                                                \
               If we don't find the entry, then return this.                                                                                                                            \
               Otherwise, return the found entry. */                                                                                                                                    \
            if (tombstone == NULL) tombstone = entry;                                                                                                                                   \
        } else if (slice_equals(entry->key, key)) {                                                                                                                                     \
            return entry;                                                                                                                                                               \
        }                                                                                                                                                                               \
                                                                                                                                                                                        \
        index = (index + 1) % table->capacity;                                                                                                                                          \
    }                                                                                                                                                                                   \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
Slice table_##V1##_delete(Table_##V1* table, Slice key) {                                                                                                                                 \
    if (table->count == 0)                                                                                                                                                              \
        return slice_make_empty();                                                                                                                                                      \
                                                                                                                                                                                        \
    /* Find the entry. */                                                                                                                                                               \
    Entry_##V1* entry = table_##V1##_find(table, key);                                                                                                                                    \
    if (!entry_##V1##_is_occupied(*entry))                                                                                                                                               \
        return slice_make_empty();                                                                                                                                                      \
                                                                                                                                                                                        \
    Slice the_key = entry->key;                                                                                                                                                         \
                                                                                                                                                                                        \
    /* Place a tombstone in the entry.  */                                                                                                                                              \
    *entry = entry_##V1##_make_tombstone();                                                                                                                                              \
                                                                                                                                                                                        \
    table->count -= 1;                                                                                                                                                                  \
                                                                                                                                                                                        \
    return the_key;                                                                                                                                                                     \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
bool table_##V1##_set(Table_##V1* table, Slice key, V0 value) {                                                                                                                            \
    if (table->count == 0)                                                                                                                                                              \
        return false;                                                                                                                                                                   \
                                                                                                                                                                                        \
    Entry_##V1* entry = table_##V1##_find(table, key);                                                                                                                                    \
    if (!entry_##V1##_is_occupied(*entry))                                                                                                                                               \
        return false;                                                                                                                                                                   \
                                                                                                                                                                                        \
    entry->value = value;                                                                                                                                                               \
    return true;                                                                                                                                                                        \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
bool table_##V1##_get(Table_##V1* table, Slice key, V0* value) {                                                                                                                           \
    if (table->count == 0)                                                                                                                                                              \
        return false;                                                                                                                                                                   \
                                                                                                                                                                                        \
    Entry_##V1* entry = table_##V1##_find(table, key);                                                                                                                                    \
    if (!entry_##V1##_is_occupied(*entry))                                                                                                                                               \
        return false;                                                                                                                                                                   \
                                                                                                                                                                                        \
    *value = entry->value;                                                                                                                                                              \
    return true;                                                                                                                                                                        \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
bool table_##V1##_add(Table_##V1* table, Slice key, V0 value);                                                                                                                             \
void table_##V1##_copy(Table_##V1* from, Table_##V1* to) {                                                                                                                                 \
    to->count = 0;                                                                                                                                                                      \
    for (int i = 0; i < from->capacity; i++) {                                                                                                                                          \
        Entry_##V1* entry = &from->entries[i];                                                                                                                                           \
        /* Copy if the entry is occupied, but don't copy tombstones. */                                                                                                                 \
        if (entry_##V1##_is_occupied(*entry)) {                                                                                                                                          \
            table_##V1##_add(to, entry->key, entry->value);                                                                                                                              \
        }                                                                                                                                                                               \
    }                                                                                                                                                                                   \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
void table_##V1##_grow(Table_##V1* table, int new_capacity) {                                                                                                                             \
    Entry_##V1* entries = ALLOCATE_ARRAY(Entry_##V1, new_capacity);                                                                                                                       \
    for (int i = 0; i < new_capacity; i++) {                                                                                                                                            \
        entries[i] = entry_##V1##_make_free();                                                                                                                                           \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    Table_##V1 old_table = *table;                                                                                                                                                       \
    Table_##V1 new_table = (Table_##V1) {                                                                                                                                                 \
            .capacity=new_capacity,                                                                                                                                                     \
            .count=0,                                                                                                                                                                   \
            .entries=entries                                                                                                                                                            \
    };                                                                                                                                                                                  \
                                                                                                                                                                                        \
    table_##V1##_copy(&old_table, &new_table);                                                                                                                                           \
    *table = new_table;                                                                                                                                                                 \
                                                                                                                                                                                        \
    FREE_ARRAY(Entry_##V1, old_table.entries, old_table.capacity);                                                                                                                       \
}                                                                                                                                                                                       \
                                                                                                                                                                                        \
bool table_##V1##_add(Table_##V1* table, Slice key, V0 value) {                                                                                                                            \
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD_FACTOR) {                                                                                                                   \
        int capacity = GROW_CAPACITY(table->capacity);                                                                                                                                  \
        table_##V1##_grow(table, capacity);                                                                                                                                              \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    Entry_##V1* entry = table_##V1##_find(table, key);                                                                                                                                    \
    bool is_new_key = entry->key.source == NULL;                                                                                                                                        \
                                                                                                                                                                                        \
    if (is_new_key)                                                                                                                                                                     \
        table->count++;                                                                                                                                                                 \
                                                                                                                                                                                        \
    entry->key   = key;                                                                                                                                                                 \
    entry->value = value;                                                                                                                                                               \
    return is_new_key;                                                                                                                                                                  \
}                                                                                                                                                                                       \
                                                                                                                                                                                             \


static u32 hash(Slice slice) {
    u32 hash = 2166136261u;
    for (int i = 0; i < slice.count; i++) {
        hash ^= (u8) slice.source[i];
        hash *= 16777619;
    }
    return hash;
}
