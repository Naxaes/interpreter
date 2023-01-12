#include "value.h"
#include "object.h"
#include "slice.h"


Value MAKE_INVALID()            { return ((Value) { { .val_bool = 0     }, make_primitive_type(0) } ); }
Value MAKE_NULL()               { return ((Value) { { .val_bool = 0     }, make_primitive_type(PrimitiveType_null)    } ); }
Value MAKE_BOOL(bool value)     { return ((Value) { { .val_bool = value }, make_primitive_type(PrimitiveType_bool)    } ); }
Value MAKE_F64(f64 value)       { return ((Value) { { .val_f64  = value }, make_primitive_type(PrimitiveType_f64)     } ); }
Value MAKE_I64(i64 value)       { return ((Value) { { .val_i64  = value }, make_primitive_type(PrimitiveType_i64)     } ); }
Value impl_MAKE_OBJ(Obj* value) { return ((Value) { { .val_obj  = value }, make_primitive_type(PrimitiveType_object)  } ); }

void print_value(Value value) {
    switch (value.type.primitive) {
        case PrimitiveType_inferred: break;

        case PrimitiveType_user_defined:
            print_object(AS_OBJ(value));
            break;

        case PrimitiveType_number:
        case PrimitiveType_u8:
        case PrimitiveType_u16:
        case PrimitiveType_u32:
        case PrimitiveType_rune:
        case PrimitiveType_u64:
        case PrimitiveType_u128:
            PANIC("Not implemented");

        case PrimitiveType_bool:
            printf(AS_BOOL(value) ? "true" : "false");
            break;

        case PrimitiveType_int:
        case PrimitiveType_i8:
        case PrimitiveType_char:
        case PrimitiveType_i16:
        case PrimitiveType_i32:
        case PrimitiveType_i64:
            printf("%lld", AS_I64(value));
            break;

        case PrimitiveType_i128:
            PANIC("Not implemented");

        case PrimitiveType_float:
        case PrimitiveType_f32:
        case PrimitiveType_f64:
            printf("%g",   AS_F64(value));
            break;

        case PrimitiveType_f16:
        case PrimitiveType_f128:
            PANIC("Not implemented");

        case PrimitiveType_object:        break;
        case PrimitiveType_string:
            printf("string");
            break;

        case PrimitiveType_function:      break;
        case PrimitiveType_null:          printf("null"); break;
        case PrimitiveTypeCount:          break;
    }
}

void print_type(Value value) {

}

const char* type_string(Value value) {
    // @TODO: FIX.
    return PrimitiveType_TYPE_NAMES[value.type.primitive].source;
}


bool value_equals(Value a, Value b) {
    // @TODO: Shouldn't require runt-time check for types.
    if (is_type(a.type, b.type))
        PANIC("Values a and b are not the same");


    return a.as.val_u64 == b.as.val_u64;
}



bool value_is_falsy(Value value) {
    return value.as.val_u64 == 0;
}
