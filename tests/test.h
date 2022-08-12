#pragma once

#include <stdio.h>
#include <stdbool.h>



/* ---- COLORS ----
             foreground background
    black        30         40
    red          31         41
    green        32         42
    yellow       33         43
    blue         34         44
    magenta      35         45
    cyan         36         46
    white        37         47
*/
typedef enum {
    COLOR_CODE_BLACK   = '0',
    COLOR_CODE_RED     = '1',
    COLOR_CODE_GREEN   = '2',
    COLOR_CODE_YELLOW  = '3',
    COLOR_CODE_BLUE    = '4',
    COLOR_CODE_MAGENTA = '5',
    COLOR_CODE_CYAN    = '6',
    COLOR_CODE_WHITE   = '7'
} ColorCode;

typedef enum {
    COLOR_MODIFIER_NO_MODIFIER = '0',  // Set everything back to normal.
    COLOR_MODIFIER_BOLD        = '1',  // Often a brighter shade of the same color.
    COLOR_MODIFIER_FAINT       = '2',
    COLOR_MODIFIER_ITALIC      = '3',  // Not widely supported.
    COLOR_MODIFIER_UNDERLINE   = '4',
    COLOR_MODIFIER_SLOW_BLINK  = '5',
    COLOR_MODIFIER_RAPID_BLINK = '6',
    COLOR_MODIFIER_INVERSE     = '7',  // Swap foreground and background colours.
} Modifier;

typedef enum {
    ANSI_COLOR_INDEX_MODIFIER   = 2,
    ANSI_COLOR_INDEX_FOREGROUND = 5,
    ANSI_COLOR_INDEX_BACKGROUND = 8,
} AnsiColorIndex;

typedef struct {
    char data[11];
} AnsiColor;

typedef struct {
    char data[23];
} AnsiColor24;


AnsiColor ansi_color_make(ColorCode foreground, ColorCode background, Modifier modifier) {
    AnsiColor color = { 0 };
    color.data[0] = '\033';
    color.data[1] = '[';
    color.data[2] = (char) modifier;
    color.data[3] = ';';
    color.data[4] = '3';
    color.data[5] = (char) foreground;
    color.data[6] = ';';
    color.data[7] = '4';
    color.data[8] = (char) background;
    color.data[9] = 'm';
    color.data[10] = '\0';
    return color;
}

AnsiColor24 ansi_color_make_rgb(Modifier modifier, const char background[3], const char foreground[3]) {
    AnsiColor24 color = { 0 };
    color.data[0]  = '\033';
    color.data[1]  = '[';
    color.data[2]  = (char) modifier;
    color.data[3]  = ';';
    color.data[4]  = '3';
    color.data[5]  = '8';
    color.data[6]  = ';';
    color.data[7]  = '2';
    color.data[8]  = ';';
    color.data[9]  = foreground[0];
    color.data[10] = ';';
    color.data[11] = foreground[1];
    color.data[12] = ';';
    color.data[13] = foreground[2];
    color.data[14]  = ';';
    color.data[15]  = '4';
    color.data[16] = background[0];
    color.data[17]  = ';';
    color.data[18] = background[1];
    color.data[19]  = ';';
    color.data[20] = background[2];
    color.data[21] = 'm';
    color.data[22] = '\0';
    return color;
}

void ansi_color_set_background_color(AnsiColor* color, ColorCode code) {
    color->data[ANSI_COLOR_INDEX_BACKGROUND] = (char) code;
}

void ansi_color_set_foreground_color(AnsiColor* color, ColorCode code) {
    color->data[ANSI_COLOR_INDEX_FOREGROUND] = (char) code;
}

void ansi_color_set_modifier(AnsiColor* color, Modifier code) {
    color->data[ANSI_COLOR_INDEX_MODIFIER] = (char) code;
}


#define ESC "\033"
#define CSI ESC "["

// Control sequence (Select Graphic Rendition (SGR)):  CSI n m

static const AnsiColor RED           = (AnsiColor) { CSI "0;31m" /*;40m"*/ };
static const AnsiColor BOLD_RED      = (AnsiColor) { CSI "1;31m" /*;40m"*/ };
static const AnsiColor BLUE          = (AnsiColor) { CSI "0;34m" /*;40m"*/ };
static const AnsiColor BOLD_BLUE     = (AnsiColor) { CSI "1;34m" /*;40m"*/ };
static const AnsiColor CYAN          = (AnsiColor) { CSI "0;36m" /*;40m"*/ };
static const AnsiColor BOLD_CYAN     = (AnsiColor) { CSI "1;36m" /*;40m"*/ };
static const AnsiColor GREEN         = (AnsiColor) { CSI "0;32m" /*;40m"*/ };
static const AnsiColor BOLD_GREEN    = (AnsiColor) { CSI "1;32m" /*;40m"*/ };
static const AnsiColor YELLOW        = (AnsiColor) { CSI "0;33m" /*;40m"*/ };
static const AnsiColor BOLD_YELLOW   = (AnsiColor) { CSI "1;33m" /*;40m"*/ };
static const AnsiColor NORMAL        = (AnsiColor) { CSI "0m"       };







