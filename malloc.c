#include <sys/queue.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#include "malloc.h"
#include "malloc_internals.h"
#include "malloc_constants.h"

static LIST_HEAD(, mem_chunk) chunk_list; /* list of all chunks */
static int malloc_initialised = 0;

static pthread_mutex_t malloc_mutex = PTHREAD_MUTEX_INITIALIZER;


void sigsegv_handler(int code, siginfo_t *s, void *v_ctx) {
    UNUSED(s); UNUSED(v_ctx); UNUSED(code);
    mdump(0);
	exit(0);	
}

struct sigaction sigsegv_sigaction;

void bind_handler() {
    printf("halo");
	sigsegv_sigaction.sa_sigaction = sigsegv_handler;
	sigsegv_sigaction.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sigsegv_sigaction, NULL);
}


/* Implementation of interface: */

void *foo_malloc(size_t size) {
    if(MALLOC_DEBUG)
        fprintf(stderr, "entering malloc(size = %lu)\n", size);
        
    pthread_mutex_lock(&malloc_mutex);
    void *result = do_malloc(size);
    pthread_mutex_unlock(&malloc_mutex);

    if(MALLOC_DEBUG)
        fprintf(stderr, "exiting malloc (returned value = %p)\n", result);
    
    return result;
}

void *foo_calloc(size_t count, size_t size) {
    if(MALLOC_DEBUG)
        fprintf(stderr, "entering calloc(count = %lu, size = %lu)\n", count, size);

    pthread_mutex_lock(&malloc_mutex);
    void *result = do_calloc(count, size);
    pthread_mutex_unlock(&malloc_mutex);

    if(MALLOC_DEBUG)
        fprintf(stderr, "exiting calloc (returned value = %p)\n", result);

    return result; 
}

void *foo_realloc(void* ptr, size_t size) {
    if(MALLOC_DEBUG)
        fprintf(stderr, "entering realloc (ptr = %p, size = %lu)\n", ptr, size);
    
    pthread_mutex_lock(&malloc_mutex);
    void *result = do_realloc(ptr, size);
    pthread_mutex_unlock(&malloc_mutex);

    if(MALLOC_DEBUG)
        fprintf(stderr, "exiting realloc (returned value = %p)\n", result);
    
    return result;
}

int foo_posix_memalign(void **memptr, size_t alignment, size_t size) {
    if(MALLOC_DEBUG)
        fprintf(stderr, "entering posix_memalign (memptr = %p, alignment = %lu, size = %lu)\n", memptr, alignment, size);
    
    pthread_mutex_lock(&malloc_mutex);
    int result = do_posix_memalign(memptr, alignment, size);
    pthread_mutex_unlock(&malloc_mutex);

    if(MALLOC_DEBUG)
        fprintf(stderr, "exiting posix_memalign (returned value = %d, *memptr = %p)\n", result, *memptr);
    
    return result;
}

void foo_free(void *addr) {
    if(MALLOC_DEBUG)
        fprintf(stderr, "entering free (addr = %p)\n", addr);
    
    pthread_mutex_lock(&malloc_mutex);
    do_free(addr);
    pthread_mutex_unlock(&malloc_mutex);

    if(MALLOC_DEBUG)
        fprintf(stderr, "exiting free\n");
}


/* ==============================================================================================================================*/
/* Auxilary functions: */

void *do_malloc(size_t size) {
    if(!malloc_initialised)
        malloc_init();
    
    if(size >= LARGE_THRESHOLD) {
        mem_chunk_t *chunk = allocate_chunk(allign(size, 8));
        return give_block_from_chunk(&chunk->ma_first, allign(size, 8), DEFAULT_ALIGNMENT);
    }

    if(size < 16)
        size = 16;
    mem_chunk_t *chunk;
    mem_block_t *block;
    LIST_FOREACH(chunk, &chunk_list, ma_node) {
        LIST_FOREACH(block, &chunk->ma_freeblks, mb_node) {
            if(block_has_enough_space(block, size, DEFAULT_ALIGNMENT)) 
                return give_block_from_chunk(block, size, DEFAULT_ALIGNMENT);
        }
    }

    chunk = allocate_chunk(NEW_CHUNK_SIZE);
    return give_block_from_chunk(&chunk->ma_first, size, DEFAULT_ALIGNMENT);
}


