#pragma once

#include "preamble.h"
#include "value.h"
#include "chunk.h"
#include "slice.h"


typedef struct {
    Obj  obj;
    int  size;
    char data[];
} ObjString;

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;


typedef Value (*NativeFn)(int arg_count, Value* args);
typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;


#define AS_STRING(object)   ((ObjString*)(object))
#define AS_CSTRING(object)  (AS_STRING(object)->data)
#define AS_FUNCTION(object) ((ObjFunction*)(object))
#define AS_NATIVE(object)   (((ObjNative*)(object)))

void print_object(Obj* obj);
void print_object_type(Obj* obj);

const char* object_type_string(Obj* obj);

void print_string(ObjString* string);
void print_function(ObjFunction* function);
void print_native(ObjNative* function);

void object_free(Obj* obj);
bool objects_equals(Obj* a, Obj* b);

Slice string_to_slice(ObjString* string);

ObjString* string_make(const char* chars, int size);
ObjFunction* function_make();
ObjNative* native_make(NativeFn function);