typedef struct {
    AnsiColor failure;
    AnsiColor test_suite;
    AnsiColor test;
    AnsiColor check;
    AnsiColor other;
}  OutputColors;


typedef enum {
    OUTPUT_NOTHING      = 0 << 0,
    OUTPUT_FAILURES     = 1 << 0,
    OUTPUT_TESTS_SUITS  = 1 << 1,
    OUTPUT_TESTS        = 1 << 2,
    OUTPUT_CHECKS       = 1 << 3,
    OUTPUT_EVERYTHING   = OUTPUT_FAILURES | OUTPUT_TESTS_SUITS | OUTPUT_TESTS | OUTPUT_CHECKS,
} OutputFlag;


#define BUFFER_SIZE 4096

typedef struct {
    const char* name;
    const char* file;
    int  line;
    int  run_tests;
    int  failed_tests;
    int  checks;
    int  failures;
    bool succeeded;
    char buffer[BUFFER_SIZE];
    int  buffer_used;
    char failure_buffer[BUFFER_SIZE];
    int  failure_buffer_used;
} TestSuitInfo;

typedef struct {
    const char*  name;
    const char*  file;
    int  line;
    int  checks;
    int  failures;
    bool succeeded;
} TestInfo;

typedef struct {
    const char* file;
    int  line;
    const char* format;
    const char* lhs;
    const char* rhs;
    bool succeeded;
} CheckInfo;


struct Option;
typedef void (*PreTestSuiteFn)(const struct Option* options, TestSuitInfo* test_suit_info);
typedef void (*PostTestSuiteFn)(const struct Option* options, TestSuitInfo* test_suit_info);
typedef void (*PreTestFn)(const struct Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info);
typedef void (*PostTestFn)(const struct Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info);
typedef void (*CheckFn)(const struct Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info, const CheckInfo* check_info);

struct Option {
    OutputFlag    flags;
    FILE*         output_fd;
    FILE*         output_failure;
    OutputColors  color;

    PreTestSuiteFn  test_suite_start_output;
    PostTestSuiteFn test_suite_end_output;
    PreTestFn       test_start_output;
    PostTestFn      test_end_output;
    CheckFn         check_output;

    bool continue_test_on_failure;
    bool continue_test_suit_on_failure;
};
typedef struct Option Option;

void test_suite_start_output(const Option* options, TestSuitInfo* test_suit_info);
void test_suite_end_output(const Option* options, TestSuitInfo* test_suit_info);
void test_start_output(const Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info);
void test_end_output(const Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info);
void check_output(const Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info, const CheckInfo* check_info);

Option option_default() {
    Option options;
    options.output_fd = stdout;
    options.output_failure = stdout;
    options.color = (OutputColors) {.failure=RED, .test_suite=BLUE, .test=YELLOW, .check=GREEN, .other=CYAN};
    options.flags = OUTPUT_EVERYTHING;
    options.test_suite_start_output  = test_suite_start_output;
    options.test_suite_end_output    = test_suite_end_output;
    options.test_start_output        = test_start_output;
    options.test_end_output          = test_end_output;
    options.check_output             = check_output;
    options.continue_test_on_failure      = false;
    options.continue_test_suit_on_failure = false;
    return options;
}


#define APPEND_TO_BUFFER(x, ...) do { \
    if (x->buffer_used < BUFFER_SIZE-1)                                          \
        x->buffer_used += snprintf(x->buffer + x->buffer_used, BUFFER_SIZE - x->buffer_used - 1, __VA_ARGS__); \
    else                                  \
        x->buffer_used += snprintf(x->buffer + BUFFER_SIZE, 0, __VA_ARGS__); \
    } while(0)

#define APPEND_TO_ERR_BUFFER(x, ...) do { \
    if (x->failure_buffer_used < BUFFER_SIZE-1)                                          \
        x->failure_buffer_used += snprintf(x->failure_buffer + x->failure_buffer_used, BUFFER_SIZE - x->failure_buffer_used - 1, __VA_ARGS__); \
    else                                  \
        x->failure_buffer_used += snprintf(x->failure_buffer + BUFFER_SIZE, 0, __VA_ARGS__); \
    } while(0)


