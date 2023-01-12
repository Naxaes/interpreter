/* Things a table need:
 *  1. TABLE_KEY type
 *  2. TABLE_KEY compare function
 *  3. TABLE_KEY hash function
 *  4. TABLE_KEY empty value
 *  5. TABLE_VALUE type
 *  6. TABLE_VALUE compare function
 *  7. TABLE_VALUE empty value
 *  8. TABLE_VALUE tombstone value
 *
 * Optional values:
 *  1. Load factor to grow
 *  2. Load factor to shrink
 *  3. Resize factor
 *  4. Index sparseness
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


// How many percent of the table's capacity, or more, until it grows.
#ifndef LOAD_FACTOR_TO_GROW
#define LOAD_FACTOR_TO_GROW     0.75
#endif
// How many percent of the table's capacity, or less, until it shrinks.
#ifndef LOAD_FACTOR_TO_SHRINK
#define LOAD_FACTOR_TO_SHRINK   0.33
#endif
// Factor of index array relative to the table's capacity.
#ifndef SPARSENESS_FACTOR
#define SPARSENESS_FACTOR       2
#endif
// Factor the table should grow or shrink with.
#ifndef RESIZE_FACTOR
#define RESIZE_FACTOR           2
#endif


typedef struct {
    // @INVARIANT: Must always be less than `capacity * LOAD_FACTOR_TO_GROW`.
    // @NOTE: Is lazy and not always accurate due to tombstones.
    uint32_t _count;
    uint32_t capacity;  // @INVARIANT: Must always be a multiple of 2.
    /* Joint allocation with the
     * key, value, hash, index - arrays
     * */
    uint8_t* data;
} Table;
Table table_make();
static inline void  table_free(Table* self) {
    free(self->data);
    *self = table_make();
}




#ifdef TABLE_IMPLEMENTATION
// ---------------- TABLE ----------------
// https://www.dropbox.com/s/5wxmeffrm1i5zqw/Pycon2017CompactDictTalk.pdf?dl=0
// Interesting hash functions:
//      * http://www.cse.yorku.ca/~oz/hash.html
//      * https://stackoverflow.com/a/57960443/6486738
//      * https://stackoverflow.com/a/69812981/6486738
// Modulus without division:
//      * https://homepage.cs.uiowa.edu/~jones/bcd/mod.shtml
#include <assert.h>
#include <stddef.h>

// @TODO: Make user definable?
#define HashIndex uint32_t

// @TODO: Make user definable?
#define SlotIndex uint16_t
#define INVALID_SLOT    ((SlotIndex)-1)
#define EMPTY_ENTRY     ((SlotIndex)-1)
#define TOMBSTONE_ENTRY ((SlotIndex)(-2))


TABLE_KEY*    p_table_key_array(Table* table);
TABLE_VALUE*  p_table_value_array(Table* table);
HashIndex*    p_table_hash_array(Table* table);
SlotIndex*    p_table_index_array(Table* table);


static bool p_table_resize(Table* self, int new_capacity);


// https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2#comment97021102_466242
static inline uint64_t next_power_of_2(uint64_t x) {
    return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1));
}
static inline bool is_power_of_two(uint64_t n) {
    return n && ((n & (n - 1)) == 0);
}
static inline void* to_nearest_power_of_two(const void* const data, uintptr_t alignment) {
    assert(is_power_of_two(alignment) && "Alignment must be a power of 2");
    const uint8_t* const ptr = data;
    return (void*) (((uintptr_t)(ptr + alignment - 1)) & -alignment);
}
#define align_of(type) offsetof(struct { char c; type member; }, member)
#define with_alignment(ptr, type) (type*) to_nearest_power_of_two(ptr, align_of(type))


#define cap_with_slack(capacity) (uint64_t)((double)(capacity) * SPARSENESS_FACTOR)


