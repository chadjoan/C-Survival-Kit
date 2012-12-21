
#ifndef SKIT_FEMU_THREAD_CTX_INCLUDED
#define SKIT_FEMU_THREAD_CTX_INCLUDED

#include <pthread.h>
#include <stdio.h> 

#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/setjmp/jmp_fstack.h"

/* Internal use only.  This may not be extern anymore once initialization gets
 * shuffled around to prevent main's setjmp from living in another stack frame.
 */
extern pthread_key_t skit_thread_context_key;

/* 
Used internally to save the position of the call stack before CALLs and other
nested transfers.  The skit_thread_context_pos struct can then be used to
reconcile against the skit_thread_context later when the nested code completes.
The skit_reconcile_thread_context function is used for this.
*/
typedef struct skit_thread_context_pos skit_thread_context_pos; 
struct skit_thread_context_pos
{
	ssize_t try_jmp_pos;
	ssize_t exc_jmp_pos;
	ssize_t scope_jmp_pos;
	ssize_t debug_info_pos;
};

/**
This structure contains thread-local data structures needed to implement the
emulation of various language features found in this module.
*/
typedef struct skit_thread_context skit_thread_context;
struct skit_thread_context
{
	skit_jmp_fstack   try_jmp_stack;
	skit_jmp_fstack   exc_jmp_stack;
	skit_jmp_fstack   scope_jmp_stack;
	skit_debug_fstack debug_info_stack;
	skit_exc_fstack   exc_instance_stack;
	char *error_text_buffer;
	int error_text_buffer_size;
};

/* Internal: used in macros to emulate language features. */
skit_thread_context *skit_thread_context_get();

/* More internals. */
void skit_save_thread_context_pos( skit_thread_context *ctx, skit_thread_context_pos *pos );
void skit_reconcile_thread_context( skit_thread_context *ctx, skit_thread_context_pos *pos );
ssize_t skit_stack_depth( skit_thread_context *ctx );
skit_thread_context *_skit_create_thread_context();
void _skit_thread_context_ctor( skit_thread_context *ctx );
void _skit_thread_context_dtor( skit_thread_context *ctx );

#endif