void *do_calloc(size_t count, size_t size) {
    if(count == 0 || size == 0)
        return NULL;
    void *ptr = do_malloc(count * size);
    memset(ptr, 0, count * size);
    return ptr;
}


void *do_realloc(void *ptr, size_t size) {  
    if(!malloc_initialised)
        malloc_init();

    if(ptr == NULL)
        return do_malloc(size);
    
    mem_block_t* block = (mem_block_t*) (ptr - MEM_BLOCK_OVERHEAD);
    assert(block->mb_size < 0);
    if(MALLOC_DEBUG) fprintf(stderr, "block->mb_size = %d, size = %lu\n", block->mb_size, size);
    int comparison = compare_block_sizes(block, size);
    if(comparison == 0) {
        if(MALLOC_DEBUG) fprintf(stderr, "no need to realocate\n");
        return ptr; //no need to reallocate
    }
    if(comparison < 0) {
        if(comparison >= -8) // because of problems with mb_node in new block
            size += 8;

        if(is_block_extension_possible(block, size)) {
            if(is_extension_with_split_possible(block, size)) {
                if(MALLOC_DEBUG) fprintf(stderr, "Extension with split\n");
                extend_block_with_split(block, size);
            } else {
                if(MALLOC_DEBUG) fprintf(stderr, "Extension without split\n");
                extend_block_without_split(block, size);
            }
            return ptr;
        } else {
            if(MALLOC_DEBUG) fprintf(stderr, "extension is impossible. Reallocating in another place\n");
            void *new_ptr = do_malloc(size);
            memcpy(new_ptr, ptr, -block->mb_size - BOUNDARY_TAG_SIZE);
            do_free(ptr);
            return new_ptr;
        }
    } else {
        if(allign(size, 8u) < 16)
            size = 16;
        if(is_shrink_possible(block, size)) {
            if(MALLOC_DEBUG) fprintf(stderr, "Shrinking is possible\n");
            shrink_block(block, size);
        }
        return ptr;
    }
}


int do_posix_memalign(void **memptr, size_t alignment, size_t size) {
    if(!malloc_initialised)
        malloc_init();
    if (alignment % sizeof(void*) != 0 || (alignment & (alignment-1)) != 0)
        return EINVAL;

    size_t size_and_alignment = size + 2 * alignment; // 2*alignment is enough when alignment = 16, so it should be enough whene alignment > 16
    if(size>= LARGE_THRESHOLD) {
        mem_chunk_t *chunk = allocate_chunk(size_and_alignment);
        *memptr = give_block_from_chunk(&chunk->ma_first, size, alignment);
        return 0;
    }

    if(size < 16)
        size = 16;
    mem_chunk_t *chunk;
    mem_block_t *block;
    LIST_FOREACH(chunk, &chunk_list, ma_node) {
        LIST_FOREACH(block, &chunk->ma_freeblks, mb_node) {
            if(block_has_enough_space(block, size, alignment)) {
                *memptr = give_block_from_chunk(block, size, alignment);
                return 0;
            }
        }
    }

    chunk = allocate_chunk(NEW_CHUNK_SIZE);
    *memptr = give_block_from_chunk(&chunk->ma_first, size, alignment); 
    return 0;
}


