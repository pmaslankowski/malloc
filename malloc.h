/*                      SYSTEMY OPERACYJNE 2017/2018                         *
 *                      Projekt 1 - Menadżer pamięci                         *
 *                       Autor: Piotr Maślankowski                           *
 *                                                                           *
 *     malloc.h - deklaracja funkcji z interfejsu oraz flagi ustawień        */

#ifndef __MALLOC_H__
#define __MALLOC_H__


// Settings flags:
#define MALLOC_DEBUG_SAFE 0
#define MALLOC_DEBUG 0
#define OVERRIDE_SIGSEGV_HANDLER 0
#define OVERRIDE_STD_MALLOC 1

#if OVERRIDE_STD_MALLOC
#define foo_malloc malloc 
#define foo_calloc calloc 
#define foo_realloc realloc 
#define foo_posix_memalign posix_memalign 
#define foo_free free 
#endif 

extern void *foo_malloc(size_t size);
extern void *foo_calloc(size_t count, size_t size);
extern void *foo_realloc(void *ptr, size_t size);
extern int foo_posix_memalign(void **memptr, size_t alignment, size_t size);
extern void foo_free(void *ptr);
void mdump(int verbose);

#endif