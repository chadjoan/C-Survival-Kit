
#ifndef SKIT_FEATURE_EMULATION_CALL_INCLUDED
#define SKIT_FEATURE_EMULATION_CALL_INCLUDED

#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/throw.h"

/** 
Evaluates the given expression while creating a stack entry at this point
in the code.  The whole purpose of doing this is to provide better debug
info in stack traces.  It is tolerable to forget to use this when calling
into other functions because the exception handling mechanism will still
be able to return to the nearest enclosing sTRY-sCATCH.  Wrapping function
calls in this macro is desirable though, because the calling file, line,
and function name will be absent from stack traces if this is not used.
*/
#define sCALL(expr) /* */ \
	do { \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
		\
		/* Save a copies of the current stack positions. */ \
		skit_thread_context_pos __skit_thread_ctx_pos;\
		skit_save_thread_context_pos(skit_thread_ctx, &__skit_thread_ctx_pos); \
		/* This ensures that we can end up where we started in this call, */ \
		/*   even if the callee messes up the jmp_stack. */ \
		/* The jmp_stack could be messed up if someone tries to jump */ \
		/*   execution out of an sTRY-sCATCH or scope guard block */ \
		/*   (using break/continue/goto/return) and somehow manages to succeed, */ \
		/*   thus preventing pop calls that are supposed to match push calls. */ \
		\
		skit_debug_info_store( \
			skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc), \
			__LINE__,__FILE__,__func__); \
		\
		SKIT_FEATURE_TRACE("Skit_jmp_stack_alloc\n"); \
		if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack, &skit_malloc)) == 0 ) { \
			SKIT_FEATURE_TRACE("%s, %d.40: sCALL.setjmp\n", __FILE__, __LINE__); \
			(expr); \
		} else { \
			SKIT_FEATURE_TRACE("%s, %d.43: sCALL.longjmp\n", __FILE__, __LINE__); \
			skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack); \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			skit_reconcile_thread_context(skit_thread_ctx, &__skit_thread_ctx_pos); \
			__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
		} \
		\
		skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack); \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
		skit_reconcile_thread_context(skit_thread_ctx, &__skit_thread_ctx_pos); \
		\
		SKIT_FEATURE_TRACE("%s, %d.54: sCALL.success\n", __FILE__, __LINE__); \
	} while (0)

#endif
