#include "interpreter.h"
#include "opcodes.h"
#include "error.h"
#include "compiler.h"
#include "chunk.h"
#include "object.h"

#include <time.h>


#define STRING_TO_SLICE(x) ((Slice) { .source=x->data, .count=x->size })


static Value clock_native(int arg_count, Value* args) {
    if (arg_count != 0)
        return NULL_VALUE();
    return F64_VALUE((double) clock() / (double) CLOCKS_PER_SEC);
}

static void define_native(const char* name, NativeFn function) {
    ObjString* string = string_make(name, (int) strlen(name));
    vm_push(OBJ_VALUE(string));
    vm_push(OBJ_VALUE(native_make(function)));
    if (!table_add(&vm.globals, STRING_TO_SLICE(AS_STRING(AS_OBJ(vm.stack[0]))), vm.stack[1])) {
        printf("ERROR!");
        exit(1);
    }
    vm_pop();
    vm_pop();
}



static void type_error_unary(const char* message, Value a);
static void type_error_binary(const char* message, Value a, Value b);
static bool call(ObjFunction* function, int arg_count);

bool call_value(Value peek, int count);

VM vm = {0 };


static void runtime_error(const char* message) {
    for (int i = vm.frame_count - 1; i >= 0; --i) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction].row);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%.*s()\n", function->name->size, function->name->data);
        }
    }
}


void vm_init() {
    memset(vm.stack,  0, VM_STACK_MAX  * sizeof(Value));
    memset(vm.frames, 0, VM_FRAMES_MAX * sizeof(CallFrame));

    vm.stack_top = vm.stack;
    vm.objects   = NULL;
    vm.globals   = table_make();
    vm.frame_count = 0;

    define_native("clock", clock_native);
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

Value vm_pop(void) {
    vm.stack_top--;
    Value value = *vm.stack_top;
    *vm.stack_top = INVALID_VALUE();
    return value;
}

Value vm_peek(int x) {
    return *(vm.stack_top-1-x);
}

InterpretResult vm_interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    vm_init();

    vm_push(OBJ_VALUE(function));
    call(function, 0);

    return vm_run();
}

InterpretResult vm_run() {
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
      vm_push(type ## _VALUE(AS_ ## type(a) op AS_ ## type(b))); \
    } while (false)

#define BINARY_RELATION(op, type) \
    do { \
      Value b = vm_pop(); \
      Value a = vm_pop(); \
      vm_push(BOOL_VALUE(AS_ ## type(a) op AS_ ## type(b))); \
    } while (false)

#define BINARY_EQUALS() \
    do { \
      Value b = vm_pop(); \
      Value a = vm_pop(); \
      vm_push(BOOL_VALUE(value_equals(a, b))); \
    } while (false)


    while (1) {
#ifdef VM_DEBUG_TRACE_EXECUTION
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
#endif

        u8 instruction;
        switch (instruction = READ_BYTE()) {
            case OP_POP:      vm_pop();                    break;
            case OP_CONSTANT: vm_push(READ_CONSTANT());    break;
            case OP_TRUE:     vm_push(BOOL_VALUE(true));   break;
            case OP_FALSE:    vm_push(BOOL_VALUE(false));  break;
            case OP_NEGATE:   {
                if      (IS_F64(vm_peek(0))) { vm_push(F64_VALUE(-AS_F64(vm_pop()))); }
                else if (IS_I64(vm_peek(0))) { vm_push(I64_VALUE(-AS_I64(vm_pop()))); }
                else type_error_unary("NEGATE", vm_peek(0));
                break;
            }
            case OP_NOT: {
                if (IS_BOOL(vm_peek(0))) vm_push(BOOL_VALUE(!AS_BOOL(vm_pop())));
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
                if       (IS_SAME(F64)) { runtime_error("Unsafe equality on floats."); }
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
                return INTERPRET_OK;
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
                    fprintf(stderr, "Undefined variable '%.*s'\n.", name->size, name->data);
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                Slice key = (Slice) { name->data, name->size };
                if (!table_set(&vm.globals, key, vm_peek(0))) {
                    table_delete(&vm.globals, key);
                    fprintf(stderr, "Undefined variable '%.*s'\n.", name->size, name->data);
                    return INTERPRET_RUNTIME_ERROR;
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
                if (!call_value(vm_peek(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_RETURN: {
                Value result = vm_pop();
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    vm_pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                vm_push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;

            }
            case OP_NULL: {
                vm_push(NULL_VALUE());
                break;
            }
            default: {
                printf("[VM]: Unknown opcode %d\n", instruction);
                break;
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


static bool call(ObjFunction* function, int arg_count) {
    if (arg_count != function->arity) {
        error(INTERPRETER, "Expected %d arguments but got %d.", function->arity, arg_count);
    }
    if (vm.frame_count == VM_FRAMES_MAX) {
        runtime_error("Stack overflow");
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack_top - arg_count - 1;
    return true;
}

bool call_value(Value callee, int arg_count) {
    if (IS_FUNCTION(callee)) {
        return call(AS_FUNCTION(AS_OBJ(callee)), arg_count);
    } else if (IS_NATIVE(callee)) {
        NativeFn native = AS_NATIVE(AS_OBJ(callee));
        Value result = native(arg_count, vm.stack_top - arg_count);
        vm.stack_top -= arg_count + 1;
        vm_push(result);
        return true;
    } else {
        runtime_error("Can only call functions and classes.");
        return false;
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

