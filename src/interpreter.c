#include "interpreter.h"
#include "opcodes.h"
#include "error.h"
#include "compiler.h"
#include "chunk.h"
#include "object.h"

#include <time.h>


#define STRING_TO_SLICE(x) ((Slice) { .source=x->data, .count=x->size })

Error vm_run(const char* path, const char* source, bool quiet);


static Value clock_native(int arg_count, Value* args) {
    if (arg_count != 0)
        return MAKE_INVALID();
    clock_t time = clock();
    return MAKE_F64((double) time / (double) CLOCKS_PER_SEC);
}

static ErrorCode define_native(const char* name, NativeFn function) {
    ObjString* string = string_make(name, (int) strlen(name));
    vm_push(MAKE_OBJ(string));
    vm_push(MAKE_OBJ(native_make(function)));
    if (!table_add(&vm.globals, STRING_TO_SLICE(AS_STRING(AS_OBJ(vm.stack[0]))), vm.stack[1])) {
        return RUNTIME_ERROR_REDEFINITION_OF_NATIVE_FUNCTION;
    }
    vm_pop();
    vm_pop();
    return NO_ERROR;
}



static void type_error_unary(const char* message, Value a);
static void type_error_binary(const char* message, Value a, Value b);
static ErrorCode call(ObjFunction* function, int arg_count);

ErrorCode call_value(Value peek, int count);

VM vm = { 0 };


#define VM_ERROR_MAKE(code_, arg_) (Error) { .path=path, .source=source, .function=(frame->function->name) ? (Slice) { .count=frame->function->name->size, .source=frame->function->name->data } : SLICE("script"), .code=code_, .start=chunk_line(&frame->function->chunk, (int) (frame->ip - frame->function->chunk.code - 2)), .count=1, .arg=arg_ }


static void runtime_error(Error error) {
    if (vm.frame_count > 1) {
        fprintf(stderr, "Stacktrace:\n");
    }
    for (int i = 0; i < vm.frame_count-1; ++i) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        Location loc = chunk_line(&function->chunk, (int) (frame->ip - function->chunk.code - 1));
        if (function->name == NULL) {
            fprintf(stderr, "    at %s:%d:%d - <script>\n", error.path, loc.row, loc.col);
        } else {
            fprintf(stderr, "    at %s:%d:%d - fun %.*s(", error.path, loc.row, loc.col, function->name->size, function->name->data);
            for (int j = 0; j < function->arity; ++j)
                if (j != function->arity-1) fprintf(stderr, "_, ");
                else fprintf(stderr, "_");
            fprintf(stderr, ")\n");
        }

        Slice line = current_line(error.source, loc.index);
        fprintf(stderr, "       %-4d| %.*s\n", loc.row, line.count, line.source);
    }

    print_error(error);

}


ErrorCode vm_init() {
    memset(vm.stack,  0, VM_STACK_MAX  * sizeof(Value));
    memset(vm.frames, 0, VM_FRAMES_MAX * sizeof(CallFrame));

    vm.stack_top = vm.stack;
    vm.objects   = NULL;
    vm.globals   = table_make();
    vm.frame_count = 0;

    return define_native("clock", clock_native);
}

void vm_free() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        object_free(object);
        object = next;
    }
    table_free(&vm.globals);
}

void vm_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

Value vm_pop() {
    vm.stack_top--;
    Value value = *vm.stack_top;
    *vm.stack_top = MAKE_INVALID();
    return value;
}

Value vm_peek(int x) {
    return *(vm.stack_top-1-x);
}

void vm_interpret(const char* path, const char* source, bool quiet) {
    ObjFunction* function = compile(path, source);

    if (function == NULL)
        return;

    vm_push(MAKE_OBJ(function));
    call(function, 0);

    Error result = vm_run(path, source, quiet);
    if (result.code != NO_ERROR) {
        runtime_error(result);
    }
}

