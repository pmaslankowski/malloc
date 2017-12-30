#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "malloc.h"
#include "malloc_internals.h"
#include "munit.h"

void allocate_chunk_test();
void block_has_enough_space_test1();
void block_has_enough_space_test2();
void block_has_enough_space_test3();
void block_has_enough_space_test4();
void trim_block_test1();
void trim_block_test2();
void trim_block_test3();
void block_may_be_splited_test1();
void split_block_test1();
void split_block_test2();
void give_block_from_chunk_test1();
void give_block_from_chunk_test2();

void malloc_int();

int main() {
    allocate_chunk_test();
    block_has_enough_space_test1();
    block_has_enough_space_test2();
    block_has_enough_space_test3();
    block_has_enough_space_test4();
    trim_block_test1();
    trim_block_test2();
    trim_block_test3();
    block_may_be_splited_test1();
    split_block_test1();
    split_block_test2();
    give_block_from_chunk_test1();
    give_block_from_chunk_test2();
    malloc_int();
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

void block_has_enough_space_test1() {
    /* Allignment to 8 in this test */
    printf("Test: block_has_enough_space_1 - allignment to 8\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(8, 100);
    block->mb_size = 40;

    munit_assert_int(block_has_enough_space(block, 9, 8), ==, 1);
    munit_assert_int(block_has_enough_space(block, 26, 8), ==, 1);
    munit_assert_int(block_has_enough_space(block, 32, 8), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 33, 8), ==, 0);
    munit_assert_int(block_has_enough_space(block, 40, 8), ==, 0);

    free(block);    
}

void block_has_enough_space_test2() {
    /* Allignmment to 16 */
    printf("Test: block_has_enough_space_2 - allignment to 16\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(16, 100);
    block->mb_size = 72;

    munit_assert_int(block_has_enough_space(block, 9, 16), ==, 1);
    munit_assert_int(block_has_enough_space(block, 17, 16), ==, 1);
    munit_assert_int(block_has_enough_space(block, 24, 16), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 25, 16), ==, 0);
    munit_assert_int(block_has_enough_space(block, 32, 16), ==, 0);

    free(block);       
}

void block_has_enough_space_test3() {
    /* Allignmment to 32 */
    printf("Test: block_has_enough_space_3 - allignment to 32\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(32, 100);
    block->mb_size = 88;

    munit_assert_int(block_has_enough_space(block, 9, 32), ==, 1);
    munit_assert_int(block_has_enough_space(block, 18, 32), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 24, 32), ==, 1);
    munit_assert_int(block_has_enough_space(block, 25, 32), ==, 0);
    munit_assert_int(block_has_enough_space(block, 32, 32), ==, 0);

    free(block);       
}

void block_has_enough_space_test4() {
    /* Allignmment to 64 */
    printf("Test: block_has_enough_space_4 - allignment to 64\n");
    mem_block_t* block = (mem_block_t*) aligned_alloc(64, 500);
    block->mb_size = 128;

    munit_assert_int(block_has_enough_space(block, 9, 64), ==, 1);
    munit_assert_int(block_has_enough_space(block, 30, 64), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 60, 64), ==, 1);
    munit_assert_int(block_has_enough_space(block, 64, 64), ==, 1);
    munit_assert_int(block_has_enough_space(block, 65, 64), ==, 0);

    free(block);       
}

void trim_block_test1() {
    /* Allignment - 32 */
    printf("Test: trim_block_1- allignment 32\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t* block = (mem_block_t*) aligned_alloc(32, 500);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);

    block->mb_size = 104;
    set_boundary_tag(block);
    mem_block_t* trimed_block = block_trim(block, 40, 32);

    munit_assert_int(block->mb_size, ==, 48);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 48); // boundary tag
    munit_assert_int((uint64_t)trimed_block, ==, (uint64_t)&block->mb_data[6]);
    munit_assert_int(trimed_block->mb_size, ==, 48);
    munit_assert_int(trimed_block->mb_data[trimed_block->mb_size / 8 - 1], ==, 48);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == trimed_block);
    munit_assert((uint64_t)trimed_block->mb_data % 32 == 0);

    free(block);
}

void trim_block_test2() {
    /* Allignment - 16 */
    printf("Test: trim_block_2 - allignment 16\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t* block = (mem_block_t*) aligned_alloc(16, 100);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);

    block->mb_size = 88;
    set_boundary_tag(block);
    mem_block_t* trimed_block = block_trim(block, 32, 16);

    munit_assert_int(block->mb_size, ==, 32);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 32); // boundary tag
    munit_assert_int((uint64_t)trimed_block, ==, (uint64_t)&block->mb_data[4]);
    munit_assert_int(trimed_block->mb_size, ==, 48);
    munit_assert_int(trimed_block->mb_data[trimed_block->mb_size / 8 - 1], ==, 48);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == trimed_block);
    munit_assert((uint64_t)trimed_block->mb_data % 16 == 0);
    
    free(block);
}

