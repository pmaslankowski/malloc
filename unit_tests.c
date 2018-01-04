#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "munit.h"
#include "malloc.h"
#include "malloc_internals.h"
#include "munit.h"



static MunitResult allocate_chunk_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);
    malloc_init();
    mem_chunk_t* chunk = allocate_chunk(4000);
    
    munit_assert_int(chunk->eoc, ==, EOC);
    munit_assert_int(chunk->size, ==, 4096-56);
    munit_assert_int(chunk->ma_first.mb_size, ==, 4096-56);
    munit_assert_int(chunk->ma_first.mb_data[chunk->size / 8], ==, EOC);
    munit_assert_int( *(int64_t*)((void*)chunk + 4096 - 8), ==, EOC); // checks EOC in different way
    munit_assert(LIST_FIRST(&chunk->ma_freeblks) == &chunk->ma_first);

    return MUNIT_OK;
}

static MunitResult block_has_enough_space_test1(const MunitParameter params[], void *data) {
    /* Allignment to 8 in this test */
    UNUSED(params); UNUSED(data);
    mem_block_t* block = (mem_block_t*) aligned_alloc(8, 100);
    block->mb_size = 40;

    munit_assert_int(block_has_enough_space(block, 9, 8), ==, 1);
    munit_assert_int(block_has_enough_space(block, 26, 8), ==, 1);
    munit_assert_int(block_has_enough_space(block, 32, 8), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 33, 8), ==, 0);
    munit_assert_int(block_has_enough_space(block, 40, 8), ==, 0);

    free(block);    

    return MUNIT_OK;
}

static MunitResult block_has_enough_space_test2(const MunitParameter params[], void *data) {
    /* Allignmment to 16 */
    UNUSED(params); UNUSED(data);
    mem_block_t* block = (mem_block_t*) aligned_alloc(16, 100);
    block->mb_size = 72;

    munit_assert_int(block_has_enough_space(block, 9, 16), ==, 1);
    munit_assert_int(block_has_enough_space(block, 17, 16), ==, 1);
    munit_assert_int(block_has_enough_space(block, 24, 16), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 25, 16), ==, 0);
    munit_assert_int(block_has_enough_space(block, 32, 16), ==, 0);

    free(block);      

    return MUNIT_OK; 
}

static MunitResult block_has_enough_space_test3(const MunitParameter params[], void *data) {
    /* Allignmment to 32 */
    UNUSED(params); UNUSED(data);
    mem_block_t* block = (mem_block_t*) aligned_alloc(32, 100);
    block->mb_size = 88;

    munit_assert_int(block_has_enough_space(block, 9, 32), ==, 1);
    munit_assert_int(block_has_enough_space(block, 18, 32), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 24, 32), ==, 1);
    munit_assert_int(block_has_enough_space(block, 25, 32), ==, 0);
    munit_assert_int(block_has_enough_space(block, 32, 32), ==, 0);

    free(block);       

    return MUNIT_OK;
}

static MunitResult block_has_enough_space_test4(const MunitParameter params[], void *data) {
    /* Allignmment to 64 */
    UNUSED(params); UNUSED(data);
    mem_block_t* block = (mem_block_t*) aligned_alloc(64, 500);
    block->mb_size = 128;

    munit_assert_int(block_has_enough_space(block, 9, 64), ==, 1);
    munit_assert_int(block_has_enough_space(block, 30, 64), ==, 1);    
    munit_assert_int(block_has_enough_space(block, 60, 64), ==, 1);
    munit_assert_int(block_has_enough_space(block, 64, 64), ==, 1);
    munit_assert_int(block_has_enough_space(block, 65, 64), ==, 0);

    free(block);     

    return MUNIT_OK;  
}

static MunitResult trim_block_test1(const MunitParameter params[], void *data) {
    /* Allignment - 32 */
    UNUSED(params); UNUSED(data);    
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

    return MUNIT_OK;
}

