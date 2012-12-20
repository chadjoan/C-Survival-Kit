#ifndef SKIT_EXCEPTIONS_INCLUDED
#define SKIT_EXCEPTIONS_INCLUDED

#include <unistd.h> /* For ssize_t */
#include <inttypes.h>
#include <stdarg.h>

#include "survival_kit/feature_emulation/frame_info.h"

#ifndef skit_thread_context
struct skit_thread_context;
#define skit_thread_context struct skit_thread_context
#endif

/**
Exceptions in C Survival Kit are not polymorphic objects like in many languages
that have exceptions.  They still, however, have a hierarchy.  Each node in this
hierarchy is a possible exception type and is identified with a skit_err_code
value.  This is a unique integer of some kind that distinguishes one kind of
exception from another.
*/
#define skit_err_code ssize_t

/** */
typedef struct skit_exception skit_exception;
struct skit_exception
{
	/* Implementation note: keep this small, it will be returned by-value from functions a lot. */
	skit_err_code error_code;  /** 0 should always mean "no error". TODO: Values of 0 don't make sense anymore.  It was useful for an inferior exceptions implementation.  Followup needed? */
	char *error_text;    /** READ ONLY: a description for the error. */
	size_t error_len;    /** READ ONLY: the length of error_text, minus the trailing nul character. */
	
	/** 
	Points to the stack that stores the exception's debug information.  
	This is usually the thread context's debug_info_stack, but it can be 
	different for exceptions created with SKIT_PUSH_EXCEPTION, since it
	is often necessary to copy the stack trace to preserve it whenever 
	calling code is allowed to continue executing and altering the stack.
	*/
	skit_debug_stack  *debug_info_stack; 
	
	/** Points to the point in the frame info stack where the exception happened. */
	skit_debug_stnode *frame_info_node;
};

#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_NAME exc
#include "survival_kit/templates/stack.h"
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME

/* This must be expanded after skit_exc_fstack is defined. */
/* #include "survival_kit/feature_emulation/thread_context.h" */

/**
This is the intended intial value for the global write-once skit_err_code 
variables.
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

/* Internal: Frees all resources tied to the exception. */
/*           (but not the exception itself.) */
/* Code outside of feature_emulation submodules should use */
/*   SKIT_EXCEPTION_FREE instead. */
void skit_exception_dtor( skit_thread_context *ctx, skit_exception *exc );

/*
This skit_throw_exception_no_ctx definition is used to break macro recursion.
This definition is duplicated in "survival_kit/feature_emulation/stack.c" and
  "survival_kit/feature_emulation/fstack.c".  If this ever changes, then those
  must be updated.
The recursion happens when (f)stack.c includes this module which includes
  feature_emulation/types.h which then redefines SKIT_T_ELEM_TYPE and 
  includes stack again.  
There is no way to save the template parameters (like SKIT_T_ELEM_TYPE or
  SKIT_T_NAME) and forcing callers to '#include "survival_kit/feature_emulation/types.h"'
  before including (f)stack.c is inconsistent and undesirable.
The strategy then is to call an exception throwing function directly, without
  the aid of macros.  This requires keeping the definitions of this function
  in sync.
Due to the lack of the ability to include "survival_kit/feature_emulation/types.h"
  this definition is not allowed to use skit_thread_context* or anything that uses
  fstacks/stacks.  It obtains this information in the .c file by calling
  skit_thread_context_get.  Beware: it also has no way of expanding the
  __SKIT_SCAN_SCOPE_GUARDS macro and thus can't finalize scope guards.  
  Do not use this in a function with scope guards.
*/
void skit_throw_exception_no_ctx(
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...);

/*
This is the more common alternative (still internal-use-only) to
skit_throw_exception_no_ctx.  
This is used by the ubiquitous sTHROW macro and also the SKIT_PUSH_EXCEPTION
macro, in conjunction with __SKIT_PROPOGATE_THROWN_EXCEPTIONS when needed.
It avoids making repeated calls to skit_thread_context_get.
*/
void skit_push_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...);

/* Same as skit_push_exception, but accepts a va_list instead of raw variadic
arguments. */
void skit_push_exception_va(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	va_list var_args);
	
/*
This works like skit_push_exception, except that it allows the caller to store
the exception object and throw it at a later time, or just use the stored
exceptions as a way of generating error reports (ex: non-fatal parser errors).
*/
skit_exception *skit_new_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...);
	
/* Same as skit_new_exception, but accepts a va_list instead of raw variadic
arguments. */
skit_exception *skit_new_exception_va(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	va_list var_args);

/*
Internal function used to do the same thing as skit_push_exceptions, but with
an exception object provided instead of exception info.  This function is
important for allowing the caller to create exceptions and save the debugging
into until a later point in execution.
*/
void skit_push_exception_obj(skit_thread_context *skit_thread_ctx, skit_exception *exc);

/* 
Unwinds the stack while propogating the last thrown exception.
This is used by both __SKIT_PROPOGATE_THROWN_EXCEPTIONS and 
skit_throw_exception_no_ctx.
It should not be called by user code or anything else that doesn't truly need
access to the internals of exception handling.
*/
void skit_propogate_exceptions( skit_thread_context *skit_thread_ctx );

/** Prints the given exception to stdout. */
void skit_print_exception(skit_exception *e);

#ifdef skit_thread_context
#undef skit_thread_context
#endif

#endif