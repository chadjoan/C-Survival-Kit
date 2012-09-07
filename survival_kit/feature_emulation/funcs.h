
#ifndef SKIT_FEATURE_EMULATION_FUNCS_INCLUDED
#define SKIT_FEATURE_EMULATION_FUNCS_INCLUDED

#include <stdio.h>
#include <pthread.h>

/* Debugging twiddly knobs.  Useful if you have the misfortune of needing to
// debug the code that is supposed to make debugging easier. */
#define SKIT_DO_FEATURE_EMULATION_TRACING 1
#if SKIT_DO_FEATURE_EMULATION_TRACING == 1
	#define SKIT_FEATURE_TRACE(...) printf(__VA_ARGS__)
#else
	#define SKIT_FEATURE_TRACE(...)
#endif

#include "survival_kit/feature_emulation/types.h"

extern int init_was_called;
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



#endif