TABLE_KEY*    p_table_key_array(Table* self)     { TABLE_KEY*   x = (TABLE_KEY*)   (self->data);                                                                                   assert((uintptr_t)x % 8 == 0); return x; }
TABLE_VALUE*  p_table_value_array(Table* self)   { TABLE_VALUE* x = (TABLE_VALUE*) (self->data +  sizeof(TABLE_KEY) * self->capacity);                                             assert((uintptr_t)x % 8 == 0); return x; }
HashIndex*    p_table_hash_array(Table* self)    { HashIndex*   x = (HashIndex*)   (self->data + (sizeof(TABLE_KEY) + sizeof(TABLE_VALUE)) * self->capacity);                      assert((uintptr_t)x % 8 == 0); return x; }
SlotIndex*    p_table_index_array(Table* self)   { SlotIndex*   x = (SlotIndex*)   (self->data + (sizeof(TABLE_KEY) + sizeof(TABLE_VALUE) + sizeof(HashIndex)) * self->capacity);  assert((uintptr_t)x % 8 == 0); return x; }


typedef struct {
    SlotIndex i;
    SlotIndex lookup;
    HashIndex hash;
} TableIndex;


Table table_make() {
    uint32_t initial_capacity = 8;
//    assert(initial_capacity != 0 && "We always assume a positibe capacity");
    initial_capacity = (int) next_power_of_2(initial_capacity);

    // @NOTE: Since capacity is always a power of 2 greater than 8,
    //  these will automatically be 8 byte aligned.
    uint64_t total_size =
            sizeof(TABLE_KEY)   * initial_capacity +
            sizeof(TABLE_VALUE) * initial_capacity +
            sizeof(HashIndex)   * initial_capacity +
            sizeof(SlotIndex)   * cap_with_slack(initial_capacity);

    void*    data     = malloc(total_size);
    uint32_t capacity = initial_capacity;

    // @INVARIANT: Requires at least 8 byte alignment.
    assert((uintptr_t)(data) % 8 == 0);
    Table table = (Table) { 0, (data == NULL) ? 0 : capacity, data };

    SlotIndex* indices = p_table_index_array(&table);
    memset(indices, EMPTY_ENTRY, sizeof(SlotIndex) * cap_with_slack(initial_capacity));

    return table;
}


static inline TableIndex p_table_get_first_free_slot_index(Table* self, TABLE_KEY key) {
    assert(self->data != NULL && self->capacity != 0);

    HashIndex hash = (HashIndex) (TABLE_HASH(key));
    HashIndex slot = hash;

    SlotIndex* indices   = p_table_index_array(self);
    SlotIndex  tombstone = INVALID_SLOT;

    int collisions = 0;
    while (true) {
        SlotIndex slot_index  = slot % cap_with_slack(self->capacity);
        SlotIndex entry_index = indices[slot_index];
        if (entry_index == EMPTY_ENTRY) {
//            printf("'%s' found empty slot after %d collisions\n", key, collisions);
            return (tombstone != INVALID_SLOT) ?
                (TableIndex) { .i=tombstone,  .lookup=EMPTY_ENTRY, .hash=hash } :
                (TableIndex) { .i=slot_index, .lookup=EMPTY_ENTRY, .hash=hash };
        } else if (entry_index == TOMBSTONE_ENTRY) {
            // Entry is dead but still needed for proper lookup.
            // Continue to next, but store this index if we don't
            // find the key and want to return the first available slot.;
            if (tombstone == INVALID_SLOT)
                tombstone = slot_index;
        } else {
            TABLE_KEY* keys = p_table_key_array(self);
            TABLE_KEY  candidate = keys[entry_index];
            if (TABLE_KEY_COMPARE(key, candidate)) {
//                printf("'%s' found itself slot after %d collisions\n", key, collisions);
                return (TableIndex) { .i=slot_index, .lookup=entry_index, .hash=hash };
            }
        }

        ++collisions;
        slot += 1;
    }
}

