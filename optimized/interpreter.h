#pragma once

#include "value.h"
#include "object.h"
#include "table.h"
#include "chunk.h"


#define VM_STACK_MAX  1024


typedef struct {
    Chunk    chunk;
    uint8_t* ip;
    Value*   slots;

    Value  stack[VM_STACK_MAX];
    Value* stack_top;
    Obj*   objects;
} VM;

extern VM vm;

void vm_init();
void  vm_free();
void  vm_push(Value value);
Value vm_pop();
Value vm_peek(int x);
void  vm_interpret(const char* path, const char* source, bool quiet);
void vm_run(Chunk chunk);