static MunitResult trim_block_test2(const MunitParameter params[], void *data) {
    /* Allignment - 16 */
    UNUSED(params); UNUSED(data);    
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
    
    return MUNIT_OK;
}

static MunitResult trim_block_test3(const MunitParameter params[], void *data) {
    /* Allignment - 64 */
    UNUSED(params); UNUSED(data);    
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

    return MUNIT_OK;
}

static MunitResult block_may_be_splited_test1(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);        
    mem_block_t *block = (mem_block_t*) malloc(100);
    block->mb_size = 64;

    munit_assert_int(block_may_be_splited(block, 24), ==, 1);
    munit_assert_int(block_may_be_splited(block, 29), ==, 0);
    munit_assert_int(block_may_be_splited(block, 32), ==, 0);
    munit_assert_int(block_may_be_splited(block, 33), ==, 0);
    munit_assert_int(block_may_be_splited(block, 39), ==, 0);

    free(block);  

    return MUNIT_OK; 
}

static MunitResult split_block_test1(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);        
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

    return MUNIT_OK;
}

static MunitResult split_block_test2(const MunitParameter params[], void *data) { 
    UNUSED(params); UNUSED(data);        
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

    return MUNIT_OK;
}

static MunitResult give_block_from_chunk_test1(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);            
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

    return MUNIT_OK;
}

static MunitResult give_block_from_chunk_test2(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);            
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

    return MUNIT_OK;
}

static MunitResult has_lower_block_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);                
    void *addr = malloc(500);
    *(uint64_t*)addr = EOC;
    mem_block_t *block1 = (mem_block_t*) (addr+8);
    block1->mb_size = 128;
    set_boundary_tag(block1);
    mem_block_t *block2 = (mem_block_t*) (addr + 8 + MEM_BLOCK_OVERHEAD + 128);
    block2->mb_size = 104;
    set_boundary_tag(block2);
    
    munit_assert_int(has_lower_block(block1), ==, 0);
    munit_assert_int(has_lower_block(block2), ==, 1);

    free(addr);

    return MUNIT_OK;
}

static MunitResult has_higher_block_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);                
    void *addr = malloc(500);
    *(uint64_t*)addr = EOC;
    mem_block_t *block1 = (mem_block_t*) (addr+8);
    block1->mb_size = 128;
    set_boundary_tag(block1);
    mem_block_t *block2 = (mem_block_t*) (addr + 8 + MEM_BLOCK_OVERHEAD + 128);
    block2->mb_size = 104;
    set_boundary_tag(block2);
    block2->mb_data[104/8] = EOC;
    
    munit_assert_int(has_higher_block(block1), ==, 1);
    munit_assert_int(has_higher_block(block2), ==, 0);

    free(addr);

    return MUNIT_OK;
}

static MunitResult get_lower_block_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);                
    void *addr = malloc(500);
    *(uint64_t*)addr = EOC;
    mem_block_t *block1 = (mem_block_t*) (addr+8);
    block1->mb_size = 128;
    set_boundary_tag(block1);
    mem_block_t *block2 = (mem_block_t*) (addr + 8 + MEM_BLOCK_OVERHEAD + 128);
    block2->mb_size = 104;
    set_boundary_tag(block2);
    
    mem_block_t *lower_block = get_lower_block(block2);
    
    munit_assert(lower_block == block1);

    free(addr);

    return MUNIT_OK;
}

static MunitResult get_higher_block_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);                
    void *addr = malloc(500);
    *(uint64_t*)addr = EOC;
    mem_block_t *block1 = (mem_block_t*) (addr+8);
    block1->mb_size = 128;
    set_boundary_tag(block1);
    mem_block_t *block2 = (mem_block_t*) (addr + 8 + MEM_BLOCK_OVERHEAD + 128);
    block2->mb_size = 104;
    set_boundary_tag(block2);
    block2->mb_data[104/8] = EOC;
    
    mem_block_t *higher_block = get_higher_block(block1);

    munit_assert(higher_block == block2);

    free(addr);

    return MUNIT_OK;
}

