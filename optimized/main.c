#include "c-preamble/nax_preamble.h"

#define NAX_LOGGING_IMPLEMENTATION
#include "nax_logging/nax_logging.h"

#define NAX_ALLOCATOR_IMPLEMENTATION
#include "allocators/allocator.h"

#include "tokenizer.h"
#include "slice.h"
#include "memory.h"
#include "table.h"
#include "utf8.h"
#include "ast.h"
#include "parser.h"
#include "compiler.h"
#include "value.h"
#include "object.h"
#include "array.h"
#include "opcodes.h"
#include "compiler.h"
#include "interpreter.h"

#include <sys/mman.h>
#include <stdlib.h>
#include "slice.h"



u32 hash_slice(Slice key) {
    unsigned hash = 2166136261u;
    while (key.count--) {
        hash ^= (unsigned) *key.source++;
        hash *= 16777619;
    }
    return hash;
}

#define TABLE_KEY_TYPE    Slice
#define TABLE_VALUE_TYPE  Ast*
#define TABLE_KEY_HASH_FN hash_slice
#define TABLE_NAME        table_ast
#include "c_table/table.h"


//define_table(Ast*, ast)
define_table(Ast_Identifier, variable)
define_table(Type, type)
define_table(u32, u32)



//define_array(Slice)
define_array(Token)
define_dynarray(OpCode)
define_dynarray(Chunk)
define_dynarray(Location)
define_dynarray(Value)
define_dynarray(u8)
define_dynarray(Variable)
define_dynarray(Slice)
define_dynarray(Ast_Identifier)



Slice load_file(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* buffer = nax_alloc(char, (word_t)length+1);
        if (buffer) {
            fread(buffer, 1, length, file);
            buffer[length] = '\0';
            fclose(file);
            return (Slice) { .source=buffer, .count=(int)length };
        } else {
            fclose(file);
            fprintf(stderr, "Couldn't read file '%s'", file_path);
            exit(EXIT_FAILURE);
        }
    }
    fprintf(stderr, "Couldn't open file '%s'", file_path);
    exit(EXIT_FAILURE);
}


int main(int argc, const char* const argv[]) {
    int capacity = 64*64*64*64;
    void* memory = mmap(NULL, capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
    nax_assert(memory != MAP_FAILED, "Mapping Failed\n");


    const char* paths[] = {
//        "../../examples/declaration.chain",
//        "../../examples/functions.chain",
//        "../../examples/printing.chain",
//        "../../examples/program.chain",
//        "../../examples/runtime-errors.chain",
        "../../examples/scopes.chain",
        "../../examples/strings.chain",
        "../../examples/single_expression.chain",
    };

    for (int i = 0; i < (int) (sizeof(paths) / sizeof(*paths)); ++i) {
        printf("\n---- %s ----\n", paths[i]);

        StackAllocator allocator = make_stack(memory, capacity);
        Slice source = load_file(paths[i]);

        Array_Token all_tokens = tokenize(source.source, paths[i], &allocator);
        Parser parser = make_parser(source.source, paths[i], all_tokens, split_stack(allocator));

        Ast_Module* node = module(&parser, paths[i]);
        if (parser.error_count > 0) {
            for (int j = 0; j < parser.error_count; ++j) {
                print_error(parser.errors[j]);
            }
            exit(EXIT_FAILURE);
        }
        visit((Ast*) node, &parser, 0);

        Compiler compiler = compiler_make(&parser);
        compile(&compiler, node);

        Chunk chunk = *compiler.chunks.data;

        chunk_disassemble(&chunk, "<script>");

        vm_init();
        vm_run(chunk);
    }
}
