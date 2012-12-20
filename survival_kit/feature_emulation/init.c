
#ifdef __DECC
#pragma module skit_feature_emu_init
#endif

#include <stdlib.h>
#include <stdio.h>

#include "survival_kit/misc.h" /* skit_die */
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/debug.h"


static void skit_thread_context_init( skit_thread_context *ctx )
{
	skit_jmp_fstack_init(&ctx->try_jmp_stack);
	skit_jmp_fstack_init(&ctx->exc_jmp_stack);
	skit_jmp_fstack_init(&ctx->scope_jmp_stack);
	skit_debug_fstack_init(&ctx->debug_info_stack);
	skit_exc_fstack_init(&ctx->exc_instance_stack);
	
	/* 16kB has GOT to be enough.  Riiight? */
	ctx->error_text_buffer_size = 16384;
	ctx->error_text_buffer = (char*)skit_malloc(ctx->error_text_buffer_size);
	if ( ctx->error_text_buffer == NULL )
		ctx->error_text_buffer_size = 0;
	
	/* TODO: BUG: This works amazingly well for how broken it is! */
	/*   The problem is that we setjmp and then return from the function. */
	/*   After that the main routine will call some other function that */
	/*   will almost assuredly take up residence in this functions stack */
	/*   space. */
	/*   Now, whenever we jump back to this place, the memory used to store */
	/*   state for this function (notable the '*ctx' argument) will be */
	/*   potentially ravaged.  */
	/*   This init function should probably be a MACRO that performs the */
	/*   setjmp inside the thread's entry function so that the stack space */
	/*   used here will for-sure be valid later on. */
	/*   For now, the undefined behavior seems to work quite well! */
	/*     *ducks and crosses fingers*      */
	if( setjmp( *skit_jmp_fstack_alloc(&ctx->exc_jmp_stack, &skit_malloc) ) != 0 )
	{
		/* jmp'ing from god-knows-where. */
		/* At least once the ctx->exc_instance_stack member was fubar'd once */
		/*   execution got to this point, so we should probably restore this */
		/*   context before proceeding.  Otherwise: random segfault time! */
		ctx = skit_thread_context_get();
	
		/* Uncaught exception(s)!  We're going down! */
		while ( ctx->exc_instance_stack.used.length > 0 )
		{
			fprintf(stderr,"Attempting to print exception:\n");
			skit_print_exception( skit_exc_fstack_pop(&ctx->exc_instance_stack) );
			fprintf(stderr,"\n");
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
}

void skit_features_thread_init()
{
	skit_thread_context *ctx = (skit_thread_context*)skit_malloc(sizeof(skit_thread_context));
	skit_thread_context_init(ctx);
	pthread_setspecific(skit_thread_context_key, (void*)ctx);
}
