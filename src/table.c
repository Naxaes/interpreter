#include "table.h"
#include "array.h"

// Interesting hash functions:
//      * http://www.cse.yorku.ca/~oz/hash.html
//      * https://stackoverflow.com/a/57960443/6486738
//      * https://stackoverflow.com/a/69812981/6486738
static u32 hash(Slice slice);

static inline bool entry_is_empty(Entry entry);
static inline bool entry_is_tombstone(Entry entry);
static inline bool entry_is_occupied(Entry entry);
static inline bool entry_is_available(Entry entry);
static inline Entry entry_make_free();
static inline Entry entry_make_tombstone();


Table table_make() {
    Table table = { 0 };
    table.entries  = NULL;
    table.count    = 0;
    table.capacity = 0;
    return table;
}


void table_free(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    *table = table_make();
}


Entry* table_find(Table* table, Slice key) {
    u32 index = hash(key) % table->capacity;

    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &table->entries[index];
        if (entry_is_empty(*entry)) {
            // Empty entry.
            // If we found a tombstone, reuse it.
            // Otherwise, return the empty entry.
            return tombstone != NULL ? tombstone : entry;
        } else if (entry_is_tombstone(*entry)) {
            // We found a tombstone. Store and continue.
            // If we don't find the entry, then return this.
            // Otherwise, return the found entry.
            if (tombstone == NULL) tombstone = entry;
        } else if (slice_equals(entry->key, key)) {
            return entry;
        }

        index = (index + 1) % table->capacity;
    }
}


Slice table_delete(Table* table, Slice key) {
    if (table->count == 0)
        return slice_make_empty();

    // Find the entry.
    Entry* entry = table_find(table, key);
    if (!entry_is_occupied(*entry))
        return slice_make_empty();

    Slice the_key = entry->key;

    // Place a tombstone in the entry.
    *entry = entry_make_tombstone();

    // @TODO: Verify this is correct!
    //  We decrease count when deleting.
    table->count -= 1;

    return the_key;
}

bool table_set(Table* table, Slice key, Value value) {
    if (table->count == 0)
        return false;

    Entry* entry = table_find(table, key);
    if (!entry_is_occupied(*entry))
        return false;

    entry->value = value;
    return true;
}


bool table_get(Table* table, Slice key, Value* value) {
    if (table->count == 0)
        return false;

    Entry* entry = table_find(table, key);
    if (!entry_is_occupied(*entry))
        return false;

    *value = entry->value;
    return true;
}


void table_copy(Table* from, Table* to) {
    to->count = 0;
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        // Copy if the entry is occupied, but don't copy tombstones.
        if (entry_is_occupied(*entry)) {
            table_add(to, entry->key, entry->value);
        }
    }
}


void table_grow(Table* table, int new_capacity) {
    Entry* entries = ALLOCATE_ARRAY(Entry, new_capacity);
    for (int i = 0; i < new_capacity; i++) {
        entries[i] = entry_make_free();
    }

    Table old_table = *table;
    Table new_table = (Table) {
        .capacity=new_capacity,
        .count=0,
        .entries=entries
    };

    table_copy(&old_table, &new_table);
    *table = new_table;

    FREE_ARRAY(Entry, old_table.entries, old_table.capacity);
}


bool table_add(Table* table, Slice key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD_FACTOR) {
        int capacity = GROW_CAPACITY(table->capacity);
        table_grow(table, capacity);
    }

    Entry* entry = table_find(table, key);

    bool is_new_key = entry->key.source == NULL;
//    // If it's a tombstone, it's already accounted for.
//    if (is_new_key && IS_NULL(entry->value))
//        table->count++;

    // @TODO: Verify this is correct!
    //  We decrease count when deleting.
    if (is_new_key)
        table->count++;


    entry->key   = key;
    entry->value = value;
    return is_new_key;
}



static u32 hash(Slice slice) {
    u32 hash = 2166136261u;
    for (int i = 0; i < slice.count; i++) {
        hash ^= (u8) slice.source[i];
        hash *= 16777619;
    }
    return hash;
}


static inline bool entry_is_empty(Entry entry)      { return  slice_is_empty(entry.key) &&  IS_INVALID(entry.value); }
static inline bool entry_is_tombstone(Entry entry)  { return  slice_is_empty(entry.key) &&  IS_NULL(entry.value);    }
static inline bool entry_is_occupied(Entry entry)   { return !slice_is_empty(entry.key); }
static inline bool entry_is_available(Entry entry)  { return !entry_is_occupied(entry); }

static inline Entry entry_make_free()      { return (Entry) { .key={ 0, 0 }, .value=INVALID_VALUE() }; }
static inline Entry entry_make_tombstone() { return (Entry) { .key={ 0, 0 }, .value=NULL_VALUE()    }; }