void test_suite_start_output(const Option* options, TestSuitInfo* test_suit_info) {
    if ((options->flags & OUTPUT_TESTS_SUITS) == OUTPUT_TESTS_SUITS)
        APPEND_TO_BUFFER(test_suit_info, "%s[Test '%s']:\n", options->color.test_suite.data, test_suit_info->name);
}


void test_suite_end_output(const Option* options, TestSuitInfo* test_suit_info) {
    if ((options->flags & OUTPUT_TESTS_SUITS) == OUTPUT_TESTS_SUITS) {
        if (test_suit_info->buffer_used > BUFFER_SIZE) {
            fprintf(options->output_fd,
                   "%s\n%s['%s' completed %i out of %i test%s(%i out of %i check%s]\n%s[ %d characters truncated ]\n\n%s",
                   test_suit_info->buffer,
                   (test_suit_info->succeeded) ? options->color.test_suite.data : options->color.failure.data,
                   test_suit_info->name,
                   test_suit_info->run_tests - test_suit_info->failed_tests,
                   test_suit_info->run_tests,
                   (test_suit_info->run_tests > 1) ? "s " : " ",
                   test_suit_info->checks - test_suit_info->failures,
                   test_suit_info->checks,
                   (test_suit_info->checks > 1) ? "s)" : ")",
                   options->color.other.data,
                   test_suit_info->buffer_used - BUFFER_SIZE,
                   NORMAL.data
            );
        } else {
            fprintf(options->output_fd,
                   "%s%s['%s' completed %i out of %i test%s(%i out of %i check%s]\n\n",
                   test_suit_info->buffer,
                   (test_suit_info->succeeded) ? options->color.test_suite.data : options->color.failure.data,
                   test_suit_info->name,
                   test_suit_info->run_tests - test_suit_info->failed_tests,
                   test_suit_info->run_tests,
                   (test_suit_info->run_tests > 1) ? "s " : " ",
                   test_suit_info->checks - test_suit_info->failures,
                   test_suit_info->checks,
                   (test_suit_info->checks > 1) ? "s)" : ")"
            );
        }
    }

    if ((options->flags & OUTPUT_FAILURES) == OUTPUT_FAILURES && !test_suit_info->succeeded) {
        if (test_suit_info->buffer_used > BUFFER_SIZE) {
            fprintf(options->output_failure,"%s\n%s[ %d characters truncated ]\n\n%s" , test_suit_info->failure_buffer, options->color.other.data, test_suit_info->failure_buffer_used - BUFFER_SIZE, NORMAL.data);
        } else {
            fprintf(options->output_failure,"%s\n\n" , test_suit_info->failure_buffer);
        }
    }
}

void test_start_output(const Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info) {
    if ((options->flags & OUTPUT_TESTS) == OUTPUT_TESTS)
        APPEND_TO_BUFFER(test_suit_info, "%s\t* %-32s  %s", options->color.test.data, test_info->name, options->color.check.data);
}

