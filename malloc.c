#include <sys/queue.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

#include "malloc.h"
#include "malloc_internals.h"
#include "malloc_constants.h"

#define DEBUG_ULONG(x) printf("%s = %lu\n", #x, x) 
#define DEBUG_INT(x) printf("%s = %d\n", #x, x)

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
            if(block_has_enough_space(block, size, 8)) 
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
                    if(i % 8 == 0) printf("\n\t\t%lx: ",(uint64_t) block->mb_data + i);
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
    size_t actual_size = size + MEM_CHUNK_OVERHEAD + EOC_SIZE + BOUNDARY_TAG_SIZE;
    memory_map(actual_size, &alloc_context);

    mem_chunk_t *chunk = (mem_chunk_t*) alloc_context.addr;
    mem_block_t *block = &chunk->ma_first;
    chunk->eoc = EOC;
    chunk->size = alloc_context.chunk_size - MEM_CHUNK_OVERHEAD - EOC_SIZE;
    block->mb_size = alloc_context.chunk_size - MEM_CHUNK_OVERHEAD - EOC_SIZE; // size of mb_data
    block->mb_data[block->mb_size / 8] = EOC; 
    
    set_boundary_tag(block);

    LIST_INSERT_HEAD(&chunk->ma_freeblks, block, mb_node);
    LIST_INSERT_HEAD(&chunk_list, chunk, ma_node);
    return chunk;
}


void memory_map(size_t size, alloc_context_t* res) {
    int page_size = getpagesize();
    int num_pages = (size + page_size - 1) / page_size;
    res->addr = mmap(NULL, num_pages*page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    res->chunk_size = page_size * num_pages;
}


/* boundary tags: mb_size of block or EOC tag */ 
void set_boundary_tag(mem_block_t *block) {
    block->mb_data[block->mb_size / 8 - 1] = block->mb_size;
}

uint64_t allign(uint64_t value, uint64_t allignment) {
    return (value + allignment - 1) / allignment * allignment;    
}

/* Function checks if block has enough space to share size memory. If start of block's payload is not alligned, then function checks if 
   block has enough space to divide into two blocks with proper allignment */
int block_has_enough_space(mem_block_t *block, size_t size, unsigned allignment) {
    uint64_t addr = (uint64_t)block->mb_data;
    uint64_t alligned_addr = allign(addr, allignment);
    if(alligned_addr == addr) 
        return size <= (uint64_t)block->mb_size - BOUNDARY_TAG_SIZE;
    else
        return alligned_addr - addr >= sizeof(mem_block_t) && alligned_addr - addr + size <= (uint64_t)block->mb_size - BOUNDARY_TAG_SIZE;
}

int is_trimming_needed(mem_block_t *block, unsigned allignment) {
    return (uint64_t)block->mb_data % allignment != 0;
}

/* Function trims block and returns new, properly alligned block. Function assumes that trimming is possible and necessary. */
mem_block_t *block_trim(mem_block_t *block, unsigned allignment) {
    uint64_t addr = (uint64_t)block->mb_data;
    uint64_t alligned_addr = allign(addr, allignment);
    mem_block_t *alligned_block = (mem_block_t*)(alligned_addr - 8);
    alligned_block->mb_size = block->mb_size - (alligned_addr - 8 - addr);
    block->mb_size = alligned_addr - 8 - addr;

    set_boundary_tag(block); // update because size has changed       
    set_boundary_tag(alligned_block);
    
    LIST_INSERT_AFTER(block, alligned_block, mb_node);
    return alligned_block;
}

int block_may_be_splited(mem_block_t *block, size_t size) {
    return (uint64_t) block->mb_size >= allign(size, 8u) + BOUNDARY_TAG_SIZE +  sizeof(mem_block_t); // true if there is enough space to split block
}


size_t allign_to_8(size_t size) {
    if (size % 8 == 0) return size;
    return size + 8 - size % 8;
}

mem_block_t *split_block(mem_block_t *block, size_t size) { //TODO: add allignment constraints to this and similar functions to make implementation of memalign possible
    // splits current block and returns a block consisting space which left after split
    size = allign_to_8(size);
    mem_block_t* block_left = (mem_block_t*) block->mb_data + size + 8; // we add 8 because of boundary tag
    block_left->mb_size = block->mb_size - size - 8;
    block->mb_size = size + 8;
    set_boundary_tag(block);
    return block_left;
}


void chunk_add_free_block(mem_chunk_t *chunk, mem_block_t *block) {
    mem_block_t *block_iter;
    LIST_FOREACH(block_iter, &chunk->ma_freeblks, mb_node) {
        if(block >= block_iter) {
            LIST_INSERT_BEFORE(block_iter, block, mb_node);
            return;
        }
    }
    LIST_INSERT_AFTER(block_iter, block, mb_node);
}


void *give_block_from_chunk(mem_chunk_t *chunk, mem_block_t *block, size_t size) {
    if(block_may_be_splited(block, size)) {
        mem_block_t* block_left = split_block(block, size);
       // set_lower_boundary_tag(block_left, 0);
       // set_upper_boundary_tag(block, 1);

        // set boundary tag for upper neighbor of current block
        if(block >= &chunk->ma_first)
            *((uint64_t*)block-1) |= 0 << 31;
        
        LIST_INSERT_AFTER(block, block_left, mb_node);
        LIST_REMOVE(block, mb_node);

        return block->mb_data + 1; 
    } else {
        // set boundary tags for neighbor blocks
        if(block >= &chunk->ma_first)
            *((uint64_t*)block-1) &= 0 << 31;
        if((uint8_t*) block->mb_data + block->mb_size < (uint8_t*)chunk->ma_first.mb_data + chunk->size)
            *(uint64_t*)((uint8_t*)block->mb_data + block->mb_size + sizeof(mem_block_t)) &= 0 << 31;

        LIST_REMOVE(block, mb_node);

        return block->mb_data + 1;    
    }
}



