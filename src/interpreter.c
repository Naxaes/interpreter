#include "interpreter.h"
#include "opcodes.h"
#include "error.h"

static void type_error_unary(const char* message, Value a);
static void type_error_binary(const char* message, Value a, Value b);

VM vm = { 0 };


void vm_init(Chunk* chunk) {
    memset(vm.stack, 0, VM_STACK_MAX * sizeof(Value));
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    vm.stack_top = vm.stack;
    vm.objects = NULL;
    vm.globals = table_make();
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

InterpretResult vm_interpret(Chunk* chunk) {
    vm_init(chunk);
    return vm_run();
}

InterpretResult vm_run() {
#define READ_BYTE()     (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants[READ_BYTE()])
#define READ_SHORT()    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_STRING()   AS_STRING(AS_OBJ(READ_CONSTANT()))
#define IS_SAME(type) (IS_## type(vm_peek(0)) && IS_ ## type(vm_peek(1)))
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
        chunk_instruction_disassemble(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        u8 instruction;
        switch (instruction = READ_BYTE()) {
            case STMT_RETURN: {
                print_value(vm_pop());
                printf("\n");
                break;
            }
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
                if       (IS_SAME(F64)) { error(COMPILER, "Unsafe equality on floats."); }
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
            case STMT_EXIT:
                return INTERPRET_OK;
            case STMT_PRINT: {
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
                vm_push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = vm_peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (value_is_falsy(vm_peek(0)))
                    vm.ip += offset;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                vm.ip -= offset;
                break;
            }
            default: {
                printf("Unknown opcode %d\n", instruction);
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

