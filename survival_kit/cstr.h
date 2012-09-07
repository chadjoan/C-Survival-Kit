
#ifndef SKIT_CSTR_INCLUDED
#define SKIT_CSTR_INCLUDED

#include <inttypes.h>

/* Do not call this.  Call skit_init or skit_thread_init instead. */
/* See "survival_kit/init.h" */
void skit_cstr_init();
void skit_cstr_thread_init();

/** 
Converts a signed integer into a string.
The string returned is from thread-local statically allocated memory.
*/
char *skit_int_to_cstr(int64_t val);

/** 
Converts an unsigned integer into a string.
The string returned is from thread-local statically allocated memory.
*/
char *skit_uint_to_cstr(uint64_t val);

#endif
