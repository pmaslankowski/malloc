/*                                  SYSTEMY OPERACYJNE 2017/2018                                *
 *                                  Projekt 1 - Menadżer pamięci                                *
 *                                   Autor: Piotr Maślankowski                                  *
 *                                                                                              *
 * malloc_internals.h - definicje struktur oraz deklaracje prywatnych funkcji menadżera pamięci */


/* This header exposes internal functions of malloc for unit test purposes */
#ifndef __MALLOC_INTERNALS_H__
#define __MALLOC_INTERNALS_H__

#include <sys/queue.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>

#define BOUNDARY_TAG_SIZE 8
#define EOC_SIZE 8
#define EOC ((int64_t)-1)
#define DEFAULT_ALIGNMENT 8
#define MEM_BLOCK_OVERHEAD (sizeof(mem_block_t) - sizeof(LIST_ENTRY(mem_block_t))) // size of memory meta data in mem_block_t
#define MEM_CHUNK_OVERHEAD (sizeof(mem_chunk_t) - sizeof(LIST_ENTRY(mem_block_t))) // size of memory meta data in mem_chunk_t

#define UNUSED(x) (void)(x)
#define DEBUG_ULONG(x) printf("%s = %lu\n", #x, x) 
#define DEBUG_INT(x) printf("%s = %d\n", #x, x)
#define DEBUG_LONG(x) printf("%s = %ld\n", #x, x)

#define SAFE_PRINT(str) write(STDERR_FILENO, str, strlen(str))


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
    int64_t eoc; /* lower end of chunk mark */
    mem_block_t ma_first; /* first block in the chunk */
} mem_chunk_t;

/* struct returned from alloc pages */
typedef struct alloc_context {
    void *addr;
    int32_t chunk_size;
} alloc_context_t;

void *do_malloc(size_t size);
void *do_calloc(size_t count, size_t size);
void *do_realloc(void *ptr, size_t size);
int do_posix_memalign(void **memptr, size_t alignment, size_t size);
void do_free(void *ptr);
void malloc_init();
mem_chunk_t *allocate_chunk(size_t size);
void memory_map(size_t size, alloc_context_t* res);
void set_boundary_tag(mem_block_t *block);
int block_has_enough_space(mem_block_t *block, size_t size, unsigned allignment);
int is_trimming_needed(mem_block_t *block, unsigned allignment);
uint64_t allign(uint64_t value, uint64_t allignment);
uint64_t get_alligned_addr(mem_block_t *block, size_t size, unsigned allignment);
mem_block_t *block_trim(mem_block_t *block, size_t size, unsigned allignment);
int block_may_be_splited(mem_block_t *block, size_t size);
mem_block_t *split_block(mem_block_t *block, size_t size);
void set_block_allocated(mem_block_t *block);
void *give_block_from_chunk(mem_block_t *block, size_t size, unsigned allignment);
void chunk_add_free_block(mem_chunk_t *chunk, mem_block_t *block);
int has_lower_block(mem_block_t *block);
int has_higher_block(mem_block_t *block);
mem_block_t *get_lower_block(mem_block_t *block);
mem_block_t *get_higher_block(mem_block_t *block);
int is_merge_with_lower_block_possible(mem_block_t *block);
int is_merge_with_higher_block_possible(mem_block_t *block);
int is_addr_in_chunk(mem_chunk_t *chunk, void *addr);
mem_chunk_t *get_chunk_of(void *addr);
int is_unmap_needed(mem_block_t *block);
void free_chunk(mem_chunk_t *chunk);
void free_chunk_by_block(mem_block_t *block);
int compare_block_sizes(mem_block_t *block, int64_t size);
int is_shrink_possible(mem_block_t *block, int64_t size);
void shrink_block(mem_block_t *block, int64_t size);
int is_block_extension_possible(mem_block_t *block, int64_t size);
int is_extension_with_split_possible(mem_block_t *block, int64_t size);
void extend_block_without_split(mem_block_t *block, int64_t size);
void extend_block_with_split(mem_block_t *block, int64_t size);
void *return_from_malloc(void *addr);
void sigsegv_handler(int code, siginfo_t *s, void *v_ctx);
void bind_sigsegv_handler();

#endif