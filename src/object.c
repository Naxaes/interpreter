#include "object.h"
#include "error.h"
#include "memory.h"


static Obj* make_obj(Obj* obj, ObjType type) { obj->type = type; return obj; }
#define ALLOCATE_OBJ(class_, type) ((class_*) make_obj(malloc(sizeof(class_)), type))


void print_object(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:   print_string(AS_STRING(obj));     break;
        case OBJ_FUNCTION: print_function(AS_FUNCTION(obj)); break;
        case OBJ_NATIVE:   print_native(AS_NATIVE(obj));    break;
        case OBJ_INVALID:  error(INTERPRETER, "<INVALID>");
    }
}


void print_string(ObjString* string) {
    printf("%.*s", string->size, string->data);
}

void print_function(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
    } else {
       printf("<fn %.*s>", function->name->size, function->name->data);
    }
}

void print_native(ObjNative* function) {
    printf("<native fn>");
}



void print_object_type(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING:   printf("String");   break;
        case OBJ_FUNCTION: printf("Function"); break;
        case OBJ_NATIVE:   printf("Native");   break;
        case OBJ_INVALID:  error(INTERPRETER, "<INVALID>");
    }
}

void object_free(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString* string = AS_STRING(obj);
            FREE_RAW(string, sizeof(ObjString) + string->size * sizeof(char));
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = AS_FUNCTION(obj);
            chunk_free(&function->chunk);
            FREE(ObjFunction, obj);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, obj);
            break;
        }
        case OBJ_INVALID:  error(INTERPRETER, "<INVALID>");
    }
}

bool objects_equals(Obj* a, Obj* b) {
    if (a->type != b->type)
        error(INTERPRETER, "Objects a and b are not the same");

    switch (a->type) {
        case OBJ_STRING: {
            ObjString* string_a = AS_STRING(a);
            ObjString* string_b = AS_STRING(b);
            return string_a->size == string_b->size &&
            memcmp(string_a->data, string_b->data, string_a->size) == 0;
        }
        case OBJ_FUNCTION:
        case OBJ_NATIVE:  return a == b;
        case OBJ_INVALID:
            error(INTERPRETER, "Objects a and b are invalid");
    }
}


// @TODO: Remove null terminator?
ObjString* string_make(const char* chars, int size) {
    ObjString* string = (ObjString*) malloc(sizeof(ObjString) + size * sizeof(char) + 1);
    string->obj.type  = OBJ_STRING;
    string->size      = size;
    memcpy(string->data, chars, size);
    string->data[size] = '\0';
    return string;
}


ObjFunction* function_make() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name  = NULL;
    function->chunk = chunk_make();
    return function;
}


ObjNative* native_make(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}
