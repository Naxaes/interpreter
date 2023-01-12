#define declare_array(T)                                         \
typedef struct {                                                 \
    T*  data;                                                    \
    size_t count;                                                \
} Array_##T;                                                     \
Array_##T make_array_##T(T* data, size_t count);                 \
T* array_##T##_get(Array_##T* array, size_t index);              \
void array_##T##_set(Array_##T* array, size_t index, T value);


#define define_array(T)                                          \
Array_##T make_array_##T(T* data, size_t count) {                \
    return (Array_##T) { .data=data, .count=count };             \
}                                                                \
T* array_##T##_get(Array_##T* array, size_t index) {             \
    nax_assert(index < array->count);                              \
    return &array->data[index];                                  \
}                                                                \
void array_##T##_set(Array_##T* array, size_t index, T value) {  \
    nax_assert(index < array->count);                              \
    array->data[index] = value;                                  \
}



#define declare_dynarray(T)                                         \
typedef struct {                                                    \
    T*  data;                                                       \
    u32 count;                                                      \
    u32 capacity;                                                   \
} DynArray_##T;                                                     \
DynArray_##T make_dynarray_##T();                                   \
T* dynarray_##T##_get(DynArray_##T* array, u32 index);              \
void dynarray_##T##_set(DynArray_##T* array, u32 index, T value);   \
u32 dynarray_##T##_append(DynArray_##T* array, T value);


#define define_dynarray(T)                                       \
DynArray_##T make_dynarray_##T() {                               \
    return (DynArray_##T) { .data=0, .count=0 };                 \
}                                                                \
T* dynarray_##T##_get(DynArray_##T* array, u32 index) {          \
    nax_assert(index < array->count);                              \
    return &array->data[index];                                  \
}                                                                \
void dynarray_##T##_set(DynArray_##T* array, u32 index, T value) { \
    nax_assert(index < array->count);                              \
    array->data[index] = value;                                  \
}                                                                \
u32 dynarray_##T##_append(DynArray_##T* array, T value) {       \
    if (array->count + 1 > array->capacity) {                    \
        u32 old_capacity = array->capacity;                      \
        array->capacity  = GROW_CAPACITY(old_capacity);          \
        array->data      = RESIZE_ARRAY(T, array->data, old_capacity, array->capacity);  \
    }                                                            \
    array->data[array->count] = value;                           \
    return array->count++;                                       \
}



#define for_each(array, type)  u32 array##_it_index = 0; type* it = 0; for (array##_it_index = 0, it = &(array).data[array##_it_index]; array##_it_index < (array).count; ++array##_it_index, it = &(array).data[array##_it_index])