static MunitResult is_addr_in_chunk_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    mem_chunk_t *chunk = malloc(128 + MEM_CHUNK_OVERHEAD);
    chunk->size = 128;

    munit_assert_int(is_addr_in_chunk(chunk, chunk->ma_first.mb_data), ==, 1);
    munit_assert_int(is_addr_in_chunk(chunk, chunk->ma_first.mb_data + 64 / 8), ==, 1);
    munit_assert_int(is_addr_in_chunk(chunk, chunk->ma_first.mb_data + 128 / 8 - 1), ==, 1);
    munit_assert_int(is_addr_in_chunk(chunk, chunk->ma_first.mb_data + 128 / 8), ==, 0);
    munit_assert_int(is_addr_in_chunk(chunk, chunk->ma_first.mb_data + 160 / 8), ==, 0);

    free(chunk);

    return MUNIT_OK;
}

static MunitResult compare_block_sizes_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    printf("Test: compare_block_sizes\n");
    mem_block_t *block = (mem_block_t*) malloc(100);    
    block->mb_size = 40;
    set_boundary_tag(block);
    block->mb_size *= -1;

    munit_assert(compare_block_sizes(block, 24) > 0);
    munit_assert(compare_block_sizes(block, 30) > 0);
    munit_assert(compare_block_sizes(block, 32) == 0);
    munit_assert(compare_block_sizes(block, 33) < 0);
    munit_assert(compare_block_sizes(block, 40) < 0);

    free(block);

    return MUNIT_OK;
}

static MunitResult shrink_block_test1(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    printf("Test: shrink_block_1\n");
    mem_chunk_t *chunk = allocate_chunk(4000);
    mem_block_t *block1 = (mem_block_t*)(give_block_from_chunk(&chunk->ma_first, 64, 8) - MEM_BLOCK_OVERHEAD);
    mem_block_t *block2 = (mem_block_t*)(give_block_from_chunk(LIST_FIRST(&chunk->ma_freeblks), 32, 8) - MEM_BLOCK_OVERHEAD);

    shrink_block(block1, 32);

    munit_assert_int(block1->mb_size, ==, -40);
    munit_assert_int(block1->mb_data[-block1->mb_size / 8 -1], ==, 40);
    mem_block_t *block_free = LIST_FIRST(&chunk->ma_freeblks);
    munit_assert((void*)block_free == (void*)(block1->mb_data + (-block1->mb_size / 8)));
    munit_assert_int(block_free->mb_size, ==, 24);
    munit_assert_int(block_free->mb_data[block_free->mb_size / 8 - 1], ==, 24);
    munit_assert((void*)block2 == (void*)(block_free->mb_data + block_free->mb_size / 8));
    munit_assert_int(block2->mb_size, ==, -40);
    munit_assert_int(block2->mb_data[-block2->mb_size / 8 - 1], ==, 40);
    mem_block_t *block_free2 = LIST_NEXT(block_free, mb_node);
    munit_assert((void*)block_free2 == (void*)(block2->mb_data + (-block2->mb_size / 8)));
    munit_assert_int(block_free2->mb_size, ==, 4040 - 128);
    munit_assert_int(block_free2->mb_data[block_free2->mb_size / 8 - 1], ==, 4040 - 128);

    return MUNIT_OK;
}

