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


define_table(Ast*, ast)
define_table(Ast_Identifier, variable)
define_table(Type, type)
define_table(u32, u32)
define_array(Slice)
define_array(Token)
define_dynarray(OpCode)
define_dynarray(Chunk)
define_dynarray(Location)
define_dynarray(Value)
define_dynarray(u8)
define_dynarray(Local)



char* load_file(const char* file_path, StackAllocator* allocator) {
    FILE* file = fopen(file_path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* buffer = allocate(allocator, char, length+1);
        if (buffer) {
            fread(buffer, 1, length, file);
            buffer[length] = '\0';
            fclose(file);
            return buffer;
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
    if (memory == MAP_FAILED){
        fprintf(stderr, "Mapping Failed\n");
        exit(EXIT_FAILURE);
    }


    const char* paths[] = {
//        "../../examples/token_errors.chain",
//        "../../examples/errors.chain",
//        "../../examples/declaration.chain",
//        "../../examples/functions.chain",
//        "../../examples/printing.chain",
//        "../../examples/program.chain",
//        "../../examples/runtime-errors.chain",
//        "../../examples/scopes.chain",
        "../../examples/strings.chain",
        "../../examples/single_expression.chain",
    };

    for (int i = 0; i < (int) (sizeof(paths) / sizeof(*paths)); ++i) {
        printf("\n---- %s ----\n", paths[i]);

        StackAllocator allocator = make_stack(memory, capacity);
        char* source = load_file(paths[i], &allocator);

        Array_Token all_tokens = tokenize(source, paths[i], &allocator);
        Parser parser = make_parser(source, paths[i], all_tokens, split_stack(allocator));

        Ast* node = expression_start(&parser);
        if (parser.error_count > 0) {
            for (int j = 0; j < parser.error_count; ++j) {
                print_error(parser.errors[j]);
            }
            exit(EXIT_FAILURE);
        }
        visit((Ast*) node, &parser, 0);

        Compiler compiler = compiler_make(&parser);
        visit_expression(&compiler, node);
        emit_byte(&compiler, OP_EXIT);

        Chunk chunk = *compiler.chunks.data;

        chunk_disassemble(&chunk, "<script>");

        vm_init();
        vm_run(chunk);
    }
}
