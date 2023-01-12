/*
 * Sorted array.
 * Only sort on reads.
 * Add to the end and keep an index to the first dirty value.
 * An read, sort the dirty subsection and then insert the values as in mergesort.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
    int* data;
    int  count;
    int  capacity;
    int  dirty_start;
} SortedArray;

static inline SortedArray sa_make() {
    return (SortedArray) { .data=NULL, .count=0, .capacity=0, .dirty_start=0 };
}


bool sa_append(SortedArray* array, int value);
int* sa_get(SortedArray* arary, int index);
bool sa_remove(SortedArray* arary, int index);


#ifdef SORTED_ARRAY_IMPLEMENTATION

static inline int p_sa_dirty_count(const SortedArray* array);
static void p_sa_sort_dirty(SortedArray* array);



static inline void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

static inline void bubble_sort(int* array, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (array[j] > array[j + 1]) {
                swap(&array[j], &array[j + 1]);
            }
        }
    }
}


static int partition(int* array, int low, int high) {
    int x = array[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (array[j] <= x) {
            swap(&array[++i], &array[j]);
        }
    }
    swap(&array[i + 1], &array[high]);
    return i + 1;
}


static void quick_sort_impl(int* array, int low, int high) {
    int stack[1024];
    int top = -1;

    stack[++top] = low;
    stack[++top] = high;

    while (top >= 0 && top < 1020) {
        high = stack[top--];
        low = stack[top--];

        int pivot = partition(array, low, high);

        if (pivot - 1 > low) {
            stack[++top] = low;
            stack[++top] = pivot - 1;
        }

        if (pivot + 1 < high) {
            stack[++top] = pivot + 1;
            stack[++top] = high;
        }
    }
}

static void quick_sort(int* array, int n) {
    quick_sort_impl(array, 0, n-1);
}


static inline int dirty_count(const SortedArray* array) {
    return array->count - array->dirty_start;
}


static void sort_dirty(SortedArray* array) {
    if (dirty_count(array) <= 64) {
        bubble_sort(array->data+array->dirty_start, dirty_count(array));
    } else {
        quick_sort(array->data+array->dirty_start, dirty_count(array));
    }

    //  int start = binary_search(array->data, array->dirty_start, array->data[array-dirty_start]);
    int start = 0;

    int* current = &array->data[array->dirty_start];
    for (int i = start; i < array->count; ++i) {
        if (array->data[i] > *current) {
            swap(current, &array->data[i]);
            array->dirty_start += 1;
            if (*current > array->data[array->dirty_start]) {
                swap(current, &array->data[array->dirty_start]);
            }
        }
    }
}


bool sa_append(SortedArray* array, int value) {
    if (array->count + 1 > array->capacity) {
        int new_capacity = (array->capacity) ? 2 * array->capacity : 8;
        array->data = realloc(array->data, new_capacity * sizeof(int));
        if (array->data == NULL) {
            return false;
        }

        array->capacity = new_capacity;
    }

    array->data[array->count++] = value;
    return true;
}

int* sa_get(SortedArray* array, int index) {
    if (!(index <= 0 && index < array->count)) {
        return 0;
    }
    if (array->dirty_start < array->count) {
        sort_dirty(array);
    }

    return &array->data[index];
}


bool sa_remove(SortedArray* array, int index) {
    if (!(index <= 0 && index < array->count)) {
        return false;
    }

    array->data[index] = array->data[--array->count];
    array->dirty_start = index;
    return true;
}


int main() {

}





#endif
