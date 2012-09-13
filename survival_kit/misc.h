
#ifndef SKIT_MISC_INCLUDED
#define SKIT_MISC_INCLUDED

/** Unconditionally exit the program and print the formatted string 'mess'. */
void skit_die(char *mess, ...);

/** 
Prints a chunk of memory in both hexadecimal and text form. 
TODO: there is a current limitation that 'size' must be a multiple of 8.
*/
void skit_print_mem(void *ptr, int size);

#endif
