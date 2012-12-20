
#ifdef __DECC
#pragma module skit_feature_emu_thread_ctx
#endif

#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "survival_kit/misc.h" /* skit_die */
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/stack_trace.h"
#include "survival_kit/feature_emulation/debug.h"

pthread_key_t skit_thread_context_key;

skit_thread_context *skit_thread_context_get()
{
	return (skit_thread_context*)pthread_getspecific(skit_thread_context_key);
}

void skit_save_thread_context_pos( skit_thread_context *skit_thread_ctx, skit_thread_context_pos *pos )
{
	pos->try_jmp_pos    = skit_thread_ctx->try_jmp_stack.used.length;
	pos->exc_jmp_pos    = skit_thread_ctx->exc_jmp_stack.used.length;
	pos->scope_jmp_pos  = skit_thread_ctx->scope_jmp_stack.used.length;
	pos->debug_info_pos = skit_thread_ctx->debug_info_stack.used.length;
}

static void skit_fstack_reconcile_warn( ssize_t expected, ssize_t got, char *name )
{
	fprintf(stderr,"Warning: %s was unbalanced after the most recent return.\n", name);
	fprintf(stderr,"  Expected size: %li\n", expected );
	fprintf(stderr,"  Actual size:   %li\n", got );
	fprintf(stderr,"  This may mean that a goto, break, continue, or return was made while inside\n");
	fprintf(stderr,"  an sTRY-sCATCH block or a sSCOPE guard.  Jumping away from within a\n");
	fprintf(stderr,"  sTRY-sCATCH block or sSCOPE guard with raw C primitives can lead to \n");
	fprintf(stderr,"  very buggy, bizarre, and inconsistent runtime behavior.\n");
	fprintf(stderr,"  Just don't do it.\n");
	/* TODO: there should be some non-primitive control constructs that should be mentioned here
	 *   as a way of accomplishing desired logic in sTRY-sCATCH statements. */
	/* TODO: Although we can probably fix any problems the user creates for themselves, dieing might be better than warning. */
	
	printf("Printing stack trace:\n");
	printf("%s",skit_stack_trace_to_str());
}

#define SKIT_FSTACK_RECONCILE(stack, prev_length, name, name_str) \
	do { \
		if ( (stack).used.length > prev_length ) \
		{ \
			skit_fstack_reconcile_warn(prev_length, (stack).used.length, name_str); \
			while ( (stack).used.length > prev_length ) \
				name##_pop(&(stack)); \
		} \
		else if ( (stack).used.length < prev_length ) \
		{ \
			skit_fstack_reconcile_warn(prev_length, (stack).used.length, name_str); \
			skit_die("\nStack was popped too far!  This cannot be recovered.\n"); \
		} \
	} while (0)

void skit_reconcile_thread_context( skit_thread_context *ctx, skit_thread_context_pos *pos )
{
	SKIT_FSTACK_RECONCILE(ctx->try_jmp_stack,    pos->try_jmp_pos,    skit_jmp_fstack,   "try_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->exc_jmp_stack,    pos->exc_jmp_pos,    skit_jmp_fstack,   "exc_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->scope_jmp_stack,  pos->scope_jmp_pos,  skit_jmp_fstack,   "scope_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->debug_info_stack, pos->debug_info_pos, skit_debug_fstack, "debug_info_stack");
}
