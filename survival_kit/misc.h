
#ifndef SKIT_MISC_INCLUDED
#define SKIT_MISC_INCLUDED

/* Implementation details. */
/* TODO: these should be stored in thread-local storage. */
#define SKIT_ERROR_BUFFER_SIZE 1024
extern char skit_error_text_buffer[SKIT_ERROR_BUFFER_SIZE];


/** Unconditionally exit the program and print the formatted string 'mess'. */
void skit_die(char *mess, ...);

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