void do_free(void *ptr) {    
    if(ptr == NULL) 
        return;

    mem_block_t *block = (mem_block_t*) (ptr - MEM_BLOCK_OVERHEAD);
    assert(block->mb_size < 0);
    block->mb_size *= -1;

    if(is_merge_with_lower_block_possible(block)) {
        mem_block_t *lower_block = get_lower_block(block);
        lower_block->mb_size += block->mb_size + MEM_BLOCK_OVERHEAD;
        set_boundary_tag(lower_block);
        if(is_merge_with_higher_block_possible(lower_block)) {
            mem_block_t *higher_block = get_higher_block(block);
            lower_block->mb_size += higher_block->mb_size + MEM_BLOCK_OVERHEAD;
            set_boundary_tag(lower_block);
            LIST_REMOVE(higher_block, mb_node);
        }
        if(is_unmap_needed(lower_block))
            free_chunk_by_block(lower_block);  
        return;
    }

    if(is_merge_with_higher_block_possible(block)) {
        mem_block_t *higher_block = get_higher_block(block);
        block->mb_size += higher_block->mb_size + MEM_BLOCK_OVERHEAD;
        set_boundary_tag(block);
        LIST_INSERT_BEFORE(higher_block, block, mb_node);
        LIST_REMOVE(higher_block, mb_node);
        if(is_unmap_needed(block))
            free_chunk_by_block(block);
        return;
    }

    // merge is not possible, so find proper place in list and insert there processed block
    mem_chunk_t *chunk = get_chunk_of(ptr);
    chunk_add_free_block(chunk, block);
    if(is_unmap_needed(block))
        free_chunk(chunk);
}


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


void malloc_init() {
    malloc_initialised = 1;
    LIST_INIT(&chunk_list);
    printf("halo1\n");
    bind_handler();
}


/* Function creates new chunk and adds it to list of available chunks.
   Assumption: size >= 16 (because we need to have place for boundary tags) */
mem_chunk_t *allocate_chunk(size_t size) {
    assert(size >= 16);
    assert(size % 8 == 0);

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
    assert(allign(size, 8u) >= 16); // because of place for boundary tag when block is free
    uint64_t addr = (uint64_t) block->mb_data;
    uint64_t alligned_addr = allign(addr, allignment);
    uint64_t alligned_size = allign(size, 8u);
    if(alligned_addr != addr) {
        while(alligned_addr - addr < sizeof(mem_block_t) + BOUNDARY_TAG_SIZE)
            alligned_addr += allignment;
    }
    if(alligned_addr + alligned_size + BOUNDARY_TAG_SIZE > addr + block->mb_size)
        return 0;
    
    return 1; 
}

int is_trimming_needed(mem_block_t *block, unsigned allignment) {
    return (uint64_t)block->mb_data % allignment != 0;
}

uint64_t get_alligned_addr(mem_block_t *block, size_t size, unsigned allignment) {
    uint64_t addr = (uint64_t) block->mb_data;
    uint64_t alligned_addr = allign(addr, allignment);
    uint64_t alligned_size = allign(size, 8u);
    assert(alligned_size >= 16);
    if(alligned_addr != addr) {
        while(alligned_addr - addr < sizeof(mem_block_t) + BOUNDARY_TAG_SIZE)
            alligned_addr += allignment;
    }
    assert(alligned_addr + alligned_size + BOUNDARY_TAG_SIZE <= addr + block->mb_size);
    return alligned_addr;
}

/* Function trims block and returns new, properly alligned block. Function assumes that trimming is possible and necessary.
   Caution: size here is only for assertion. Trimmed block is not further splited to the best size. */
mem_block_t *block_trim(mem_block_t *block, size_t size, unsigned allignment) {
    uint64_t addr = (uint64_t)block->mb_data;
    uint64_t alligned_addr = get_alligned_addr(block, size, allignment);
    mem_block_t *alligned_block = (mem_block_t*)(alligned_addr - MEM_BLOCK_OVERHEAD);
    alligned_block->mb_size = block->mb_size - (alligned_addr - addr);
    block->mb_size = alligned_addr - addr - MEM_BLOCK_OVERHEAD;

    set_boundary_tag(block); // update because size has changed       
    set_boundary_tag(alligned_block);
    
    LIST_INSERT_AFTER(block, alligned_block, mb_node);
    return alligned_block;
}

int block_may_be_splited(mem_block_t *block, size_t size) {
    assert(allign(size, 8u) >= 16);
    return (uint64_t) block->mb_size >= allign(size, 8u) + BOUNDARY_TAG_SIZE +  sizeof(mem_block_t) + BOUNDARY_TAG_SIZE; // true if there is enough space to split block
}

/* Function splits current block and returns a block consisting space which left after split.
   Function assumes that making split is possible. */
