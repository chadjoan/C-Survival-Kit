
#ifndef SKIT_FEATURE_EMULATION_CALL_INCLUDED
#define SKIT_FEATURE_EMULATION_CALL_INCLUDED

#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/assert.h"
#define SKIT_TRACE_INTERNAL(assignment, returned_expr) /* */ \
	( \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1, \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY, \
		\
		/* Save a copies of the current stack positions. */ \
		skit_save_thread_context_pos(skit_thread_ctx, &__skit_thread_ctx_pos), \
		/* This ensures that we can end up where we started in this call, */ \
		/*   even if the callee messes up the jmp_stack. */ \
		/* The jmp_stack could be messed up if someone tries to jump */ \
		/*   execution out of an sTRY-sCATCH or scope guard block */ \
		/*   (using break/continue/goto/return) and somehow manages to succeed, */ \
		/*   thus preventing pop calls that are supposed to match push calls. */ \
		\
		skit_debug_info_store( \
			skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc), \
			__LINE__,__FILE__,__func__), \
		\
		SKIT_FEATURE_TRACE("Skit_jmp_stack_alloc\n"), \
		(setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack, &skit_malloc)) == 0 ) ? \
		( \
			SKIT_FEATURE_TRACE("%s, %d.40: sTRACE.setjmp\n", __FILE__, __LINE__), \
			SKIT_FEATURE_TRACE("sTRACE: exc_jmp_stack.size == %ld\n", skit_thread_ctx->exc_jmp_stack.used.length), \
			assignment, \
			1 \
		) : ( \
			SKIT_FEATURE_TRACE("%s, %d.43: sTRACE.longjmp\n", __FILE__, __LINE__), \
			skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack), \
			SKIT_FEATURE_TRACE("sTRACE: exc_jmp_stack.size == %ld\n", skit_thread_ctx->exc_jmp_stack.used.length), \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack), \
			skit_reconcile_thread_context(skit_thread_ctx, &__skit_thread_ctx_pos), \
			__SKIT_PROPOGATE_THROWN_EXCEPTIONS \
		), \
		\
		skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack), \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack), \
		skit_reconcile_thread_context(skit_thread_ctx, &__skit_thread_ctx_pos), \
		\
		SKIT_FEATURE_TRACE("%s, %d.54: sTRACE.success\n", __FILE__, __LINE__), \
		returned_expr \
	)


/* Here, we exploit a C feature called "compound literals" to create stack */
/*   space for the temporary used to hold the expression result.  This has */
/*   the advantage of NOT requiring a declaration to be placed in the      */
/*   sETRACE logic, which allows us to use it as an expression.            */
/*   (Expressions cannot contain declarations.)                            */
#define sETRACE_ASSIGNMENT(expr) /* */ \
			(__skit_sTRACE_return_value = (void*)(char[sizeof(expr)]){""}, \
			*(__typeof__(expr)*)__skit_sTRACE_return_value = (expr))

#define sETRACE_RETURN(expr) /* */ \
	(*((__typeof__(expr)*)__skit_sTRACE_return_value))


/** 
Evaluates the given expression while creating a stack entry at this point
in the code.  The whole purpose of doing this is to provide better debug
info in stack traces.  It is tolerable to forget to use this when calling
into other functions because the exception handling mechanism will still
be able to return to the nearest enclosing sTRY-sCATCH.  Wrapping function
calls in this macro is desirable though, because the calling file, line,
and function name will be absent from stack traces if this is not used.
*/	
#define sTRACE(statement) /* */ \
	do { SKIT_TRACE_INTERNAL((statement), (void)__skit_sTRACE_return_value); } while (0)

/**
This serves the same function as sTRACE, but with the advantage that it can
forward expression values from its argument.  For example, it is possible to
do this with sETRACE:
  int bar() { return 42; }
  ...
  int foo = sETRACE(bar());

The disadvantage of sETRACE is that it cannot be used on void expressions:
  void bar() {}
  ...
  sETRACE(bar()); // Error!  Attempt to assign a void to a non-void.
*/
#define sETRACE(expr) /* */ \
	SKIT_TRACE_INTERNAL(sETRACE_ASSIGNMENT(expr), sETRACE_RETURN(expr))

#endif
