#ifndef SKIT_GENERATED_EXCEPTION_DEFS_INCLUDED
#define SKIT_GENERATED_EXCEPTION_DEFS_INCLUDED

#include <unistd.h> /* For ssize_t */
#include <inttypes.h>

#define skit_err_code ssize_t

int skit_exception_is_a( skit_err_code ecode1, skit_err_code ecode2 );

/*
TODO:
extern skit_err_code SKIT_EXCEPTION;
extern skit_err_code SKIT_FATAL_EXCEPTION;
#define SKIT_REGISTER_EXCEPTION(ecode, parent, msg) (_skit__register_exception((ecode), (parent), #ecode#, (msg)))
void _skit__register_exception( skit_err_code ecode, skit_err_code parent, const char *ecode_name, const char *default_msg );

void skit_xx_init()
{
	skit_register_exception(SKIT_SOCKET_EXCEPTION, SKIT_EXCEPTION, "socket exception.");
}
*/

/* Exception list notes: */
/* Never use 0, it has a special meaning for setjmp/longjmp. */
/* 1 is pretty dangerous too. */
/* __SKIT_EXC_TABLE_SIZE will be the number of definitions, plus 2 for the values 0 and 1 which aren't used for error codes. */
/*   (Easy calculation: take the last ID and add 1.) */
/* The rest of the numbers must appear consecutively, or it will mess up the inheritance table. */
/*	X(EXCEPTION_NAME,             PARENT_NAME,           ID,  "MESSAGE") */
#define __SKIT_EXC_TABLE_SIZE 13
#define SKIT_EXCEPTION_LIST(X) \
	X(SKIT_EXCEPTION,             SKIT_EXCEPTION,          2, "An exception was thrown.") \
	X(SKIT_FATAL_EXCEPTION,       SKIT_FATAL_EXCEPTION,    3, "An unrecoverable exception was thrown.") \
	X(SKIT_BREAK_IN_TRY_CATCH,    SKIT_EXCEPTION,          4, "Attempt to use the built-in 'break' statement while in an sTRY-sCATCH block.") \
	X(SKIT_CONTINUE_IN_TRY_CATCH, SKIT_EXCEPTION,          5, "Attempt to use the built-in 'continue' statement while in an sTRY-sCATCH block.") \
	X(SKIT_SOCKET_EXCEPTION,      SKIT_EXCEPTION,          6, "socket exception.") \
	X(SKIT_OUT_OF_BOUNDS,         SKIT_EXCEPTION,          7, "Out of bounds exception.") \
	X(SKIT_NOT_IMPLEMENTED,       SKIT_EXCEPTION,          8, "The requested feature is not implemented.") \
	X(SKIT_IO_EXCEPTION,          SKIT_EXCEPTION,          9, "Generic I/O exception.") \
	X(SKIT_FILE_IO_EXCEPTION,     SKIT_IO_EXCEPTION,      10, "File I/O exception.") \
	X(SKIT_TCP_IO_EXCEPTION,      SKIT_IO_EXCEPTION,      11, "TCP Network I/O exception.") \
	X(SKIT_FILE_NOT_FOUND,        SKIT_FILE_IO_EXCEPTION, 12, "File not found.")
/*           Don't forget to change __SKIT_EXC_TABLE_SIZE ^^           */

#define SKIT_DEFINE_EXCEPTION_IDS(EXC_NAME, PARENT_NAME, UID, ERR_TXT) \
	EXC_NAME = UID,
enum {
SKIT_EXCEPTION_LIST(SKIT_DEFINE_EXCEPTION_IDS)
};
#undef SKIT_DEFINE_EXCEPTION_IDS

#endif