void trim_block_test3() {
    /* Allignment - 64 */
    printf("Test: trim_block_3 - allignment 64\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t* block = (mem_block_t*) aligned_alloc(64, 100);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);

    block->mb_size = 88;
    set_boundary_tag(block);
    mem_block_t* trimed_block = block_trim(block, 16, 64);

    munit_assert_int(block->mb_size, ==, 48);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 48); // boundary tag
    munit_assert_int((uint64_t)trimed_block, ==, (uint64_t)&block->mb_data[6]);
    munit_assert_int(trimed_block->mb_size, ==, 32);
    munit_assert_int(trimed_block->mb_data[trimed_block->mb_size / 8 - 1], ==, 32);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == trimed_block);
    munit_assert((uint64_t)trimed_block->mb_data % 64 == 0);

    free(block);
}

void block_may_be_splited_test1() {
    printf("Test: block_may_be_splited_1\n");
    mem_block_t *block = (mem_block_t*) malloc(100);
    block->mb_size = 64;

    munit_assert_int(block_may_be_splited(block, 24), ==, 1);
    munit_assert_int(block_may_be_splited(block, 29), ==, 1);
    munit_assert_int(block_may_be_splited(block, 32), ==, 1);
    munit_assert_int(block_may_be_splited(block, 33), ==, 0);
    munit_assert_int(block_may_be_splited(block, 39), ==, 0);

    free(block);    
}

void split_block_test1() {
    printf("Test: split_block_1\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t *block = (mem_block_t*) malloc(100);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);
    block->mb_size = 64;

    mem_block_t* block_left = split_block(block, 21);

    munit_assert_int(block->mb_size, ==, 32);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 32);
    munit_assert_int((uint64_t)block_left, ==, (uint64_t)&block->mb_data[4]);
    munit_assert_int(block_left->mb_size, ==, 24);
    munit_assert_int(block_left->mb_data[block_left->mb_size / 8 - 1], ==, 24);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == block_left);

    free(block);
}

void split_block_test2() { // wrong test - this block isn't splitable
    printf("Test: split_block_2\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t *block = (mem_block_t*) malloc(100);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);
    block->mb_size = 64;

    mem_block_t* block_left = split_block(block, 17);

    munit_assert_int(block->mb_size, ==, 32);
    munit_assert_int(block->mb_data[block->mb_size / 8 - 1], ==, 32);
    munit_assert_int((uint64_t)block_left, ==, (uint64_t)&block->mb_data[4]);
    munit_assert_int(block_left->mb_size, ==, 24);
    munit_assert_int(block_left->mb_data[block_left->mb_size / 8 - 1], ==, 24);
    munit_assert(LIST_FIRST(&mock_freeblks) == block);
    munit_assert(LIST_NEXT(LIST_FIRST(&mock_freeblks), mb_node) == block_left);

    free(block);
}

void give_block_from_chunk_test1() {
    printf("Test: give_block_from_chunk_1 - allignment 8\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t *block = (mem_block_t*) malloc(500);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);

    block->mb_size = 256;
    set_boundary_tag(block);

    void *addr = give_block_from_chunk(block, 104, 8);
    mem_block_t *block_left = LIST_FIRST(&mock_freeblks);

    munit_assert((void*)block->mb_data == addr);
    munit_assert_int(block->mb_size, ==, -112);
    munit_assert_int(*((uint64_t*)addr + 104 / 8), ==, 112); 
    munit_assert_int(block_left->mb_size, ==, 136);
    munit_assert_int(*(block_left->mb_data - 2), ==, 112);

    free(block);
}

void give_block_from_chunk_test2() {
    printf("Test: give_block_from_chunk_2 - allignment 16\n");
    LIST_HEAD(, mem_block) mock_freeblks = LIST_HEAD_INITIALIZER(mock_freeblks);
    LIST_INIT(&mock_freeblks);
    mem_block_t *block = (mem_block_t*) aligned_alloc(500, 16);
    LIST_INSERT_HEAD(&mock_freeblks, block, mb_node);

    block->mb_size = 256;
    set_boundary_tag(block);

    void *addr = give_block_from_chunk(block, 32, 16);
    mem_block_t *block_after_trimming = LIST_FIRST(&mock_freeblks);
    mem_block_t *block_left = LIST_NEXT(block_after_trimming, mb_node);

    munit_assert((uint64_t)addr % 16 == 0);
    munit_assert_int(block_after_trimming->mb_size, ==, 32);
    munit_assert_int(block_after_trimming->mb_data[block_after_trimming->mb_size / 8 - 1], ==, 32);
    munit_assert_int(*(int32_t*)(addr - 8), ==, -40);
    munit_assert_int(*(block_left->mb_data - 2), ==, 40);
    munit_assert_int(block_left->mb_size, ==, 256 - 88);

    free(block);
}

/* Functional tests: */
void malloc_int() {
    printf("Test: malloc_int\n");    
    int *number = (int*) foo_malloc(sizeof(int));
    *number = 5;
    
    munit_assert_int(*(uint64_t*)(number - 4), ==, EOC);
    munit_assert_int(*(uint64_t*)(number + 4), ==, 24);
    
    //mdump(1);
}