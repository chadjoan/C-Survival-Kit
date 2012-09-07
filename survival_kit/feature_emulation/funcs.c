#include <stdio.h>

#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/misc.h"

pthread_key_t skit_thread_context_key;

static void skit_thread_context_init( skit_thread_context *ctx )
{
	skit_jmp_fstack_init(&ctx->try_jmp_stack);
	skit_jmp_fstack_init(&ctx->exc_jmp_stack);
	skit_jmp_fstack_init(&ctx->scope_jmp_stack);
	skit_debug_fstack_init(&ctx->debug_info_stack);
	skit_exc_fstack_init(&ctx->exc_instance_stack);
	
	if( setjmp( *skit_jmp_fstack_alloc(&ctx->exc_jmp_stack, &skit_malloc) ) != 0 )
	{
		/* Uncaught exception(s)!  We're going down! */
		while ( ctx->exc_instance_stack.used.length > 0 )
		{
			skit_print_exception( skit_exc_fstack_pop(&ctx->exc_instance_stack) );
		}
		skit_die("Uncaught exception(s).");
	}
}

static void skit_thread_context_dtor(void *ctx_ptr)
{
	skit_thread_context *ctx = (skit_thread_context*)ctx_ptr;
	(void)ctx;
	/* Do nothing for now. TODO: This will be important for multithreading. */
}

void skit_features_init()
{
	pthread_key_create(&skit_thread_context_key, skit_thread_context_dtor); 
	skit_features_thread_init();
}

void skit_features_thread_init()
{
	skit_thread_context *ctx = (skit_thread_context*)skit_malloc(sizeof(skit_thread_context));
	skit_thread_context_init(ctx);
	pthread_setspecific(skit_thread_context_key, (void*)ctx);
}

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
	fprintf(stderr,"  a TRY-CATCH block or a SCOPE guard.  Jumping away from within a TRY-CATCH\n");
	fprintf(stderr,"  block or SCOPE guard with raw C primitives can lead to very buggy, bizarre,\n");
	fprintf(stderr,"  and inconsistent runtime behavior.  Just don't do it.\n");
	/* TODO: there should be some non-primitive control constructs that should be mentioned here
	 *   as a way of accomplishing desired logic in TRY-CATCH statements. */
	/* TODO: Although we can probably fix any problems the user creates for themselves, dieing might be better than warning. */
	
	printf("Printing stack trace:\n");
	printf("%s",stack_trace_to_str());
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

void skit_debug_info_store( skit_frame_info *dst, int line, const char *file, const char *func )
{
	/* ERR_UTIL_TRACE("%s, %li: skit_debug_info_store(...,%li, %s, %s)\n", file, line, line, file, func); */
	
	dst->line_number = line;
	dst->file_name = file;
	dst->func_name = func;
}


void skit_print_exception(skit_exception *e)
{
	printf("%s\n",e->error_text);
	printf("%s\n",stack_trace_to_str_internal(e->frame_info_index));
}
