#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

int entry_count = 0;

typedef struct {
    const char* name;
    int id;
} Entry;


Entry* load_file(const char* file_path) {
    FILE*   file = fopen(file_path, "r");
    char*   line = NULL;
    size_t  len = 0;
    ssize_t read = 0;

    Entry* entries = (Entry*) malloc(8 * 4096);

    if (file) {
        int _, id;
        char name[128];
        while ((read = getline(&line, &len, file)) != -1) {
            sscanf(line, "%d, %d, %s", &_, &id, &name);

            len = strlen(name);
            char* temp = (char*)malloc(len+1);
            temp[len] = '\0';
            strncpy(temp, name, len+1);
            entries[entry_count++] = (Entry) { .name=temp, .id=id };
        }
    }

    entries[entry_count] = (Entry) { 0, 0 };
    return entries;
}



static uint32_t str_hash(const char* key) {
    uint32_t hash = 2166136261u;
    while (*key++ != 0) {
        hash ^= *key;
        hash *= 16777619;
    }
    return hash;
}

static bool str_compare(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}


#define TABLE_IMPLEMENTATION
#define TABLE_KEY                   const char *
#define TABLE_HASH(k)               str_hash(k)
#define TABLE_KEY_COMPARE(a, b)     str_compare(a, b)
#define TABLE_VALUE                 int
#include "table.h"
#undef TABLE_KEY
#undef TABLE_HASH
#undef TABLE_KEY_COMPARE
#undef TABLE_VALUE
#undef TABLE_IMPLEMENTATION




#include <sys/time.h>
uint64_t time_stamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
}



int main() {

    Entry* entries = load_file("../../../optimized/table/pokedex.txt");

    Table table = table_make();

    uint64_t start = time_stamp();
    for (int i = 0; i < entry_count; ++i) {
        Entry entry = entries[i];
        table_set(&table, entry.name, entry.id);
    }
    uint64_t stop = time_stamp();
    printf("Time: %g ms\n", (double)(stop-start) / 1000.0);


    printf("%d == 1\n",   *table_get(&table, "Bulbasaur,"));
    printf("%d == 603\n", *table_get(&table, "Tynamo,"));
    printf("%d == 536\n", *table_get(&table, "Tympole,"));
    printf("%d == 172\n", *table_get(&table, "Pichu,"));

    table_delete(&table, "Bulbasaur,");
    table_delete(&table, "Tynamo,");
    table_delete(&table, "Tympole,");
    table_delete(&table, "Pichu,");

    printf("Bulbasaur - %p\n",  (void*)table_get(&table, "Bulbasaur,"));
    printf("Tynamo    - %p\n",  (void*)table_get(&table, "Tynamo,"));
    printf("Tympol    - %p\n",  (void*)table_get(&table, "Tympole,"));
    printf("Pichu     - %p\n",  (void*)table_get(&table, "Pichu,"));

    table_set(&table, "Bulbasaur,", 1);
    table_set(&table, "Tynamo,", 1);
    table_set(&table, "Tympole,", 1);
    table_set(&table, "Pichu,", 1);

    printf("%d == 1\n", *table_get(&table, "Bulbasaur,"));
    printf("%d == 1\n", *table_get(&table, "Tynamo,"));
    printf("%d == 1\n", *table_get(&table, "Tympole,"));
    printf("%d == 1\n", *table_get(&table, "Pichu,"));

    return 0;
}
