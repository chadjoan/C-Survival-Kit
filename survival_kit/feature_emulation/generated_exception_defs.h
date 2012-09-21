#ifndef SKIT_GENERATED_EXCEPTION_DEFS_INCLUDED
#define SKIT_GENERATED_EXCEPTION_DEFS_INCLUDED

#include <unistd.h> /* For ssize_t */
#include <inttypes.h>

#define skit_err_code ssize_t

/* __EXC_TABLE_SIZE will be the number of definitions, plus 2 for the values 0 and 1 which aren't used for error codes. */
#define __EXC_TABLE_SIZE 8
extern skit_err_code __exc_inheritance_table[__EXC_TABLE_SIZE];

int exception_is_a( skit_err_code ecode1, skit_err_code ecode2 );

/* Exception list notes: */
/* Never use 0, it has a special meaning for setjmp/longjmp. */
/* 1 is pretty dangerous too. */
/*	X(EXCEPTION_NAME,        PARENT_NAME,      ID,  "MESSAGE") */
#define SKIT_EXCEPTION_LIST(X) \
	X(GENERIC_EXCEPTION,     GENERIC_EXCEPTION, 2, "An exception was thrown.") \
	X(FATAL_EXCEPTION,       FATAL_EXCEPTION,   3, "An unrecoverable exception was thrown.") \
	X(BREAK_IN_TRY_CATCH,    GENERIC_EXCEPTION, 4, "Attempt to use the built-in 'break' statement while in an sTRY-sCATCH block.") \
	X(CONTINUE_IN_TRY_CATCH, GENERIC_EXCEPTION, 5, "Attempt to use the built-in 'continue' statement while in an sTRY-sCATCH block.") \
	X(SOCKET_EXCEPTION,      GENERIC_EXCEPTION, 6, "socket exception.") \
	X(OUT_OF_BOUNDS,         GENERIC_EXCEPTION, 7, "Out of bounds exception.")

#define SKIT_DEFINE_EXCEPTION_IDS(EXC_NAME, PARENT_NAME, UID, ERR_TXT) \
	EXC_NAME = UID,
enum {
SKIT_EXCEPTION_LIST(SKIT_DEFINE_EXCEPTION_IDS)
};
#undef SKIT_DEFINE_EXCEPTION_IDS

#endif