#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "malloc.h"
#include "malloc_internals.h"
#include "munit.h"

void allocate_chunk_test();
void trim_block_test();

void malloc_int();

int main() {
    allocate_chunk_test();
    trim_block_test();
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

void trim_block_test() {
    printf("Test: trim_block\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t* block = (mem_block_t*) aligned_alloc(32, 500);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);

    block->mb_size = 100;
    set_boundary_tag(block);
    mem_block_t* trimed_block = block_trim(block, 32);

    munit_assert_int(block->mb_size, ==, 16);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 16); // boundary tag
    munit_assert_int((uint64_t)trimed_block, ==, (uint64_t)&block->mb_data[2]);
    munit_assert_int(trimed_block->mb_size, ==, 84);
    munit_assert_int(trimed_block->mb_data[trimed_block->mb_size / 8 - 1], ==, 84);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == trimed_block);
    
    free(block);
}

void malloc_int() {
    printf("Test: malloc_int:\n");    
    int *number = (int*) foo_malloc(sizeof(int));
    *number = 5;
    printf("%d\n", *number);
    mdump(1);
}