bool table_delete(Table* self, TABLE_KEY key) {
    assert(self->data != NULL && self->capacity != 0);

    TableIndex index = p_table_get_first_free_slot_index(self, key);
    if (index.lookup == EMPTY_ENTRY)
        return false;

    SlotIndex* indices = p_table_index_array(self);
    indices[index.i] = TOMBSTONE_ENTRY;

    return true;
}

/* Returns true if the value was added and false if it replaced an existing value */
bool table_set(Table* self, TABLE_KEY key, TABLE_VALUE value) {
    assert(self->data != NULL && self->capacity != 0);

    TableIndex index = p_table_get_first_free_slot_index(self, key);

    if (index.lookup == EMPTY_ENTRY) {
        TABLE_KEY*   keys    = p_table_key_array(self);
        TABLE_VALUE* values  = p_table_value_array(self);
        HashIndex*   hashes  = p_table_hash_array(self);
        SlotIndex*   indices = p_table_index_array(self);
        keys[self->_count]   = key;
        values[self->_count] = value;
        hashes[self->_count] = index.hash;
        indices[index.i]     = (SlotIndex)self->_count;

        assert(self->_count < 65536 && "Hashmap overflow");
        self->_count += 1;
        if (self->_count > self->capacity * LOAD_FACTOR_TO_GROW) {
            bool resized = p_table_resize(self, (self->capacity == 0) ? 8 : self->capacity * RESIZE_FACTOR);
            assert(resized && "Couldn't grow table! No more memory");
        }
        return true;
    } else {
        TABLE_VALUE* values = p_table_value_array(self);
        values[index.lookup] = value;
        return false;
    }
}

/* Returns the value if it exists, and otherwise returns NULL. */
TABLE_VALUE* table_get(Table* self, TABLE_KEY key) {
    assert(self->data != NULL && self->capacity != 0);

    TableIndex index = p_table_get_first_free_slot_index(self, key);
    if (index.lookup == EMPTY_ENTRY)
        return NULL;

    TABLE_VALUE* values = p_table_value_array(self);
    TABLE_VALUE* value  = &values[index.lookup];
    return value;
}


bool p_table_resize(Table* self, int new_capacity) {
    assert(self->data != NULL && self->capacity != 0);

    new_capacity = (int) next_power_of_2(new_capacity);
    int old_capacity = self->capacity;

    uint64_t total_size =
        sizeof(TABLE_KEY)   * new_capacity +
        sizeof(TABLE_VALUE) * new_capacity +
        sizeof(HashIndex)   * new_capacity +
        sizeof(SlotIndex)   * cap_with_slack(new_capacity);

    void* old_data = self->data;

    TABLE_KEY*   old_keys    = p_table_key_array(self);
    TABLE_VALUE* old_values  = p_table_value_array(self);
    HashIndex*   old_hashes  = p_table_hash_array(self);

    self->data     = malloc(total_size);
    self->capacity = new_capacity;

    TABLE_KEY*   new_keys    = p_table_key_array(self);
    TABLE_VALUE* new_values  = p_table_value_array(self);
    HashIndex*   new_hashes  = p_table_hash_array(self);
    SlotIndex*   new_indices = p_table_index_array(self);

    memcpy(new_keys,    old_keys,    sizeof(TABLE_KEY)   * old_capacity);
    memcpy(new_values,  old_values,  sizeof(TABLE_VALUE) * old_capacity);
    memcpy(new_hashes,  old_hashes,  sizeof(HashIndex)   * old_capacity);
    memset(new_indices, EMPTY_ENTRY, sizeof(SlotIndex)   * cap_with_slack(new_capacity));

    free(old_data);

    int count = 0;
    for (SlotIndex i = 0; i < self->_count; ++i) {
        HashIndex index = new_hashes[i] % cap_with_slack(self->capacity);
        while (true) {
            if (new_indices[index] == EMPTY_ENTRY) {
                new_indices[index] = i;
                break;
            }
            index = (index + 1)  % cap_with_slack(self->capacity);
        }
        count += 1;
    }
    self->_count = count;

    return self->data != NULL;
}

#undef align_of
#endif
