#ifndef SKIT_GENERATED_EXCEPTION_DEFS_INCLUDED
#define SKIT_GENERATED_EXCEPTION_DEFS_INCLUDED

#include <unistd.h> /* For ssize_t */
#include <inttypes.h>

#define skit_err_code ssize_t

/* __EXC_TABLE_SIZE will be the number of definitions, plus 2 for the values 0 and 1 which aren't used for error codes. */
#define __EXC_TABLE_SIZE 8
extern skit_err_code __exc_inheritance_table[__EXC_TABLE_SIZE];

int exception_is_a( skit_err_code ecode1, skit_err_code ecode2 );

#define GENERIC_EXCEPTION     2
#define FATAL_EXCEPTION       3
#define BREAK_IN_TRY_CATCH    4
#define CONTINUE_IN_TRY_CATCH 5
#define SOCKET_EXCEPTION      6
#define OUT_OF_BOUNDS         7

#endif