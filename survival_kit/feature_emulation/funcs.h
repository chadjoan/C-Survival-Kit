
#ifndef SKIT_FEATURE_EMULATION_FUNCS_INCLUDED
#define SKIT_FEATURE_EMULATION_FUNCS_INCLUDED

#include <pthread.h>

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

/** Prints the given exception to stdout. */
void skit_print_exception(exception *e);


#endif
