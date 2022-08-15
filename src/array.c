#include "preamble.h"
#include "array.h"
#include "memory.h"
#include "error.h"

#define array_impl(TYPE)                                                                    \
array_use(TYPE)                                                                             \
                                                                                            \
array_##TYPE array_##TYPE##_make() {                                                        \
    return (array_##TYPE) { 0 };                                                            \
}                                                                                           \
                                                                                            \
void array_##TYPE##_add(array_##TYPE* array, TYPE value) {                                  \
    if (array->count + 1 < array->capacity) {                                               \
        int old_capacity = array->capacity;                                                 \
        array->capacity  = GROW_CAPACITY(old_capacity);                                     \
        array->data      = RESIZE_ARRAY(TYPE, array->data, old_capacity, array->capacity);  \
                                                                                            \
        ASSERT(array->data != NULL);                                                        \
    }                                                                                       \
                                                                                            \
    array->data[array->count++] = value;                                                    \
}                                                                                           \
                                                                                            \
TYPE* array_##TYPE##_last(array_##TYPE* array) {                                            \
    D_ASSERT(array->count > 0);                                                             \
    return &array->data[array->count-1];                                                    \
}                                                                                           \
                                                                                            \
TYPE* array_##TYPE##_get(array_##TYPE* array, int index) {                                  \
    D_ASSERT(0 <= index && index < array->count);                                           \
    return &array->data[index];                                                             \
}                                                                                           \
                                                                                            \
void array_##TYPE##_set(array_##TYPE* array, int index, TYPE value) {                       \
    D_ASSERT(0 <= index && index < array->count);                                           \
    array->data[index] = value;                                                             \
}                                                                                           \
                                                                                            \
                                                                                            \
void array_##TYPE##_resize(array_##TYPE* array, int new_capacity) {                         \
    int old_capacity = array->capacity;                                                     \
    array->capacity  = new_capacity;                                                        \
    array->data      = RESIZE_ARRAY(TYPE, array->data, old_capacity, array->capacity);      \
}                                                                                           \
                                                                                            \
void array_##TYPE##_free(array_##TYPE* array) {                                             \
    FREE_ARRAY(TYPE, array->data, array->capacity);                                         \
}                                                                                           \
                                                                                            \
void array_##TYPE##_clear(array_##TYPE* array) {                                            \
    array->count = 0;                                                                       \
}                                                                                           \
                                                                                            \
bool array_##TYPE##_is_empty(const array_##TYPE* array) {                                   \
    return array->count == 0;                                                               \
}                                                                                           \




//array_impl(u8)
