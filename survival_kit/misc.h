
#ifndef SKIT_MISC_INCLUDED
#define SKIT_MISC_INCLUDED

#include <stdlib.h>

/** Unconditionally exit the program and print the formatted string 'mess'. */
void skit_die(char *mess, ...);

/** 
Prints a chunk of memory in both hexadecimal and text form. 
TODO: there is a current limitation that 'size' must be a multiple of 8.
*/
void skit_print_mem(void *ptr, int size);

/**
Returns an error message that corresponds to the posix errno value.
The string returned may point to some form of static memory or internal
buffer and should not be modified or freed by the caller.
"buf" may or may not be used.
This is very similar to the GNU-specific version of strerror_r, but is
provided as a way to get a consistent interface accross operating systems.
This should be as safe to use in signals as is possible.
This function will try to avoid allocations.  (TODO: does strerror on VMS guarantee this?)
*/
const char *skit_errno_to_cstr( char *buf, size_t buf_size);

#endif
