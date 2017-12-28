#include <stdio.h>
#include "malloc.h"
#include "malloc_internals.h"
#include "munit.h"

void allocate_chunk_test();
void malloc_int();

int main() {
    allocate_chunk_test();
    return 0;
}

void allocate_chunk_test() {
    printf("Test: allocate_chunk\n");
    malloc_init();
    mem_chunk_t* chunk = allocate_chunk(4000);
    
    munit_assert_int(chunk->eoc, ==, EOC);
    munit_assert_int(chunk->size, ==, 4096-56);
    munit_assert_int(chunk->ma_first.mb_size, ==, 4096-56);
    munit_assert_int(chunk->ma_first.mb_data[chunk->size / 8], ==, EOC);
    munit_assert_int( *(int64_t*)((void*)chunk + 4096 - 8), ==, EOC); // checks EOC in different way
    munit_assert(LIST_FIRST(&chunk->ma_freeblks) == &chunk->ma_first);
}

void malloc_int() {
    printf("Test: malloc_int:\n");    
    int *number = (int*) foo_malloc(sizeof(int));
    *number = 5;
    printf("%d\n", *number);
    mdump(1);
}