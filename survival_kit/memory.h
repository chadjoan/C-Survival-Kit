
#ifndef SKIT_MEMORY_INCLUDED
#define SKIT_MEMORY_INCLUDED

#include <stdlib.h>

/** 
Calls C's malloc function.
In the future this will check for the existence of a custom allocator and use
that instead, if it's present.
*/
void *skit_malloc(size_t size);

/** 
Calls C's free function.
In the future this will check for the existence of a custom allocator and use
that instead, if it's present.
*/
void skit_free(void *mem);

/** 
Prints a chunk of memory in both hexadecimal and text form. 
TODO: there is a current limitation that 'size' must be a multiple of 8.
*/
void skit_print_mem(void *ptr, int size);

#endif
