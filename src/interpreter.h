#pragma once

#include "chunk.h"
#include "value.h"
#include "object.h"
#include "table.h"


#define VM_STACK_MAX 1024


typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;


typedef struct {
    Chunk* chunk;
    u8* ip;
    Value  stack[VM_STACK_MAX];
    Value* stack_top;
    Obj*   objects;
    Table  globals;
} VM;

extern VM vm;

void  vm_init(Chunk* chunk);
void  vm_free();
void  vm_push(Value value);
Value vm_pop(void);
Value vm_peek(int x);
InterpretResult vm_interpret(Chunk* chunk);
InterpretResult vm_run(void);
