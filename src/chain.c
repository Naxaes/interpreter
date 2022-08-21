#include "parser.h"
#include "chunk.h"
#include "compiler.h"
#include "interpreter.h"

#include "error.h"
#include <time.h>


const char* USAGE = ""
"Usage: chain [OPTIONS] <SUBCOMMAND> [ARGS]\n"
"  OPTIONS:\n"
"    -q, --quiet           Don't output anything from the compiler\n"
"    -t, --time            Output time to finish command\n"
"  SUBCOMMAND:\n"
"    com  [file]           Compile the project or a given file\n"
"    dis  <file>           Disassemble a file\n"
"    dot  <file>           Generates a dot Graphviz file\n"
"    repl                  Start the interactive session\n"
"    run  <file>           Run a file\n"
"    sim  <file>           Interpret a file\n"
"    help                  Show this output\n";


typedef enum {
    NO_RUN_MODE,
    COM,
    DIS,
    DOT,
    REPL,
    RUN,
    SIM,
    HELP,
} RunMode;


const char* RUN_MODE_STRING[] = {
        [NO_RUN_MODE] = "none",
        [COM]  = "com",
        [DIS]  = "dis",
        [DOT]  = "dot",
        [REPL] = "repl",
        [RUN]  = "run",
        [SIM]  = "sim",
        [HELP] = "help",
};


typedef struct {
    const char* working_file;
    const char* input_file;
    RunMode mode;
    bool is_quiet;
    bool take_time;
} ArgCommands;



void pretty_print_source(const char* path, const char* source);

// @TODO: Implement keywords to change the command struct
//  during interactive session (like psql).
static void repl(ArgCommands* commands) {
    char line[1024];
    vm_init();
    while (true) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        vm_interpret("repl", line, commands->is_quiet);
    }
    vm_free();
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



#define is_argument(s, x)  (memcmp(s, x, strlen(x)) == 0)


int parse_build(int argc, const char* const argv[], ArgCommands* commands) {
    if (commands->mode == NO_RUN_MODE) {
        commands->mode = COM;
    } else {
        fprintf(stderr, "'%s' is a top-level subcommand, not a subcommand for '%s'. They can't be run at the same time.\n", RUN_MODE_STRING[COM], RUN_MODE_STRING[commands->mode]);
        exit(EXIT_FAILURE);
    }
    if (argc > 0) {
        commands->input_file = argv[0];
        return 1;
    }
    return 0;
}
int parse_dis(int argc, const char* const argv[], ArgCommands* commands) {
    if (commands->mode == NO_RUN_MODE) {
        commands->mode = DIS;
    } else {
        fprintf(stderr, "'%s' is a top-level subcommand, not a subcommand for '%s'. They can't be run at the same time.\n", RUN_MODE_STRING[DIS], RUN_MODE_STRING[commands->mode]);
        exit(EXIT_FAILURE);
    }
    if (argc > 0) {
        commands->input_file = argv[0];
        return 1;
    }
    return 0;
}
int parse_dot(int argc, const char* const argv[], ArgCommands* commands) {
    if (commands->mode == NO_RUN_MODE) {
        commands->mode = DOT;
    } else {
        fprintf(stderr, "'%s' is a top-level subcommand, not a subcommand for '%s'. They can't be run at the same time.\n", RUN_MODE_STRING[DOT], RUN_MODE_STRING[commands->mode]);
        exit(EXIT_FAILURE);
    }
    if (argc > 0) {
        commands->input_file = argv[0];
        return 1;
    }
    return 0;
}
int parse_repl(int argc,const char* const argv[], ArgCommands* commands) {
    if (commands->mode == NO_RUN_MODE) {
        commands->mode = REPL;
    } else {
        fprintf(stderr, "'%s' is a top-level subcommand, not a subcommand for '%s'. They can't be run at the same time.\n", RUN_MODE_STRING[REPL], RUN_MODE_STRING[commands->mode]);
        exit(EXIT_FAILURE);
    }
    return 0;
}
int parse_run(int argc, const char* const argv[], ArgCommands* commands) {
    if (commands->mode == NO_RUN_MODE) {
        commands->mode = RUN;
    } else {
        fprintf(stderr, "'%s' is a top-level subcommand, not a subcommand for '%s'. They can't be run at the same time.\n", RUN_MODE_STRING[RUN], RUN_MODE_STRING[commands->mode]);
        exit(EXIT_FAILURE);
    }
    if (argc > 0) {
        commands->input_file = argv[0];
        return 1;
    }
    return 0;
}
int parse_sim(int argc, const char* const argv[], ArgCommands* commands) {
    if (commands->mode == NO_RUN_MODE) {
        commands->mode = SIM;
    } else {
        fprintf(stderr, "'%s' is a top-level subcommand, not a subcommand for '%s'. They can't be run at the same time.\n", RUN_MODE_STRING[SIM], RUN_MODE_STRING[commands->mode]);
        exit(EXIT_FAILURE);
    }
    if (argc > 0) {
        commands->input_file = argv[0];
        return 1;
    }
    return 0;
}


