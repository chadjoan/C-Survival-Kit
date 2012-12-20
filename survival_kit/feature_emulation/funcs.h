
#ifndef SKIT_FEATURE_EMULATION_FUNCS_INCLUDED
#define SKIT_FEATURE_EMULATION_FUNCS_INCLUDED

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>

/* Debugging twiddly knobs.  Useful if you have the misfortune of needing to
// debug the code that is supposed to make debugging easier. */
#define SKIT_DO_FEATURE_EMULATION_TRACING 0
#if SKIT_DO_FEATURE_EMULATION_TRACING == 1
	#define SKIT_FEATURE_TRACE(...) (printf(__VA_ARGS__))
#else
	#define SKIT_FEATURE_TRACE(...) ((void)1)
#endif

#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/scope.h"

extern pthread_key_t skit_thread_context_key;

/* Internal: users should call skit_init() instead. */
void skit_features_init();

/* Internal: users should call skit_thread_init() instead. */
void skit_features_thread_init();

/* Internal: used in macros to emulate language features. */
skit_thread_context *skit_thread_context_get();

/* More internals. */
void skit_save_thread_context_pos( skit_thread_context *ctx, skit_thread_context_pos *pos );
void skit_reconcile_thread_context( skit_thread_context *ctx, skit_thread_context_pos *pos );
void skit_debug_info_store( skit_frame_info *dst, int line, const char *file, const char *func );
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

/** 
Prints the current stack trace to a string and returns it.
In order to prevent dynamic allocation, this will use a thread-local buffer 
for storing the string.  This buffer may be overwritten by other calls to
skit_* functions, so be sure to copy it to somewhere else before calling
those functions.
*/
#define skit_stack_trace_to_str() skit_stack_trace_to_str_expr(__LINE__,__FILE__,__func__)
const char *skit_stack_trace_to_str_expr( uint32_t line, const char *file, const char *func );

/** Prints the current stack trace to stdout. */
/* These definitions are duplicated in "survival_kit/assert.h" to prevent macro recursion. */
/* Be sure to change those definitions if altering these. */
#define skit_print_stack_trace() skit_print_stack_trace_func(__LINE__,__FILE__,__func__)
void skit_print_stack_trace_func( uint32_t line, const char *file, const char *func );

#endif
