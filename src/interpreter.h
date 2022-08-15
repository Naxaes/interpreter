#pragma once

#include "value.h"
#include "object.h"
#include "table.h"
#include "error.h"


#define VM_STACK_MAX  1024
#define VM_FRAMES_MAX 64


typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value*   slots;
} CallFrame;


typedef struct {
    CallFrame frames[VM_FRAMES_MAX];
    int frame_count;

    Value  stack[VM_STACK_MAX];
    Value* stack_top;
    Obj*   objects;
    Table  globals;
} VM;

extern VM vm;

ErrorCode vm_init();
void  vm_free();
void  vm_push(Value value);
Value vm_pop();
Value vm_peek(int x);
void  vm_interpret(const char* path, const char* source, bool quiet);
