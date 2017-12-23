#include <sys/queue.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

#include "malloc.h"
#include "malloc_internals.h"
#include "malloc_constants.h"


static LIST_HEAD(, mem_chunk) chunk_list; /* list of all chunks */
static int malloc_initialised = 0;

/* Implementation of interface: */

void *foo_malloc(size_t size) {
    if(!malloc_initialised)
        malloc_init();

    if(size >= LARGE_THRESHOLD) {
        mem_chunk_t *chunk = allocate_chunk(size);
        return give_block_from_chunk(chunk, &chunk->ma_first, size);
    }
    mem_chunk_t *chunk;
    mem_block_t *block;
    LIST_FOREACH(chunk, &chunk_list, ma_node) {
        LIST_FOREACH(block, &chunk->ma_freeblks, mb_node) {
            if(block_has_enough_space(block, size)) 
                return give_block_from_chunk(chunk, block, size);
        }
    }

    chunk = allocate_chunk(NEW_CHUNK_SIZE);
    return give_block_from_chunk(chunk, &chunk->ma_first, size);
}

/*
void *calloc(size_t count, size_t size) {

}


void *realloc(void *ptr, size_t size) {

}


int posix_memalign(void **memptr, size_t alignment, size_t size) {

}


void free(void *ptr) {

}*/


void mdump(int verbose) {
    mem_chunk_t *chunk;
    mem_block_t *block;
    int chunk_index = 0;
    LIST_FOREACH(chunk, &chunk_list, ma_node) {
        printf("Chunk: %d\nSize: %d\nBlocks:\n", chunk_index++, chunk->size);
        int block_index = 0;
        LIST_FOREACH(block, &chunk->ma_freeblks, mb_node) {
            printf("\tBlock: %d\t Size: %d", block_index++, block->mb_size);
            if(verbose) {
                printf("\tData:");
                for(int i=0; i < block->mb_size; i++) {
                    if(i % 4 == 0) printf("\n");
                    printf("%04x ", *((uint8_t*) block->mb_data + i));
                }
            }
        }
    }

}




/* ==============================================================================================================================*/
/* Auxilary functions: */
void malloc_init() {
    malloc_initialised = 1;
    LIST_INIT(&chunk_list);
}


/* Function creates new chunk and adds it to list of available chunks */
mem_chunk_t *allocate_chunk(size_t size) {
    alloc_context_t alloc_context;
    allocate_pages(size, &alloc_context);

    mem_chunk_t *chunk = (mem_chunk_t*) alloc_context.addr;
    mem_block_t *block = &chunk->ma_first;
    chunk->size = alloc_context.chunk_size;
    block->mb_size = alloc_context.chunk_size;
    
    set_lower_boundary_tag(block, 0);
    set_upper_boundary_tag(block, 0);

    LIST_INSERT_HEAD(&chunk->ma_freeblks, block, mb_node);

    LIST_INSERT_HEAD(&chunk_list, chunk, ma_node);
    return chunk;
}


void allocate_pages(size_t size, alloc_context_t* res) {
    int page_size = getpagesize();
    int num_pages = size / page_size + 1;
    res->addr = mmap(NULL, num_pages*page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    res->chunk_size = page_size * num_pages - sizeof(mem_chunk_t);
}


/* boundary tags: 1 -> neighbor block is free */ 
void set_lower_boundary_tag(mem_block_t *block, unsigned value) {
    block->mb_data[0] |= value << 31;
}

void set_upper_boundary_tag(mem_block_t *block, unsigned value) {
    ((uint8_t*)block->mb_data)[block->mb_size-1] |= value << 31; // cast is needed because mb_size is in bytes
}

int block_has_enough_space(mem_block_t *block, size_t size) {
    return (uint32_t) block->mb_size >= size + 16u; // because we need two additional machine words for boundary tags
}

int block_may_be_splited(mem_block_t *block, size_t size) {
    return (uint32_t) block->mb_size >= size + 16u + sizeof(mem_block_t); // true if there is enough space to split block
}

mem_block_t *split_block(mem_block_t *block, size_t size) { 
    // splits current block and returns a block consisting space which left after split
    mem_block_t* block_left = (mem_block_t*) block->mb_data + size;
    block_left->mb_size = block->mb_size - size;
    block->mb_size = size;
    set_lower_boundary_tag(block_left, 1);
    set_upper_boundary_tag(block, 1);
    return block_left;
}


void chunk_add_free_block() {

}


void *give_block_from_chunk(mem_chunk_t *chunk, mem_block_t *block, size_t size) {
    if(block_may_be_splited(block, size)) {
        mem_block_t* block_left = split_block(block, size);
        set_lower_boundary_tag(block_left, 0);
        set_upper_boundary_tag(block, 1);

        // set boundary tag for upper neighbor of current block
        if(block >= &chunk->ma_first)
            *((uint64_t*)block-1) |= 0 << 31;
        
        chunk_add_free_block(chunk, block_left);
        LIST_REMOVE(block, mb_node);

        return block->mb_data + 1; 
    } else {
        // set boundary tags for neighbor blocks
        if(block >= &chunk->ma_first)
            *((uint64_t*)block-1) |= 0 << 31;
        if((uint8_t*) block->mb_data + block->mb_size < (uint8_t*)chunk->ma_first.mb_data + chunk->size)
            *(uint64_t*)((uint8_t*)block->mb_data + block->mb_size + sizeof(mem_block_t)) |= 0 << 31;

        LIST_REMOVE(block, mb_node);

        return block->mb_data + 1;    
    }
}



