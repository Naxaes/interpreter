#include "test.h"
#include "interpreter.h"
#include "compiler.h"


TEST_SUIT_START(chain_expresssion)

    START_TEST(Simple expression)
        const char* source = "(1 + 2 + 3*4 + 5) + (6*7 - 8*9*10/11*12);";
        CHECK_EQ(INTERPRET_OK, vm_interpret(__FILE__, source));
    END_TEST

TEST_SUIT_END

