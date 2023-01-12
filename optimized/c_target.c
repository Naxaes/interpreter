#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>  // memcpy
#define panic(...)     (fprintf(stderr, "[PANIC]: %s:%d\: ", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), exit(errno))
#define assert(x, ...) (x) ? (void*)0 : (fprintf(stderr, "[ASSERT]: %s:%d: ", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), exit(errno), (void*)0)


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// ---- Types ----
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef float     f32;
typedef double    f64;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef u32       rune;   // Unicode code point.
typedef char      utf8;   // Unicode byte.


//// ---- Optional ----
//typedef i32 optional_type;
//struct c_optional_type_optional {
//    optional_type value; // where value != 10
//};
//static inline bool c_optional_type_is_valid(struct c_optional_type_optional optional) {
//    return optional.value != 10;
//}
//static inline optional_type c_optional_type_get_or(struct c_optional_type_optional optional, optional_type value) {
//    return c_optional_type_is_valid(optional) ? optional.value : value;
//}
//static inline optional_type c_optional_type_unsafe_get(struct c_optional_type_optional optional) {
//    assert(c_optional_type_is_valid(optional), "Unsafe get of '%s' with value '%d'", "c_optional_type_optional", optional.value);
//    return optional.value;
//}
//static inline struct c_optional_type_optional c_optional_type_make(optional_type value) {
//    return (struct c_optional_type_optional) { value };
//}
//
//
//// ---- Allocator ----
//// @NOTE: Pushing global allocators must be known at compile-time. We'll
////  calculate the minimum required size and allocate a static array to
////  contain all global allocator data. The global allocators must be
////  thread-safe. Pushing and popping is only allowed in the main thread,
////  as that'll allow us to use a stack.
////      Example:
////          @with_global_allocator(my_allocator) {
////              // Will use the thread-safe global allocator.
////              thread.make(my_function)
////              // A thread that is started within a global allocator block
////              // must be terminated within the block!
////              thread.join();
////          }
////
//// @NOTE: You can't nest the push of global allocators, as its effect is
////  global. The reason for pushing allocators is to get fallback
////  alternatives.
////
//// @NOTE: You can also push allocators to the thread-local temporary
////  storage.
////      Example:
////          @with_allocator(my_allocator) {
////              // Will use the thread-local allocator.
////              my_function();
////          }
////          // Will send the thread-local allocator to a
////          // new thread. NOTE! It's not allowed to access
////          // the allocator in the current thread after it
////          // has been sent to another thread.
////          thread.make(my_function, my_allocator);
////
//// @NOTE: Pushing thread-local allocators can be nested.
////
//// @NOTE: If the program doesn't push any global allocators, then a
////  non thread-safe allocator will be automatically pushed on the
////  global stack.
////
//// @NOTE: Each allocator is aligned to the 64 byte boundary. This
////  simplifies stack indexing and avoids false-sharing of shared
////  allocators (even if that is expected to be very minimal).
////
//typedef struct allocator_t {
//    u8 data[64];
//} allocator_t;
//
//#define ALLOCATOR_FREED_VALUE 0xCC
//
//bool allocator_is_freed(allocator_t* allocator) {
//    for (usize i = 0; i < sizeof(*allocator); ++i) {
//        if (allocator->data[i] != ALLOCATOR_FREED_VALUE) {
//            return false;
//        }
//    }
//    return true;
//}
//
//
//#define GLOBAL_ALLOCATOR_STACK_CAPACITY 8
//static allocator_t global_allocator_stack[GLOBAL_ALLOCATOR_STACK_CAPACITY]  = { 0 };
//static isize global_allocator_stack_current = -1;
//
//// @NOTE: Is thread-safe as only the main thread can create global allocators.
//static void global_allocator_stack_push(allocator_t* custom_allocator) {
//    assert(global_allocator_stack_current + 1 <= GLOBAL_ALLOCATOR_STACK_CAPACITY, "Global stack allocator capacity exceeded!");
//
//    global_allocator_stack_current += 1;
//    allocator_t* allocator = &global_allocator_stack[global_allocator_stack_current];
//
//    assert(allocator_is_freed(allocator), "Previous allocator wasn't freed!");
//    memcpy(allocator, custom_allocator, sizeof(allocator_t));
//
//    // @NOTE: The allocator is consumed.
//    memset(custom_allocator, ALLOCATOR_FREED_VALUE, sizeof(*custom_allocator));
//}
//
//// @TODO: Lock!
//// @NOTE: This function is shared between all threads.
//static void* global_allocator_stack_allocate(usize size) {
//    for (isize i = global_allocator_stack_current; i > -1; --i) {
//        void* memory = allocate(&global_allocator_stack[i], size);
//        if (memory)
//            return memory;
//    }
//    return malloc(size);
//}
//
//// @NOTE: Is thread-safe as only the main thread can remove global allocators.
//static void global_allocator_stack_remove() {
//    assert(global_allocator_stack_current >= 0, "Can't remove the system allocator.");
//    allocator_t* allocator = &global_allocator_stack[global_allocator_stack_current];
//
//    assert(!allocator_is_freed(allocator), "Allocator has already been freed!");
//    memset(allocator, ALLOCATOR_FREED_VALUE, sizeof(allocator_t));
//}
//
//
//// @NOTE: Lazily and dynamically allocated per thread.
//__thread allocator_t* thread_local_allocator_stack   =  0;
//__thread isize thread_local_allocator_stack_current  = -1;
//__thread isize thread_local_allocator_stack_capacity =  0;
//
//// @NOTE: Is thread-safe as each thread will have its own allocator stack.
//static void thread_local_allocator_push(allocator_t* custom_allocator) {
//    if (thread_local_allocator_stack_current + 1 >= thread_local_allocator_stack_capacity) {
//        isize grow_count = (thread_local_allocator_stack_current == 0) ? 8 : 2 * thread_local_allocator_stack_current;
//        thread_local_allocator_stack = (allocator_t*) global_allocator_stack_allocate(grow_count * sizeof(allocator_t));
//        assert(thread_local_allocator_stack != NULL, "Could not allocate more memory for thread allocator.");
//    }
//
//    thread_local_allocator_stack_current += 1;
//    allocator_t* allocator = &thread_local_allocator_stack[thread_local_allocator_stack_current];
//
//    assert(allocator_is_freed(allocator), "Previous allocator wasn't freed!");
//    memcpy(allocator, custom_allocator, sizeof(allocator_t));
//
//    // @NOTE: The allocator is consumed.
//    memset(custom_allocator, ALLOCATOR_FREED_VALUE, sizeof(*custom_allocator));
//}
//
//// @NOTE: Is thread-safe as each thread will have its own allocator stack.
//static void* thread_local_allocator_allocate(usize size) {
//    for (isize i = thread_local_allocator_stack_current; i > -1; --i) {
//        void* memory = allocate(&thread_local_allocator_stack[i], size);
//        if (memory)
//            return memory;
//    }
//    return global_allocator_stack_allocate(size);
//}
//
//// @NOTE: Is thread-safe as each thread will have its own allocator stack.
//static void thread_local_allocator_remove() {
//    assert(thread_local_allocator_stack_current >= 0, "No allocator to remove!");
//    allocator_t* allocator = &thread_local_allocator_stack[thread_local_allocator_stack_current--];
//
//    assert(!allocator_is_freed(allocator), "Allocator has already been freed!");
//    memset(allocator, ALLOCATOR_FREED_VALUE, sizeof(allocator_t));
//}
//
typedef struct {
    void* d;
} array;


