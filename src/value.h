#pragma once

#include "preamble.h"

typedef enum {
    OBJ_INVALID,
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};
typedef struct Obj Obj;

typedef enum {
    VALUE_INVALID,
    VALUE_NULL,

    VALUE_BOOL,
    VALUE_F64,
    VALUE_I64,
    VALUE_OBJ,
} ValueType;


typedef struct {
    union {
        bool  val_bool;
        f64   val_f64;
        i64   val_i64;
        Obj*  val_obj;
    } as;
    ValueType type;
} Value;


Value INVALID_VALUE();
Value NULL_VALUE();
Value BOOL_VALUE(bool value);
Value F64_VALUE(f64 value);
Value I64_VALUE(i64 value);
Value OBJ_VALUE_(Obj* value);
#define OBJ_VALUE(obj) (OBJ_VALUE_((Obj*) (obj)))

#define AS_BOOL(value)      ((value).as.val_bool)
#define AS_F64(value)       ((value).as.val_f64)
#define AS_I64(value)       ((value).as.val_i64)
#define AS_OBJ(value)       ((value).as.val_obj)

#define IS_INVALID(value) ((value).type == VALUE_INVALID)
#define IS_NULL(value)    ((value).type == VALUE_NULL)
#define IS_BOOL(value)    ((value).type == VALUE_BOOL)
#define IS_F64(value)     ((value).type == VALUE_F64)
#define IS_I64(value)     ((value).type == VALUE_I64)
#define IS_OBJ(value)     ((value).type == VALUE_OBJ)

#define IS_STRING(value)    is_obj_type(value, OBJ_STRING)
#define IS_FUNCTION(value)  is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value)    is_obj_type(value, OBJ_NATIVE)


void print_value(Value value);
void print_type(Value value);

bool value_equals(Value a, Value b);

bool is_obj_type(Value value, ObjType type);
bool value_is_falsy(Value value);
