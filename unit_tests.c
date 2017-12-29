#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "malloc.h"
#include "malloc_internals.h"
#include "munit.h"

void allocate_chunk_test();
void trim_block_test();
void block_has_enough_space_test1();
void block_has_enough_space_test2();
void block_has_enough_space_test3();

void malloc_int();

int main() {
    allocate_chunk_test();
    trim_block_test();
    block_has_enough_space_test1();
    block_has_enough_space_test2();
    block_has_enough_space_test3();
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

    block->mb_size = 104;
    set_boundary_tag(block);
    mem_block_t* trimed_block = block_trim(block, 32);

    munit_assert_int(block->mb_size, ==, 16);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 16); // boundary tag
    munit_assert_int((uint64_t)trimed_block, ==, (uint64_t)&block->mb_data[2]);
    munit_assert_int(trimed_block->mb_size, ==, 88);
    munit_assert_int(trimed_block->mb_data[trimed_block->mb_size / 8 - 1], ==, 88);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == trimed_block);
    
    free(block);
}

void block_has_enough_space_test1() {
    /* Allignment to 8 in this test */
    printf("Test: block_has_enough_space_1 - allignment to 8\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(8, 100);
    block->mb_size = 16;

    munit_assert_int(block_has_enough_space(block, 4, 8), ==, 1);
    munit_assert_int(block_has_enough_space(block, 7, 8), ==, 1);
    munit_assert_int(block_has_enough_space(block, 8, 8), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 9, 8), ==, 0);
    munit_assert_int(block_has_enough_space(block, 16, 8), ==, 0);

    free(block);    
}

void block_has_enough_space_test2() {
    /* Allignmment to 16 */
    printf("Test: block_has_enough_space_2 - allignment to 16\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(16, 100);
    block->mb_size = 32;

    munit_assert_int(block_has_enough_space(block, 0, 16), ==, 0);
    munit_assert_int(block_has_enough_space(block, 4, 16), ==, 0);
    munit_assert_int(block_has_enough_space(block, 8, 16), ==, 0);    
    munit_assert_int(block_has_enough_space(block, 16, 16), ==, 0);

    free(block);       
}

void block_has_enough_space_test3() {
    /* Allignmment to 32 */
    printf("Test: block_has_enough_space_3 - allignment to 32\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(32, 100);
    block->mb_size = 64;

    munit_assert_int(block_has_enough_space(block, 16, 32), ==, 1);
    munit_assert_int(block_has_enough_space(block, 30, 32), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 32, 32), ==, 1);
    munit_assert_int(block_has_enough_space(block, 33, 32), ==, 0);

    free(block);       
}

void malloc_int() {
    printf("Test: malloc_int:\n");    
    int *number = (int*) foo_malloc(sizeof(int));
    *number = 5;
    printf("%d\n", *number);
    mdump(1);
}