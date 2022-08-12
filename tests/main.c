#include "test.h"
#include "test_table.c"
#include "test_expression.c"



int main() {
    Option options = option_default();
    RUN_TESTS(options, table, chain_expresssion);
}
