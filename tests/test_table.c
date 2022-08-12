#include "test.h"
#include "table.h"


Slice generate_key(int i) {
    static const char ids[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    char* name = malloc(12);
    int x = 0;
    int j = i;
    while (j >= 0) {
        name[x++] = ids[j % sizeof(ids)];
        j = j / (int)sizeof(ids) - 1;
    }
    return (Slice) { .source=name, .count=x };
}



TEST_SUIT_START(table)

    START_TEST(Add and check they exist)
        Table table = table_make();
        table_add(&table, SLICE("a"), I64_VALUE(0));
        table_add(&table, SLICE("b"), I64_VALUE(1));
        table_add(&table, SLICE("c"), I64_VALUE(2));
        table_add(&table, SLICE("d"), I64_VALUE(3));
        table_add(&table, SLICE("e"), I64_VALUE(4));
        table_add(&table, SLICE("f"), I64_VALUE(5));

        CHECK_EQ(table.count, 6);

        Value value;
        CHECK_TRUE(table_get(&table, SLICE("a"), &value));
        CHECK_TRUE(table_get(&table, SLICE("b"), &value));
        CHECK_TRUE(table_get(&table, SLICE("c"), &value));
        CHECK_TRUE(table_get(&table, SLICE("d"), &value));
        CHECK_TRUE(table_get(&table, SLICE("e"), &value));
        CHECK_TRUE(table_get(&table, SLICE("f"), &value));
    END_TEST

    START_TEST(Add and check other dont exist)
        Table table = table_make();
        table_add(&table, SLICE("a"), I64_VALUE(0));
        table_add(&table, SLICE("b"), I64_VALUE(1));
        table_add(&table, SLICE("c"), I64_VALUE(2));
        table_add(&table, SLICE("d"), I64_VALUE(3));
        table_add(&table, SLICE("e"), I64_VALUE(4));
        table_add(&table, SLICE("f"), I64_VALUE(5));

        CHECK_EQ(table.count, 6);

        Value value;
        CHECK_TRUE(!table_get(&table, SLICE("x"), &value));
        CHECK_TRUE(!table_get(&table, SLICE("y"), &value));
        CHECK_TRUE(!table_get(&table, SLICE("z"), &value));
        CHECK_TRUE(!table_get(&table, SLICE("w"), &value));
    END_TEST

    START_TEST(Add and remove)
        Table table = table_make();
        table_add(&table, SLICE("a"), I64_VALUE(0));
        table_add(&table, SLICE("b"), I64_VALUE(1));
        table_add(&table, SLICE("c"), I64_VALUE(2));

        CHECK_EQ(table.count, 3);

        Value value;
        CHECK_TRUE(table_get(&table, SLICE("a"), &value));
        CHECK_TRUE(table_get(&table, SLICE("b"), &value));
        CHECK_TRUE(table_get(&table, SLICE("c"), &value));

        CHECK_TRUE(!slice_is_empty(table_delete(&table, SLICE("b"))));
        CHECK_TRUE(!slice_is_empty(table_delete(&table, SLICE("c"))));
        CHECK_TRUE(!slice_is_empty(table_delete(&table, SLICE("a"))));

        CHECK_EQ(table.count, 0);

        CHECK_TRUE(!table_get(&table, SLICE("a"), &value));
        CHECK_TRUE(!table_get(&table, SLICE("b"), &value));
        CHECK_TRUE(!table_get(&table, SLICE("c"), &value));
    END_TEST


    START_TEST(Test stuff with many)
        Table table = table_make();

        const int size = 1000000;
        for (int i = 0; i < size; ++i) {
            Slice key = generate_key(i);
            table_add(&table, key, I64_VALUE(i));
        }

        CHECK_EQ(table.count, size);

        OutputFlag flag = options.flags;
        options.flags = OUTPUT_NOTHING;
        int n = 0;
        for (int i = 13; i < size; i += 7) {
            Slice my_key  = generate_key(i);
            Slice the_key = table_delete(&table, my_key);
            CHECK_TRUE(slice_equals(my_key, the_key));
            ++n;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
            free((void*)my_key.source);
            free((void*)the_key.source);
#pragma clang diagnostic pop
        }
        options.flags = flag;

        CHECK_EQ(table.count, size-n);

    END_TEST



TEST_SUIT_END