static MunitResult shrink_block_test2(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    printf("Test: shrink_block_2\n");
    malloc_init();
    mem_chunk_t *chunk = allocate_chunk(4000);
    mem_block_t *block1 = (mem_block_t*)(give_block_from_chunk(&chunk->ma_first, 64, 8) - MEM_BLOCK_OVERHEAD);

    shrink_block(block1, 24);

    munit_assert_int(block1->mb_size, ==, -32);
    munit_assert_int(block1->mb_data[-block1->mb_size / 8 -1], ==, 32);
    mem_block_t *block_free = LIST_FIRST(&chunk->ma_freeblks);
    munit_assert((void*)block_free == (void*)(block1->mb_data + (-block1->mb_size / 8)));
    munit_assert_int(block_free->mb_size, ==, 4000);
    munit_assert_int(block_free->mb_data[block_free->mb_size / 8 - 1], ==, 4000);

    return MUNIT_OK;
}

static MunitResult is_block_extension_possible_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);      
    void *addr = malloc(100);
    mem_block_t *block = (mem_block_t*) addr;    
    block->mb_size = 40;
    set_boundary_tag(block);
    block->mb_size *= -1;
    mem_block_t *higher_block = (mem_block_t*) (addr + 48);
    higher_block->mb_size = 32;
    set_boundary_tag(higher_block);

    munit_assert_int(is_block_extension_possible(block, 48), ==, 1);
    munit_assert_int(is_block_extension_possible(block, 51), ==, 1);
    munit_assert_int(is_block_extension_possible(block, 71), ==, 1);
    munit_assert_int(is_block_extension_possible(block, 72), ==, 1);
    munit_assert_int(is_block_extension_possible(block, 73), ==, 0);
    munit_assert_int(is_block_extension_possible(block, 85), ==, 0);

    free(addr);
    
    return MUNIT_OK;    
}

static MunitResult is_extension_with_split_possible_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);      
    void *addr = malloc(200);
    mem_block_t *block = (mem_block_t*) addr;    
    block->mb_size = 40;
    set_boundary_tag(block);
    block->mb_size *= -1;
    mem_block_t *higher_block = (mem_block_t*) (addr + 48);
    higher_block->mb_size = 64;
    set_boundary_tag(higher_block);

    munit_assert_int(is_extension_with_split_possible(block, 48), ==, 1);
    munit_assert_int(is_extension_with_split_possible(block, 70), ==, 1);
    munit_assert_int(is_extension_with_split_possible(block, 72), ==, 1);
    munit_assert_int(is_extension_with_split_possible(block, 73), ==, 0);
    munit_assert_int(is_extension_with_split_possible(block, 80), ==, 0);
    

    free(addr);

    return MUNIT_OK;    
}

static MunitResult extend_block_without_split_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);      
    malloc_init();
    mem_chunk_t *chunk = allocate_chunk(4000);
    mem_block_t *block1 = (mem_block_t*)(give_block_from_chunk(&chunk->ma_first, 64, 8) - MEM_BLOCK_OVERHEAD);

    extend_block_without_split(block1, 120);

    munit_assert_int(block1->mb_size, ==, -4040);
    munit_assert_int(block1->mb_data[-block1->mb_size / 8 -1], ==, 4040);
    munit_assert(LIST_EMPTY(&chunk->ma_freeblks));

    return MUNIT_OK;    
}

static MunitResult extend_block_with_split_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);      
    malloc_init();
    mem_chunk_t *chunk = allocate_chunk(4000);
    mem_block_t *block1 = (mem_block_t*)(give_block_from_chunk(&chunk->ma_first, 64, 8) - MEM_BLOCK_OVERHEAD);

    extend_block_with_split(block1, 120);

    munit_assert_int(block1->mb_size, ==, -128);
    munit_assert_int(block1->mb_data[-block1->mb_size / 8 -1], ==, 128);
    mem_block_t *block2 = LIST_FIRST(&chunk->ma_freeblks);
    munit_assert_ptr_equal(block1->mb_data - block1->mb_size / 8, block2);
    munit_assert_int(block2->mb_size, ==, 4040 - 136);
    munit_assert_int(block2->mb_data[block2->mb_size / 8 - 1], ==, 4040 - 136);

    return MUNIT_OK;    
}