Error vm_run(const char* path, const char* source, bool quiet) {
    CallFrame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE()     (*frame->ip++)
#define READ_SHORT()    (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define READ_STRING()   AS_STRING(AS_OBJ(READ_CONSTANT()))
#define IS_SAME(type)   (IS_## type(vm_peek(0)) && IS_ ## type(vm_peek(1)))
#define BINARY_OP(op, type) \
    do { \
      Value b = vm_pop(); \
      Value a = vm_pop(); \
      vm_push(MAKE_##type(AS_##type(a) op AS_##type(b))); \
    } while (false)

#define BINARY_RELATION(op, type) \
    do { \
      Value b = vm_pop(); \
      Value a = vm_pop(); \
      vm_push(MAKE_BOOL(AS_ ## type(a) op AS_ ## type(b))); \
    } while (false)

#define BINARY_EQUALS() \
    do { \
      Value b = vm_pop(); \
      Value a = vm_pop(); \
      vm_push(MAKE_BOOL(value_equals(a, b))); \
    } while (false)


    while (1) {
//#ifdef VM_DEBUG_TRACE_EXECUTION
        if (!quiet) {
            printf(">>         ");
            if (vm.stack >= vm.stack_top)
                printf("[ ]");
            else
                for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
                    printf("[ ");
                    print_value(*slot);
                    printf(" ]");
                }
            printf("\n>> ");
            chunk_instruction_disassemble(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
        }
//#endif

        u8 instruction;
        switch (instruction = READ_BYTE()) {
            case OP_POP:      vm_pop();                   break;
            case OP_CONSTANT: vm_push(READ_CONSTANT());   break;
            case OP_TRUE:     vm_push(MAKE_BOOL(true));   break;
            case OP_FALSE:    vm_push(MAKE_BOOL(false));  break;
            case OP_NEGATE:   {
                if      (IS_F64(vm_peek(0))) { vm_push(MAKE_F64(-AS_F64(vm_pop()))); }
                else if (IS_I64(vm_peek(0))) { vm_push(MAKE_I64(-AS_I64(vm_pop()))); }
                else type_error_unary("NEGATE", vm_peek(0));
                break;
            }
            case OP_NOT: {
                if (IS_BOOL(vm_peek(0))) vm_push(MAKE_BOOL(!AS_BOOL(vm_pop())));
                else type_error_unary("NOT", vm_peek(0));
                break;
            }
            // @TODO: AND and OR should short-circuit.
            case OP_AND: {
                if   (IS_SAME(BOOL)) { BINARY_OP(&&, BOOL); }
                else type_error_binary("AND", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_OR: {
                if   (IS_SAME(BOOL)) { BINARY_OP(||, BOOL); }
                else type_error_binary("OR", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_EQUAL: {
                if (IS_SAME(F64)) { return VM_ERROR_MAKE(RUNTIME_ERROR_UNSAFE_FLOAT_COMPARISON, SLICE("")); }
                else   { BINARY_EQUALS(); }
                break;
            }
            case OP_GREATER: {
                if       (IS_SAME(F64)) { BINARY_RELATION(>, F64); }
                else if  (IS_SAME(I64)) { BINARY_RELATION(>, I64); }
                else type_error_binary("GREATER", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_LESS: {
                if       (IS_SAME(F64)) { BINARY_RELATION(<, F64); }
                else if  (IS_SAME(I64)) { BINARY_RELATION(<, I64); }
                else type_error_binary("LESS", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_ADD: {
                if      (IS_SAME(F64)) { BINARY_OP(+, F64); }
                else if (IS_SAME(I64)) { BINARY_OP(+, I64); }
                else type_error_binary("ADD", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_SUBTRACT: {
                if      (IS_SAME(F64)) { BINARY_OP(-, F64) ; }
                else if (IS_SAME(I64)) { BINARY_OP(-, I64) ; }
                else type_error_binary("SUBTRACT", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_MULTIPLY: {
                if      (IS_SAME(F64)) { BINARY_OP(*, F64) ; }
                else if (IS_SAME(I64)) { BINARY_OP(*, I64) ; }
                else type_error_binary("MULTIPLY", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_DIVIDE: {
                if      (IS_SAME(F64)) { BINARY_OP(/, F64) ; }
                else if (IS_SAME(I64)) { BINARY_OP(/, I64) ; }
                else type_error_binary("DIVIDE", vm_peek(0), vm_peek(1));
                break;
            }
            case OP_EXIT:
                return VM_ERROR_MAKE(NO_ERROR, SLICE(""));
            case OP_PRINT: {
                print_value(vm_pop());
                printf("\n");
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                table_add(&vm.globals, (Slice) { name->data, name->size }, vm_peek(0));
                vm_pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!table_get(&vm.globals, (Slice) { name->data, name->size }, &value)) {
                    return VM_ERROR_MAKE(RUNTIME_ERROR_UNDEFINED_VARIABLE, string_to_slice(name));
                }
                vm_push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                Slice key = (Slice) { name->data, name->size };
                if (!table_set(&vm.globals, key, vm_peek(0))) {
                    table_delete(&vm.globals, key);
                    return VM_ERROR_MAKE(RUNTIME_ERROR_UNDEFINED_VARIABLE, string_to_slice(name));
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm_push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = vm_peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (value_is_falsy(vm_peek(0)))
                    frame->ip += offset;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int arg_count = READ_BYTE();
                ErrorCode result = call_value(vm_peek(arg_count), arg_count);
                if (result != NO_ERROR) {
                    static char buffer[256] = { 0 };
                    switch (result) {
                        case RUNTIME_ERROR_TOO_FEW_ARGUMENTS:
                        case RUNTIME_ERROR_TOO_MANY_ARGUMENTS: {
                            // @NOTE: Safe as these errors can only happen for valid functions.
                            ObjFunction* function = AS_FUNCTION(AS_OBJ(vm_peek(arg_count)));
                            Slice fn = string_to_slice(function->name);
                            int c = snprintf(buffer, 256, "Function %.*s(", fn.count, fn.source);
                            ASSERT(0 < c && c <= 256);
                            for (int i = 0; i < function->arity; ++i) {
                                if (i < function->arity - 1) {
                                    c += snprintf(buffer+c, 256-c, "_, ");
                                    ASSERT(0 < c && c <= 256);
                                } else {
                                    c += snprintf(buffer+c, 256-c, "_");
                                    ASSERT(0 < c && c <= 256);
                                }
                            }
                            c += snprintf(buffer+c, 256-c, ") expected %d arguments, but got %d", function->arity, arg_count);
                            Slice repr = (Slice) { .source=buffer, .count=c };
                            return VM_ERROR_MAKE(result, repr);
                        }
                        case RUNTIME_ERROR_INVALID_CALL: {
                            // @NOTE: Safe as these errors can only happen for valid functions.
                            Value value = vm_peek(arg_count);
                            const char* type = type_string(value);
                            int c = snprintf(buffer, 256, "Value is not a callable object, it's a '%s'", type);
                            ASSERT(0 < c && c <= 256);
                            Slice repr = (Slice) { .source=buffer, .count=c };
                            return VM_ERROR_MAKE(result, repr);
                        }
                        case RUNTIME_ERROR_STACK_OVERFLOW: {
                            return VM_ERROR_MAKE(result, SLICE(""));
                        }
                        default:
                            PANIC("Unhandled error %d", result);
                    }

                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_RETURN: {
                Value result = vm_pop();
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    vm_pop();
                    return VM_ERROR_MAKE(NO_ERROR, SLICE(""));
                }

                vm.stack_top = frame->slots;
                vm_push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;

            }
            case OP_NULL: {
                vm_push(MAKE_NULL());
                break;
            }
            default: {
                static char buffer[4] = { 0 };
                int c = snprintf(buffer, 4, "%d", instruction);
                ASSERT(0 < c && c <= 4);
                Slice repr = (Slice) { .source=buffer, .count=c };
                return VM_ERROR_MAKE(RUNTIME_ERROR_UNKNOWN_OP_CODE, repr);
            }
        }
    }

#undef READ_CONSTANT
#undef READ_BYTE
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
#undef IS_SAME
}


static ErrorCode call(ObjFunction* function, int arg_count) {
    if (arg_count != function->arity)
        return (arg_count > function->arity) ? RUNTIME_ERROR_TOO_MANY_ARGUMENTS: RUNTIME_ERROR_TOO_FEW_ARGUMENTS;

    if (vm.frame_count == VM_FRAMES_MAX)
        return RUNTIME_ERROR_STACK_OVERFLOW;

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack_top - arg_count - 1;
    return NO_ERROR;
}

ErrorCode call_value(Value callee, int arg_count) {
    if (IS_FUNCTION(callee)) {
        return call(AS_FUNCTION(AS_OBJ(callee)), arg_count);
    } else if (IS_NATIVE(callee)) {
        NativeFn native = AS_NATIVE(AS_OBJ(callee))->function;
        Value result = native(arg_count, vm.stack_top - arg_count);
        vm.stack_top -= arg_count + 1;
        vm_push(result);
        return NO_ERROR;
    } else {
        return RUNTIME_ERROR_INVALID_CALL;
    }
}





// ---- IMPLEMENTATION ----
__attribute__((noreturn))
static void type_error_unary(const char* message, Value a) {
    printf("[Type error]: Unary operation %s not implemented for ", message);
    print_value(a);
    printf("\n");
    error(INTERPRETER, "\n");
}

__attribute__((noreturn))
static void type_error_binary(const char* message, Value a, Value b) {
    printf("[Type error]:  Binary operation %s not implemented for ", message);
    print_type(a);
    printf(" and ");
    print_type(b);
    printf("\n");
    error(INTERPRETER, "\n");
}

