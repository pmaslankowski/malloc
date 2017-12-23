/* This header exposes internal functions of malloc for unit test purposes */
#ifndef __MALLOC_INTERNALS_H__
#define __MALLOC_INTERNALS_H__

#include <sys/queue.h>
#include <stdint.h>


typedef struct mem_block {
    int32_t mb_size; /* mb_size > 0 => free, mb_size < 0 => allocated */
    union {
        LIST_ENTRY(mem_block) mb_node; /* node on free block list, valid if block is free */
        uint64_t mb_data[0]; /* user data pointer, valid if block is allocated */
    };
} mem_block_t;

typedef struct mem_chunk {
    LIST_ENTRY(mem_chunk) ma_node; /* node on list of all chunks */
    LIST_HEAD(, mem_block) ma_freeblks; /* list of all free blocks in the chunk */
    int32_t size; /* chunk size minus sizeof(mem_chunk_t) */
    mem_block_t ma_first; /* first block in the chunk */
} mem_chunk_t;

/* struct returned from alloc pages */
typedef struct alloc_context {
    void *addr;
    int32_t chunk_size;
} alloc_context_t;


void malloc_init();
mem_chunk_t *allocate_chunk(size_t size);
void allocate_pages(size_t size, alloc_context_t* res);
void set_upper_boundary_tag(mem_block_t *block, unsigned value);
void set_lower_boundary_tag(mem_block_t *block, unsigned value);
int block_has_enough_space(mem_block_t *block, size_t size);
void *give_block_from_chunk(mem_chunk_t *chunk, mem_block_t *block, size_t size);

#endif