/* Functional tests: */
static MunitResult malloc_int(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);   
    malloc_init();  
    int *number = (int*) foo_malloc(sizeof(int));
    *number = 5;
    
    munit_assert_int(*(uint64_t*)(number - 4), ==, EOC);
    munit_assert_int(*(uint64_t*)(number + 4), ==, 24);
    
    return MUNIT_OK;
}

static MunitResult posix_memalign_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    void *addr;
    int res = foo_posix_memalign(&addr, 16, 4000);

    munit_assert_int(res, ==, 0);
    munit_assert((uint64_t) addr % 16 == 0);
    munit_assert_int(*(int32_t*)(addr - 8), ==, -4008);
    
    return MUNIT_OK;
}

static MunitResult free_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    malloc_init();
    void *addr1 = foo_malloc(4);
    void *addr2 = foo_malloc(4);
    foo_free(addr2);
    foo_free(addr1);
    mdump(1);

    return MUNIT_OK;
}

static MunitResult calloc_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);
    void *addr = foo_calloc(4, 8);

    for(int i=0; i < 32; i += 8)
        munit_assert_int(*(uint64_t*)(addr + i), ==, 0);
    
    foo_free(addr);

    return MUNIT_OK;
}

static MunitResult realloc_test1(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);
    malloc_init();
    int *tab = foo_malloc(20 * sizeof(int));
    for(int i=0; i < 20; i++)
        tab[i] = i;
    int *placeholder = foo_malloc(20);
    int *tab_after_realloc = foo_realloc(tab, 80); //do nothing
    for(int i=0; i < 20; i++)
        munit_assert_int(tab_after_realloc[i], ==, i);
    foo_free(tab_after_realloc);
    foo_free(placeholder);
    
    return MUNIT_OK;
}

static MunitResult realloc_test2(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    malloc_init();
    int *tab = foo_malloc(20 * sizeof(int));
    for(int i=0; i < 20; i++)
        tab[i] = i;
    int *placeholder = foo_malloc(20);
    int *tab_after_realloc = foo_realloc(tab, 80 * sizeof(int));
    for(int i=0; i < 20; i++)
        munit_assert_int(tab_after_realloc[i], ==, i);
    for(int i=20; i < 80; i++)
        tab_after_realloc[i] = 10;
    foo_free(tab_after_realloc);
    foo_free(placeholder);
    
    return MUNIT_OK;
}

static MunitResult realloc_test3(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    malloc_init();
    int *tab = foo_malloc(20 * sizeof(int));
    for(int i=0; i < 20; i++)
        tab[i] = i;
    int *tab_after_realloc = foo_realloc(tab, 80 * sizeof(int));
    for(int i=0; i < 20; i++)
        munit_assert_int(tab_after_realloc[i], ==, i);
    for(int i=20; i < 80; i++)
        tab_after_realloc[i] = 10;
    foo_free(tab_after_realloc);
    
    return MUNIT_OK;
}

static MunitResult malloc_and_free_100000x_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);
    malloc_init();
    int *addr[100000];
    for(int i=0; i < 100000; i++) {
        addr[i] = foo_malloc(24);
        addr[i][0] = 10;
        addr[i][5] = 20;
    }
    for(int i=0; i < 100000; i++) {
        munit_assert_int(addr[i][0], ==, 10);
        munit_assert_int(addr[i][5], ==, 20);
        foo_free(addr[i]);
    }

    return MUNIT_OK;
}

static MunitResult malloc_and_free_100000x_even_odd_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    malloc_init();
    int *addr[100000];
    for(int i=0; i < 100000; i++) {
        addr[i] = foo_malloc(24);
        addr[i][0] = 10;
        addr[i][5] = 20;
    }
    for(int i=0; i < 100000; i += 2) {
        munit_assert_int(addr[i][0], ==, 10);
        munit_assert_int(addr[i][5], ==, 20);
        foo_free(addr[i]);
    }  
    for(int i=1; i < 100000; i += 2) {
        munit_assert_int(addr[i][0], ==, 10);
        munit_assert_int(addr[i][5], ==, 20);
        foo_free(addr[i]);
    }  
    
    return MUNIT_OK;
}