mem_block_t *split_block(mem_block_t *block, size_t size) { 
    size_t alligned_size = allign(size, 8u);
    assert(alligned_size >= 16);
    mem_block_t* block_left = (mem_block_t*) ((uint8_t*)block->mb_data + alligned_size + BOUNDARY_TAG_SIZE);
    block_left->mb_size = block->mb_size - alligned_size - BOUNDARY_TAG_SIZE - MEM_BLOCK_OVERHEAD;
    block->mb_size = alligned_size + BOUNDARY_TAG_SIZE;
    
    set_boundary_tag(block);
    set_boundary_tag(block_left);

    LIST_INSERT_AFTER(block, block_left, mb_node);
    return block_left;
}


void chunk_add_free_block(mem_chunk_t *chunk, mem_block_t *block) {
    if(LIST_EMPTY(&chunk->ma_freeblks)) {
        LIST_INSERT_HEAD(&chunk->ma_freeblks, block, mb_node);
        return;
    }
    mem_block_t *block_iter;
    LIST_FOREACH(block_iter, &chunk->ma_freeblks, mb_node) {
        if(block_iter >= block) {
            LIST_INSERT_BEFORE(block_iter, block, mb_node);
            return;
        }
        if(LIST_NEXT(block_iter, mb_node) == NULL) {
            LIST_INSERT_AFTER(block_iter, block, mb_node);
            return;
        }
    }
}

void set_block_allocated(mem_block_t *block) {
    LIST_REMOVE(block, mb_node);
    block->mb_size *= -1;
}

void *give_block_from_chunk(mem_block_t *block, size_t size, unsigned allignment) {
    assert(allign(size, 8u) >= 16);
    
    if(is_trimming_needed(block, allignment))
        block = block_trim(block, size, allignment);
    assert((uint64_t)block->mb_data % allignment == 0);

    if(block_may_be_splited(block, size)) 
        split_block(block, size);
    
    set_block_allocated(block);
    return block->mb_data; 
}

int has_higher_block(mem_block_t *block) {
    if(block->mb_size > 0)
        return (int64_t)block->mb_data[block->mb_size / 8] != EOC;
    else
        return (int64_t)block->mb_data[-block->mb_size / 8] != EOC;
}

int has_lower_block(mem_block_t *block) {
    return *((int64_t*)block - 1) != EOC;
}

mem_block_t *get_higher_block(mem_block_t *block) {
    if(block->mb_size > 0)
        return (mem_block_t*) &block->mb_data[block->mb_size / 8];
    else
        return (mem_block_t*) &block->mb_data[-block->mb_size / 8];
}

mem_block_t *get_lower_block(mem_block_t *block) {
    int64_t boundary_tag = *((int64_t*)block - 1);
    return (mem_block_t*)((int64_t*)block-1-(boundary_tag/8-1)-1);
}

int is_merge_with_lower_block_possible(mem_block_t *block) {
    return has_lower_block(block) && get_lower_block(block)->mb_size > 0;
}

int is_merge_with_higher_block_possible(mem_block_t *block) {
    return has_higher_block(block) && get_higher_block(block)->mb_size > 0;
}

//this function could possibly be better - it could detect invalid pointers to memory at the end of the chunk
int is_addr_in_chunk(mem_chunk_t *chunk, void *addr) {
    return addr >= (void*)chunk->ma_first.mb_data && addr < (void*)chunk->ma_first.mb_data + chunk->size;
}

mem_chunk_t *get_chunk_of(void *addr) {
    mem_chunk_t *chunk;
    LIST_FOREACH(chunk, &chunk_list, ma_node) {
        if(is_addr_in_chunk(chunk, addr))
            return chunk;
    }
    assert(0); // bad pointer
    return NULL;
}

// untested functions: (but they probably work)
int is_unmap_needed(mem_block_t *block) {
    return !has_higher_block(block) && !has_lower_block(block);
}

void free_chunk(mem_chunk_t *chunk) {
    LIST_REMOVE(chunk, ma_node);
    munmap(chunk, chunk->size + MEM_CHUNK_OVERHEAD + EOC_SIZE);
}

