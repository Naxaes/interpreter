//#include "memory.h"
//#include "error.h"
//
//#define array_use(TYPE)                                                                     \
//typedef struct {                                                                            \
//    TYPE* data;                                                                             \
//    int   count;                                                                            \
//    int   capacity;                                                                         \
//} array_##TYPE;                                                                             \
//                                                                                            \
//inline array_##TYPE array_##TYPE##_make();                                                  \
//void array_##TYPE##_add(array_##TYPE* array, TYPE value);                                   \
//TYPE* array_##TYPE##_last(array_##TYPE* array);                                             \
//TYPE* array_##TYPE##_get(array_##TYPE* array, int index);                                   \
//void array_##TYPE##_set(array_##TYPE* array, int index, TYPE value);                        \
//void array_##TYPE##_resize(array_##TYPE* array, int new_capacity);                          \
//void array_##TYPE##_free(array_##TYPE* array);                                              \
//void array_##TYPE##_clear(array_##TYPE* array) ;                                            \
//bool array_##TYPE##_is_empty(const array_##TYPE* array);                                    \
//
//
//
//
//