void test_end_output(const Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info) {
    if ((options->flags & OUTPUT_TESTS) == OUTPUT_TESTS && test_info->succeeded)
        APPEND_TO_BUFFER(test_suit_info, "  OK!\n");
    else if ((options->flags & OUTPUT_TESTS) == OUTPUT_TESTS && !test_info->succeeded)
        APPEND_TO_BUFFER(test_suit_info, "  %sFAIL!\n", options->color.failure.data);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
void check_output(const Option* options, TestSuitInfo* test_suit_info, const TestInfo* test_info, const CheckInfo* check_info) {
    if ((options->flags & OUTPUT_CHECKS) == OUTPUT_CHECKS && check_info->succeeded)
        APPEND_TO_BUFFER(test_suit_info, ".");
    else if ((options->flags & OUTPUT_CHECKS) == OUTPUT_CHECKS && !check_info->succeeded)
        APPEND_TO_BUFFER(test_suit_info, "%sf%s", options->color.failure.data, NORMAL.data);

    if ((options->flags & OUTPUT_FAILURES) == OUTPUT_FAILURES && !check_info->succeeded) {
        APPEND_TO_ERR_BUFFER(test_suit_info, "%s%s:%d: \n'%s' - '%s' failed at check %d:        ", options->color.failure.data, check_info->file, check_info->line, test_suit_info->name, test_info->name, test_info->checks);
        if (check_info->rhs)
            APPEND_TO_ERR_BUFFER(test_suit_info, check_info->format, check_info->lhs, check_info->rhs);
        else
            APPEND_TO_ERR_BUFFER(test_suit_info, check_info->format, check_info->lhs);
        APPEND_TO_ERR_BUFFER(test_suit_info, "\n");
    }
}
#pragma clang diagnostic pop


// ---------------- PUBLIC FUNCTIONS/MACROS ----------------
#define TEST_SUIT_START(test_suit_name)  \
    static TestSuitInfo test_suit_info_for_ ## test_suit_name = (TestSuitInfo) { .name=#test_suit_name, .file=__FILE__, .line=__LINE__, .run_tests=0, .failed_tests=0, .checks=0, .failures=0, .succeeded=true, .buffer={ 0 }, .buffer_used=0, .failure_buffer={0}, .failure_buffer_used=0 }; \
    bool test_suit_name(Option options) { \
        TestSuitInfo* test_suit_info = &test_suit_info_for_ ## test_suit_name;  \
        TestInfo      test_info;   \
        CheckInfo     check_info;  \
        options.test_suite_start_output(&options, test_suit_info);

#define TEST_SUIT_END options.test_suite_end_output(&options, test_suit_info); return test_suit_info->succeeded; }


#define START_TEST(test_name)  { \
    test_info = (TestInfo) { .name=#test_name, .file=__FILE__, .line=__LINE__, .checks=0, .failures=0, .succeeded=true }; \
    options.test_start_output(&options, test_suit_info, &test_info);

#define END_TEST                                                                       \
    test_suit_info->run_tests    += 1;                                                 \
    test_suit_info->failed_tests += test_info.failures > 0;                            \
    test_suit_info->checks       += test_info.checks;                                  \
    test_suit_info->failures     += test_info.failures;                                \
    test_suit_info->succeeded     = test_suit_info->succeeded && test_info.succeeded;  \
    options.test_end_output(&options, test_suit_info, &test_info);                     \
    if (!test_info.succeeded && !options.continue_test_suit_on_failure) { options.test_suite_end_output(&options, test_suit_info); return false; }; }


#define CHECK_EQ(a, b) \
    do {               \
        check_info = (CheckInfo) { .file=__FILE__, .line=__LINE__, .format="%s == %s", .lhs=#a, .rhs=#b, .succeeded=(a) == (b) }; \
        test_info.checks   += 1;                                                     \
        test_info.failures += !check_info.succeeded;                                 \
        test_info.succeeded = test_info.succeeded && check_info.succeeded;           \
        options.check_output(&options, test_suit_info, &test_info, &check_info);     \
    } while (0)


#define CHECK_TRUE(a)  \
    do {               \
        check_info = (CheckInfo) { .file=__FILE__, .line=__LINE__, .format="'%s' is true", .lhs=#a, .rhs=0, .succeeded=!!(a) }; \
        test_info.checks   += 1;                                                     \
        test_info.failures += !check_info.succeeded;                                 \
        test_info.succeeded = test_info.succeeded && check_info.succeeded;           \
        options.check_output(&options, test_suit_info, &test_info, &check_info);     \
    } while (0)


typedef bool (*TestFunction)(Option options);
void run_tests(const TestFunction* functions, int count, Option option) {
    // TODO: Allow parallelization.
    for (int i = 0; i < count; ++i) {
        functions[i](option);
    }
}
#define RUN_TESTS(option, ...) run_tests( (TestFunction[]) { __VA_ARGS__ }, sizeof((TestFunction[]) { __VA_ARGS__ }) / sizeof((TestFunction[]) { __VA_ARGS__ } [0]), options)



// @TODO: Interrupt the program to flush output status on long running processes.
//#include <pthread.h>
//#include <signal.h>
//#include <sys/time.h>
//#include <assert.h>
//
//
//
//static sigset_t block;
//void timer_handler();
//static void init( ) __attribute__((constructor));
//void init() {
//    sigemptyset(&block);
//    sigaddset(&block,SIGVTALRM);
//
//    struct sigaction act = { 0 };
//    struct timeval   interval;
//    struct itimerval period;
//
//    act.sa_handler = timer_handler;
//    assert(sigaction(SIGVTALRM, &act, NULL) == 0);
//
//    interval.tv_sec    = 0;
//    interval.tv_usec   = 10000;
//    period.it_interval = interval;
//    period.it_value    = interval;
//    setitimer(ITIMER_VIRTUAL, &period, NULL);
//}
//
//void timer_handler(int sig){
//    printf("Hello!\n");
//}