typedef struct {
    u64 id;
    i64 join;  // > 0 == AND, < 0 == OR, abs(join) == index.
} ir_constraint;

typedef struct {
    u64 id;
    ir_constraint* constraints;
} ir_type;

typedef struct {
    ir_type type;
    char*   name;

    bool is_mutable;
    bool is_reference;
} ir_param;


void check_function_def(const char* function_name, array params) {
    // Store name in lookup table.
    // Store types and constraints for each parameter.
    // Evaluate default values and type match.
    // Evaluate statements.
    // Evaluate return value.
    //
}


void check_function_call(const char* function_name, array args) {
    // Fetch function definition
    // Check parity
    // Evaluate argument expression types.
    // Type match parameters and arguments.
    //
}







#include <pthread.h>
#include <stdio.h>


void write_color(f64 r /* where 0 <= r <= 1 */, f64 g /* where 0 <= g <= 1 */, f64 b /* where 0 <= b <= 1 */, FILE* fd) {
    int ir = (int) (255.999 * r);
    int ig = (int) (255.999 * g);
    int ib = (int) (255.999 * b);
    fprintf(fd, "%d %d %d\n", ir, ig, ib);
}

void render(int image_width /* where image_width > 1 */, int image_height /* where image_width > 1 */, FILE* fd) {
    fprintf(fd, "P3\n%d %d\n255\n", image_width, image_height);

    for (int j = image_height-1; j >= 0; --j) {
        printf("Progress: %.2f%%\r", (f64) (image_height-j) / (f64) image_height);
        for (int i = 0; i < image_width; ++i) {
            f64 r = (f64) (i) / (f64) (image_width  - 1);
            f64 g = (f64) (j) / (f64) (image_height - 1);
            f64 b = 0.25;

            if (i % j == 12)
                r *= 1.2;

            // @NOTE: It's probably too difficult to do static analysis on the input values
            //  for `write_color` (which expects all values to be between 0 and 1). Probably
            //  better to warn about it and auto-generate tests. However, since this is
            //  an internal variable only dependent on the input, we still manage to
            //  avoid having to return an error (if it's sufficiently tested) due to user
            //  error.
            write_color(r, g, b, fd);

            // It is possible to solve this if we only have ONE mutable variable in the
            // expression. If we have multiple we could find the global min and max values,
            // but they might be larger/smaller than the expression will ever reach. For
            // example f(x, y) = x/y where -1 <= x <= 1 and 1 <= y <= 2 gives min(f(x, y))
            // = -2 and max(f(x, y)) = 2, but the expression:
            //      for x in range(-1, 1) for y in range(2, 1): z = x/y
            // will only give -0.5 <= z <= 1.
            //

            // https://github.com/Z3Prover/z3/blob/master/examples/c/test_capi.c
            // Using z3, it's possible to verify whether statements might fail by AND'ing
            // wanted result with the negation of the input.
            // Example:
            //      Wanted: z == x/y and 0 <= z  when 5 <= x < 22 and y < 0
            //      Check:  Solve(z == x/y, 0 <= z, Not(And(5 <= x, x < 22, y < 0)))
            // If a solution is found, then the expression has a potential bug.
        }
    }
    printf("\nDone!\n");
}


int main() {
    const int image_width  = 256;
    const int image_height = 256;

    FILE* fd = fopen("image.ppm", "w");
    assert(fd != NULL, "Couldn't open file");
    render(image_width, image_height, fd);
    fclose(fd);

    system("open image.ppm");
}
