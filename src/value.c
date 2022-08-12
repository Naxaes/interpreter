#include "value.h"
#include "error.h"
#include "object.h"


Value INVALID_VALUE()         { return ((Value) { { .val_bool = 0     }, VALUE_INVALID } ); }
Value NULL_VALUE()            { return ((Value) { { .val_bool = 0     }, VALUE_NULL    } ); }
Value BOOL_VALUE(bool value)  { return ((Value) { { .val_bool = value }, VALUE_BOOL    } ); }
Value F64_VALUE(f64 value)    { return ((Value) { { .val_f64  = value }, VALUE_F64     } ); }
Value I64_VALUE(i64 value)    { return ((Value) { { .val_i64  = value }, VALUE_I64     } ); }
Value OBJ_VALUE_(Obj* value)  { return ((Value) { { .val_obj  = value }, VALUE_OBJ     } ); }

void print_value(Value value) {
    switch (value.type) {
        case VALUE_NULL:    printf("null"); break;
        case VALUE_BOOL:    printf(AS_BOOL(value) ? "true" : "false");  break;
        case VALUE_F64:     printf("%g",   AS_F64(value));   break;
        case VALUE_I64:     printf("%lld", AS_I64(value));   break;
        case VALUE_OBJ:     print_object(AS_OBJ(value));     break;
        case VALUE_INVALID: error(INTERPRETER, "Value is invalid");
    }
}

void print_type(Value value) {
    switch (value.type) {
        case VALUE_NULL:    printf("null");      break;
        case VALUE_BOOL:    printf("bool");      break;
        case VALUE_F64:     printf("f64");       break;
        case VALUE_I64:     printf("i64");       break;
        case VALUE_OBJ:     print_object_type(AS_OBJ(value)); break;
        case VALUE_INVALID: error(INTERPRETER, "Value is invalid");
    }
}


bool value_equals(Value a, Value b) {
    if (a.type != b.type)
        error(INTERPRETER, "Values a and b are not the same");
    
    switch (a.type) {
        case VALUE_NULL: return true; 
        case VALUE_BOOL: return AS_BOOL(a) == AS_BOOL(b); 
        case VALUE_F64:  error(INTERPRETER, "Unsafe comparison between f64.");
        case VALUE_I64:  return AS_I64(a) == AS_I64(b);
        case VALUE_OBJ:  return objects_equals(AS_OBJ(a), AS_OBJ(b));
        case VALUE_INVALID: error(INTERPRETER, "Value is invalid");
    }
}


inline bool is_obj_type(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

bool value_is_falsy(Value value) {
    switch (value.type) {
        case VALUE_NULL:    return false;
        case VALUE_BOOL:    return AS_BOOL(value) == false;
        case VALUE_F64:     return AS_F64(value) == 0.0;
        case VALUE_I64:     return AS_I64(value) == 0;
        case VALUE_OBJ:     return AS_OBJ(value) == NULL;
        case VALUE_INVALID: error(INTERPRETER, "Value is invalid");
    }
}
