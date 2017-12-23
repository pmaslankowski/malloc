#include <stdio.h>
#include "malloc.h"
#include "malloc_internals.h"

void allocate_one_page();
void malloc_int();

int main() {
    malloc_int();
    return 0;
}

void allocate_one_page() {
    printf("Test: allocate_one_page:\n");
    malloc_init();
    allocate_chunk(1);
    mdump(0);
}

void malloc_int() {
    printf("Test: malloc_int:\n");    
    int *number = (int*) foo_malloc(sizeof(int));
    *number = 5;
    printf("%d\n", *number);
    mdump(1);
}