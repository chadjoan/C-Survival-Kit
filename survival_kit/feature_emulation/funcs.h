
#ifndef SKIT_FEATURE_EMULATION_FUNCS_INCLUDED
#define SKIT_FEATURE_EMULATION_FUNCS_INCLUDED

#include <stdio.h>
#include <pthread.h>

/* Debugging twiddly knobs.  Useful if you have the misfortune of needing to
// debug the code that is supposed to make debugging easier. */
#define SKIT_DO_FEATURE_EMULATION_TRACING 0
#if SKIT_DO_FEATURE_EMULATION_TRACING == 1
	#define SKIT_FEATURE_TRACE(...) printf(__VA_ARGS__)
#else
	#define SKIT_FEATURE_TRACE(...)
#endif

#include "survival_kit/feature_emulation/types.h"

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

/*
This skit_throw_exception_no_ctx definition is used to break macro recursion.
This definition is duplicated in "survival_kit/feature_emulation/stack.c" and
  "survival_kit/feature_emulation/fstack.c".  If this ever changes, then those
  must be updated.
The recursion happens when (f)stack.c includes this module which includes
  feature_emulation/types.h which then redefines SKIT_T_ELEM_TYPE and 
  includes stack again.  
There is no way to save the template parameters (like SKIT_T_ELEM_TYPE or
  SKIT_T_PREFIX) and forcing callers to '#include "survival_kit/feature_emulation/types.h"'
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
skit_throw_exception_no_ctx.  It is more desirable to use because
it can handle scope guard scanning due to the skit_scope_ctx parameter.
It also avoids making repeated calls to skit_thread_context_get.
*/
void skit_throw_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...);

/** Prints the given exception to stdout. */
void skit_print_exception(skit_exception *e);

/** Prints the current stack trace to a string and returns it.
// For now, this uses statically allocated memory for the returned string.
// It will eventually allocate dynamic memory for the string and the caller
// will be responsible for free'ing the string.
// TODO: dynamically allocate the string.
*/
#define skit_stack_trace_to_str() skit_stack_trace_to_str_expr(__LINE__,__FILE__,__func__)
char *skit_stack_trace_to_str_expr( uint32_t line, const char *file, const char *func );

/** Prints the current stack trace to stdout. */
/* These definitions are duplicated in "survival_kit/assert.h" to prevent macro recursion. */
/* Be sure to change those definitions if altering these. */
#define skit_print_stack_trace() skit_print_stack_trace_func(__LINE__,__FILE__,__func__)
void skit_print_stack_trace_func( uint32_t line, const char *file, const char *func );

#endif
