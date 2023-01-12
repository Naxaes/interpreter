#include "interpreter.h"
#include "opcodes.h"
#include "compiler.h"
#include "chunk.h"
#include "object.h"

#include <time.h>
#include <stdlib.h>


#define STRING_TO_SLICE(x) ((Slice) { .source=x->data, .count=x->size })

VM vm = { 0 };

void vm_init() {
    memset(vm.stack,  0, VM_STACK_MAX  * sizeof(Value));

    vm.stack_top = vm.stack;
    vm.objects   = NULL;
    vm.ip = 0;
    vm.slots = malloc(1024);
}

void vm_free() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        object_free(object);
        object = next;
    }
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

}

void vm_run(Chunk chunk) {
    vm.ip = chunk.code.data;

#define READ_BYTE()     (*vm.ip++)
#define READ_SHORT()    (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_CONSTANT() (chunk.constants.data[READ_BYTE()])
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
        chunk_instruction_disassemble(&chunk, (int)(vm.ip - chunk.code.data));

        u8 instruction;
        switch (instruction = READ_BYTE()) {
            case OP_POP:      vm_pop();                   break;
            case OP_CONSTANT: vm_push(READ_CONSTANT());   break;
            case OP_TRUE:     vm_push(MAKE_BOOL(true));   break;
            case OP_FALSE:    vm_push(MAKE_BOOL(false));  break;
            case OP_NEGATE:   {
                if      (IS_F64(vm_peek(0))) { vm_push(MAKE_F64(-AS_F64(vm_pop()))); }
                else if (IS_I64(vm_peek(0))) { vm_push(MAKE_I64(-AS_I64(vm_pop()))); }
                break;
            }
            case OP_NOT: {
                if (IS_BOOL(vm_peek(0))) vm_push(MAKE_BOOL(!AS_BOOL(vm_pop())));
                break;
            }
            // @TODO: AND and OR should short-circuit.
            case OP_AND: {
                if   (IS_SAME(BOOL)) { BINARY_OP(&&, BOOL); }
                break;
            }
            case OP_OR: {
                if   (IS_SAME(BOOL)) { BINARY_OP(||, BOOL); }
                break;
            }
            case OP_EQ: {
                { BINARY_EQUALS(); }
                break;
            }
            case OP_GT: {
                if       (IS_SAME(F64)) { BINARY_RELATION(>, F64); }
                else if  (IS_SAME(I64)) { BINARY_RELATION(>, I64); }
                break;
            }
            case OP_LT: {
                if       (IS_SAME(F64)) { BINARY_RELATION(<, F64); }
                else if  (IS_SAME(I64)) { BINARY_RELATION(<, I64); }
                break;
            }
            case OP_ADD: {
                if      (IS_SAME(F64)) { BINARY_OP(+, F64); }
                else if (IS_SAME(I64)) { BINARY_OP(+, I64); }
                break;
            }
            case OP_SUB: {
                if      (IS_SAME(F64)) { BINARY_OP(-, F64) ; }
                else if (IS_SAME(I64)) { BINARY_OP(-, I64) ; }
                break;
            }
            case OP_MUL: {
                if      (IS_SAME(F64)) { BINARY_OP(*, F64) ; }
                else if (IS_SAME(I64)) { BINARY_OP(*, I64) ; }
                break;
            }
            case OP_DIV: {
                if      (IS_SAME(F64)) { BINARY_OP(/, F64) ; }
                else if (IS_SAME(I64)) { BINARY_OP(/, I64) ; }
                break;
            }
            case OP_EXIT:
                return;
            case OP_PRINT: {
                print_value(vm_pop());
                printf("\n");
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm_push(vm.slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.slots[slot] = vm_peek(0);
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
            case OP_NULL: {
                vm_push(MAKE_NULL());
                break;
            }
            default: {
                return;
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

