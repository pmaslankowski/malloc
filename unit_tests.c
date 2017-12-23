#include <stdio.h>
#include "malloc.h"
#include "malloc_internals.h"

void allocate_one_page();


int main() {
    allocate_one_page();
    return 0;
}

void allocate_one_page() {
    printf("Test: allocate_one_page:\n");
    malloc_init();
    allocate_chunk(1);
    mdump();
}