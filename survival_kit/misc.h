
#ifndef SKIT_MISC_INCLUDED
#define SKIT_MISC_INCLUDED

#include <stdlib.h>
#include <stdarg.h>

/** Print the given message along with any other error messages that would
normally get printed at this point if "skit_die" was called. */
void skit_death_cry(char *mess, ...);
void skit_death_cry_va(char *mess, va_list vl); /** ditto */

/** Die without printing any error messages. */
void skit_die_only();

/**
Unconditionally exit the program and print the formatted string 'mess' along
with any other diagnostic information available to the survival kit context.
*/
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

/// This is similar to skit_errno_to_cstr, but this version allows the caller
/// to pass an error code value that is to be used instead of global errno.
const char *skit_error_code_to_cstr( int error_code, char *buf, size_t buf_size);

/// Thread-safe and consistent alternative to the gai_strerror function that
/// is used to stringize error codes returned from getaddrinfo and related
/// functions (anything that returns an EAI_* error code, it seems).
const char *skit_gai_strerror(int errcode);

#endif