static MunitResult realloc_extension_and_shrink_test(const MunitParameter params[], void *data) {
    UNUSED(params); UNUSED(data);  
    malloc_init();
    int *addr[100000];
    int size = 24;
    for(int i=0; i < 100000; i++) {
        addr[i] = foo_malloc(size);
        addr[i][5] = 10;
    }
    for(int i=0; i < 100000; i++) {
        size = 24;
        for(int j=0; j < 10; j++) {
            size += 8;
            addr[i] = foo_realloc(addr[i], size);
            for(int k=0; k < size / 4; k++)
                addr[i][k] = 10;
        }
    }
    for(int i=0; i < 100000; i++) 
        for(int j=0; j < size / 4; j++) 
            munit_assert_int(addr[i][j], ==, 10);
    int old_size = size;
    for(int i=0; i < 10000; i++) {
        size = old_size;
        for(int j=0; j < 10; j++) {
            size -= 8;
            addr[i] = foo_realloc(addr[i], size);
        }
    }
    for(int i=0; i < 100000; i++) {
        for(int j=0; j < size / 4; j++) 
            munit_assert_int(addr[i][j], ==, 10);
        foo_free(addr[i]);
    }

    return MUNIT_OK;
}



static MunitTest tests[] = {
    { (char*) "/allocate_chunk", allocate_chunk_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_has_enough_space/test1 (allignment to 8)", block_has_enough_space_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_has_enough_space/test2 (allignment to 16)", block_has_enough_space_test2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_has_enough_space/test2 (allignment to 32)", block_has_enough_space_test3, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_has_enough_space/test4 (allignment to 64)", block_has_enough_space_test4, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/trim_block/test1 (allignment to 32)", trim_block_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/trim_block/test2 (allignment to 16)", trim_block_test2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/trim_block/test3 (allignment to 64)", trim_block_test3, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_spliting/block_may_be_splited", block_may_be_splited_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_spliting/split_block/test1", split_block_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_spliting/split_block/test2", split_block_test2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/give_block_from_chunk/test1", give_block_from_chunk_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/give_block_from_chunk/test2", give_block_from_chunk_test2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_navigation/has_higher_block", has_higher_block_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_navigation/get_higher_block", get_higher_block_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_navigation/has_lower_block", has_lower_block_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/block_navigation/get_lower_block", get_lower_block_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/is_addr_in_chunk", is_addr_in_chunk_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/compare_block_sizes", compare_block_sizes_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/shrink_block/test1", shrink_block_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/shrink_block/test2", shrink_block_test2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/is_block_extension_possible", is_block_extension_possible_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/is_extension_with_split_possible", is_extension_with_split_possible_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/extend_block_without_split", extend_block_without_split_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/extend_block_with_split", extend_block_with_split_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    { (char*) "/malloc_int", malloc_int, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/posix_memalign", posix_memalign_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/free", free_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/calloc", calloc_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/realloc/test1", realloc_test1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/realloc/test2", realloc_test2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/realloc/test3", realloc_test3, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/malloc_and_free_100000x", malloc_and_free_100000x_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/malloc_and_100000x_even_odd", malloc_and_free_100000x_even_odd_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },    
    { (char*) "/realloc_extension_and_shrink", realloc_extension_and_shrink_test, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }, 
    NULL
};

static const MunitSuite test_suite = {
    (char*) "/malloc_tests", 
    tests, 
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE
};
    


int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    #ifdef USE_CUSTOM_MALLOC
        printf("Comment line 7 (#define USE_CUSTOM_MALLOC) in malloc.h and build project again\n");
        return -1;
    #endif

    return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}