// function analogical to free_chunk, but takes block, not chunk as argument
// assumptions: block is the only block in chunk
void free_chunk_by_block(mem_block_t *block) {
    assert(!has_higher_block(block));
    assert(!has_lower_block(block));
    mem_chunk_t *chunk = (void*) block + MEM_BLOCK_OVERHEAD - MEM_CHUNK_OVERHEAD;
    free_chunk(chunk); 
}

// returns block - size
// assumption: block cannot be free
int compare_block_sizes(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0); // this function should be used only on allocated blocks
    int64_t block_size = -block->mb_size;
    return block_size - BOUNDARY_TAG_SIZE - size;
}

int is_shrink_possible(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0);
    assert(allign(size, 8u) >= 16);
    block->mb_size *= -1;
    int result = block_may_be_splited(block, size);
    block->mb_size *= -1;
    return result;
}

void shrink_block(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0);
    int64_t aligned_size = allign(size, 8);
    assert(aligned_size >= 16);
    
    mem_block_t* block_left = (mem_block_t*) ((uint8_t*)block->mb_data + aligned_size + BOUNDARY_TAG_SIZE);
    block_left->mb_size = -block->mb_size - aligned_size - BOUNDARY_TAG_SIZE - MEM_BLOCK_OVERHEAD;
    set_boundary_tag(block_left);

    block->mb_size = aligned_size + BOUNDARY_TAG_SIZE;
    set_boundary_tag(block);
    block->mb_size *= -1;

    if(is_merge_with_higher_block_possible(block_left)) {
        mem_block_t *higher_block = get_higher_block(block_left);
        block_left->mb_size += higher_block->mb_size + MEM_BLOCK_OVERHEAD;
        set_boundary_tag(block_left);
        LIST_INSERT_BEFORE(higher_block, block_left, mb_node);
        LIST_REMOVE(higher_block, mb_node);
    } else {
        mem_chunk_t *chunk = get_chunk_of((void*)block_left + MEM_BLOCK_OVERHEAD);
        chunk_add_free_block(chunk, block_left);
    }
}

int is_block_extension_possible(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0);
    int64_t aligned_size = allign(size, 8);
    assert(aligned_size >= 16);
    if(!has_higher_block(block))
        return 0;
    int64_t block_size = -block->mb_size;
    mem_block_t *higher_block = get_higher_block(block);
    return higher_block->mb_size >= (int64_t)(aligned_size + BOUNDARY_TAG_SIZE - block_size - MEM_BLOCK_OVERHEAD);
}

int is_extension_with_split_possible(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0);
    int64_t aligned_size = allign(size, 8);
    assert(aligned_size >= 16);
    if(!has_higher_block(block))
        return 0;
    int64_t block_size = -block->mb_size;
    mem_block_t *higher_block = get_higher_block(block);
    return higher_block->mb_size >= (int64_t)(size + BOUNDARY_TAG_SIZE - block_size - MEM_BLOCK_OVERHEAD + sizeof(mem_block_t) + BOUNDARY_TAG_SIZE);  
}

void extend_block_without_split(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0);
    int64_t aligned_size = allign(size, 8);
    assert(aligned_size >= 16);

    mem_block_t *higher_block = get_higher_block(block);
    block->mb_size += -(higher_block->mb_size + MEM_BLOCK_OVERHEAD);
    block->mb_size *= -1;
    set_boundary_tag(block);
    block->mb_size *= -1;

    LIST_REMOVE(higher_block, mb_node);
}

void extend_block_with_split(mem_block_t *block, int64_t size) {
    assert(block->mb_size < 0);
    int64_t aligned_size = allign(size, 8);
    assert(aligned_size >= 16);

    int64_t block_size = -block->mb_size;
    mem_block_t *higher_block = get_higher_block(block);
    mem_block_t *block_left = (mem_block_t*)((void*)block->mb_data + aligned_size + BOUNDARY_TAG_SIZE);

    LIST_INSERT_AFTER(higher_block, block_left, mb_node);
    LIST_REMOVE(higher_block, mb_node);

    block_left->mb_size = block_size + higher_block->mb_size - aligned_size - BOUNDARY_TAG_SIZE;
    set_boundary_tag(block_left);

    block->mb_size = aligned_size + BOUNDARY_TAG_SIZE;
    set_boundary_tag(block);
    block->mb_size *= -1;
}