
#ifndef SKIT_FEMU_INIT_INCLUDED
#define SKIT_FEMU_INIT_INCLUDED

#include "survival_kit/feature_emulation/init.h"
#include "survival_kit/feature_emulation/thread_context.h"

/* Internal: users should call skit_init() instead. */
void skit_features_init();

/* Internal: establishes the stack start. */
#define SKIT_THREAD_CONTEXT_INIT(ctx) \
	( \
		ctx = _skit_create_thread_context(), \
		\
		/* This establishes the bottom of the stack for exception handling. */ \
		/* It MUST be impossible for other code to jump here when the function */ \
		/*   enclosing this macro returns.  */ \
		/* This means that _skit_thread_context_dtor must be called before the */ \
		/*   enclosing function returns. */ \
		( setjmp( *skit_jmp_fstack_alloc(&ctx->exc_jmp_stack, &skit_malloc) ) != 0 ) ? \
		( \
			/* Uncaught exception(s)!  We're going down! */ \
			skit_print_uncaught_exceptions(ctx), \
			\
			/* Attempt to uninitialize gracefully. */ \
			/* This is important for avoiding memory leaks in multithreaded programs. */ \
			SKIT_THREAD_EXIT_POINT(skit_thread_ctx), \
			\
			/* Bail. */ \
			skit_die("Uncaught exception(s)."), \
			1 \
		) : ( 1 ), \
		1 \
	)

#define SKIT_THREAD_CONTEXT_CLEANUP(ctx) \
	( \
		_skit_thread_context_dtor(ctx), \
		1 \
	)
	
#endif