int main(int argc, const char* const argv[]) {

    if (argc == 1) {
        printf("%s", USAGE);
        exit(EXIT_SUCCESS);
    }

    ArgCommands commands = { .working_file=argv[0], .input_file=0, .mode=NO_RUN_MODE, .is_quiet=false, .take_time=false };
    argv++; argc--;
    for (int i = 0; i < argc; ++i) {
        const char* const arg = argv[i];
        if      (is_argument(arg, RUN_MODE_STRING[COM]))   {  i += parse_build(argc-i-1, argv+i+1, &commands); }
        else if (is_argument(arg, RUN_MODE_STRING[DIS]))   {  i += parse_dis(argc-i-1,   argv+i+1, &commands); }
        else if (is_argument(arg, RUN_MODE_STRING[DOT]))   {  i += parse_dot(argc-i-1,   argv+i+1, &commands); }
        else if (is_argument(arg, RUN_MODE_STRING[REPL]))  {  i += parse_repl(argc-i-1,  argv+i+1, &commands); }
        else if (is_argument(arg, RUN_MODE_STRING[RUN]))   {  i += parse_run(argc-i-1,   argv+i+1, &commands); }
        else if (is_argument(arg, RUN_MODE_STRING[SIM]))   {  i += parse_sim(argc-i-1,   argv+i+1, &commands); }
        else if (is_argument(arg, RUN_MODE_STRING[HELP]))  {  commands.mode = HELP; }
        else if (is_argument(arg, "-q") || is_argument(arg, "--quiet")) {  commands.is_quiet  = true; }
        else if (is_argument(arg, "-t") || is_argument(arg, "--time"))  {  commands.take_time = true; }
        else {
            // @TODO: Check that there are no more commands.
            fprintf(stderr, "Unknown command '%s'\n", argv[i]);
            printf("%s", USAGE);
            exit(EXIT_SUCCESS);
        }
    }

    clock_t start = (commands.take_time) ? clock() : 0;
    switch (commands.mode) {
        case NO_RUN_MODE: {
            fprintf(stderr, "No subcommand provided.");
            printf("%s", USAGE);
            break;
        } case COM: {
            ASSERTF(commands.input_file != NULL, "Not implemented");
            break;
        } case DIS: {
            // @TODO: Read from a disassembled file instead.
            ObjFunction* script = compile(commands.input_file, load_file(commands.input_file));
            if (script) {
                chunk_disassemble(&script->chunk, commands.input_file);
            } else {
                printf("[COMPILATION ERROR]\n");
            }
            break;
        } case DOT: {
            PANIC("TODO: Implement dot generation.");
            break;
        } case REPL: {
            repl(&commands);
            break;
        } case RUN: {
            PANIC("TODO: Implement loading bytecode and runing.");
        } case SIM: {
            vm_init();
            vm_interpret(commands.input_file, load_file(commands.input_file), commands.is_quiet);
            vm_free();
            break;
        } case HELP: {
            printf("%s", USAGE);
        }
    }

    if (commands.take_time) {
        clock_t stop = clock();
        double time_spent = (double)(stop - start) / CLOCKS_PER_SEC;
        printf("[Finished in %f ms]\n", 1000.0 * time_spent);
    }

    exit(EXIT_SUCCESS);
}

void pretty_print_source(const char* path, const char* source) {
    printf("===================== %*s%*s =====================\n", 10 + (int)strlen(path)/2, path, 10 - (int)strlen(path)/2, "");
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
    fflush(stdout);
}


