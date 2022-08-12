#include "parser.h"
#include "chunk.h"
#include "compiler.h"
#include "interpreter.h"

#include "error.h"
#include <time.h>



static InterpretResult interpret(const char* source) {
    return vm_interpret(source);
}


static void repl() {
    char line[1024];
    while (true) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}



char* load_file(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* buffer = (char*) malloc(length + 1);
        if (buffer) {
            fread(buffer, 1, length, file);
            buffer[length] = '\0';
            fclose(file);
            return buffer;
        } else {
            fclose(file);
            error(STREAM, "Couldn't read file '%s'", file_path);
        }
    }
    error(STREAM, "Couldn't open file '%s'", file_path);
}


int main(int argc, const char* argv[]) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        const char* source = load_file(argv[1]);
        printf("===================== %*s%*s =====================\n", 10 + (int)strlen(argv[1])/2, argv[1], 10 - (int)strlen(argv[1])/2, "");
        const char* base = source;
        const char* ptr  = source;
        int line = 1;
        while (true) {
            if (*ptr == '\n') {
                ptr++;
                printf("%3d |  %.*s", line++, (int) (ptr - base), base);
                base = ptr;
            } else if (*ptr == '\0') {
                if (ptr != base)
                    printf("%3d |  %.*s\n", line++, (int) (ptr - base), base);
                break;
            } else {
                ++ptr;
            }
        }
        printf("===============================================================\n");
        clock_t begin = clock();

        interpret(source);

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("\n[Finished in %f ms.]\n", 1000.0 * time_spent);
    } else if (argc == 3) {
        if (strncmp(argv[1], "dis", 3) != 0) {
            printf("Invalid command!\n");
            return -1;
        }

        const char* source = load_file(argv[2]);
        printf("===================== %*s%*s =====================\n", 10 + (int)strlen(argv[2])/2, argv[2], 10 - (int)strlen(argv[2])/2, "");
        const char* base = source;
        const char* ptr  = source;
        int line = 1;
        while (true) {
            if (*ptr == '\n') {
                ptr++;
                printf("%3d |  %.*s", line++, (int) (ptr - base), base);
                base = ptr;
            } else if (*ptr == '\0') {
                if (ptr != base)
                    printf("%3d |  %.*s\n", line++, (int) (ptr - base), base);
                break;
            } else {
                ++ptr;
            }
        }
        printf("===============================================================\n");
        clock_t begin = clock();

        ObjFunction* script = compile(source);
        if (script) {
            chunk_disassemble(&script->chunk, argv[2]);
        } else {
            printf("[COMPILATION ERROR]\n");
        }

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("\n[Finished in %f ms.]\n", 1000.0 * time_spent);
    }

    vm_free();

    return 0;
}


