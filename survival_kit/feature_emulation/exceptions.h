#ifndef SKIT_EXCEPTIONS_INCLUDED
#define SKIT_EXCEPTIONS_INCLUDED

#include <unistd.h> /* For ssize_t */
#include <inttypes.h>

#define skit_err_code ssize_t

/**
This is the intended intial value for exceptions.
Use this when definiting exceptions if you need an initializer that is
guaranteed to not be present in the exception table.
This is (intentionally) not a valid exception value and cannot be registered
or thrown.
*/
#define SKIT_EXCEPTION_INITIAL_VAL (0xFFFFFFFFUL)

/* TODO: A lot of these definitions belong in other places. */
extern skit_err_code SKIT_EXCEPTION;
extern skit_err_code SKIT_FATAL_EXCEPTION;
extern skit_err_code SKIT_BREAK_IN_TRY_CATCH;
extern skit_err_code SKIT_CONTINUE_IN_TRY_CATCH;
extern skit_err_code SKIT_SOCKET_EXCEPTION;
extern skit_err_code SKIT_OUT_OF_BOUNDS;
extern skit_err_code SKIT_NOT_IMPLEMENTED;
extern skit_err_code SKIT_IO_EXCEPTION;
extern skit_err_code SKIT_FILE_IO_EXCEPTION;
extern skit_err_code SKIT_TCP_IO_EXCEPTION;
extern skit_err_code SKIT_FILE_NOT_FOUND;

int skit_exception_is_a( skit_err_code ecode1, skit_err_code ecode2 );

/**
Call this during initialization to register new exception types.

It will initialize it's first argument with a globally unique value that
establishes the exception's identity.  It will also place this value in the
exception table, thus making it correctly derive from exception type given
in the second argument.

This is NOT thread safe.  Do this in your very first initialization steps,
before any threads are spawned.

Example:

// In myfile.h: 
extern skit_err_code MY_EXCEPTION1;
extern skit_err_code MY_EXCEPTION2;

... other definitions ...

// In myfile.c:
#include "survival_kit/feature_emulation.h"

skit_err_code MY_EXCEPTION1;
skit_err_code MY_EXCEPTION2;

skit_init_exceptions()
{
	//                      Exception,     Derived from,   Default message
	SKIT_REGISTER_EXCEPTION(MY_EXCEPTION1, SKIT_EXCEPTION, "Something bad happened.");
	SKIT_REGISTER_EXCEPTION(MY_EXCEPTION2, MY_EXCEPTION1,  "Something more specific happened.");
}

... other implementations ...

*/
#define SKIT_REGISTER_EXCEPTION(ecode, parent, msg) (_skit__register_exception(&(ecode), &(parent), #ecode, (msg)))

/* Don't call this directly. */
void _skit__register_exception( skit_err_code *ecode, const skit_err_code *parent, const char *ecode_name, const char *default_msg );

void skit_init_exceptions();

#endif