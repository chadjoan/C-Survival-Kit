
#ifndef SKIT_FEATURE_EMULATION_CALL_INCLUDED
#define SKIT_FEATURE_EMULATION_CALL_INCLUDED

#include "survival_kit/init.h"
#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/assert.h"

#define SKIT_TRACE_INTERNAL(original_code, assignment, returned_expr, on_entry, on_exit) /* */ \
	( \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1, \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY, \
		\
		on_entry, \
		\
		SKIT_THREAD_CHECK_ENTRY(skit_thread_ctx), \
		\
		/* Save a copies of the current stack positions. */ \
		skit_save_thread_context_pos(skit_thread_ctx, &skit__thread_ctx_pos), \
		/* This ensures that we can end up where we started in this call, */ \
		/*   even if the callee messes up the jmp_stack. */ \
		/* The jmp_stack could be messed up if someone tries to jump */ \
		/*   execution out of an sTRY-sCATCH or scope guard block */ \
		/*   (using break/continue/goto/return) and somehow manages to succeed, */ \
		/*   thus preventing pop calls that are supposed to match push calls. */ \
		\
		skit_debug_info_store( \
			skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc), \
			__LINE__,__FILE__,__func__, "code: " #original_code), \
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
			skit_reconcile_thread_context(skit_thread_ctx, &skit__thread_ctx_pos), \
			SKIT__PROPOGATE_THROWN_EXCEPTIONS \
		), \
		\
		skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack), \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack), \
		skit_reconcile_thread_context(skit_thread_ctx, &skit__thread_ctx_pos), \
		\
		SKIT_THREAD_CHECK_EXIT(skit_thread_ctx), \
		\
		on_exit, \
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
			(skit__sTRACE_return_value = (void*)(char[sizeof(expr)]){""}, \
			*(__typeof__(expr)*)skit__sTRACE_return_value = (expr))

#define sETRACE_RETURN(expr) /* */ \
	(*((__typeof__(expr)*)skit__sTRACE_return_value))

/* ------------------------------------------------------------------------- */
/* Stuff for sTRACE */

#define sTRACE_HOOKED(statement, on_entry, on_exit) /* */ \
	do { SKIT_TRACE_INTERNAL(statement, (statement), (void)skit__sTRACE_return_value, on_entry, on_exit); } while (0)


#define sTRACE0_HOOKED(statement, on_entry, on_exit) sTRACE_HOOKED(statement, on_entry, on_exit)

#if sTRACE_LEVEL >= 1
#define sTRACE1_HOOKED(statement, on_entry, on_exit) sTRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sTRACE1_HOOKED(statement, on_entry, on_exit) statement
#endif

#if sTRACE_LEVEL >= 2
#define sTRACE2_HOOKED(statement, on_entry, on_exit) sTRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sTRACE2_HOOKED(statement, on_entry, on_exit) statement
#endif

#if sTRACE_LEVEL >= 3
#define sTRACE3_HOOKED(statement, on_entry, on_exit) sTRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sTRACE3_HOOKED(statement, on_entry, on_exit) statement
#endif

#if sTRACE_LEVEL >= 4
#define sTRACE4_HOOKED(statement, on_entry, on_exit) sTRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sTRACE4_HOOKED(statement, on_entry, on_exit) statement
#endif

/** 
Evaluates the given expression while creating a stack entry at this point
in the code.  The whole purpose of doing this is to provide better debug
info in stack traces.  It is tolerable to forget to use this when calling
into other functions because the exception handling mechanism will still
be able to return to the nearest enclosing sTRY-sCATCH.  Wrapping function
calls in this macro is desirable though, because the calling file, line,
and function name will be absent from stack traces if this is not used.

sTRACE statements may optionally have different levels that allow them to
conditionally include traceback information.  This is useful in places that
might be performance critical yet also require debugging.
5 levels are available:
sTRACE0(statement) -- same as sTRACE(statement), always inserts debug info.
sTRACE1(statement) -- Inserts debug info if the macro define sTRACE_LEVEL is >= 1
sTRACE2(statement) -- sTRACE_LEVEL >= 2
sTRACE3(statement) -- sTRACE_LEVEL >= 3
sTRACE4(statement) -- sTRACE_LEVEL >= 4
It is often convenient to alter these using compile time flags:
gcc myfile.c -I<skit_dir> -DsTRACE_LEVEL=5
*/
#define sTRACE(statement) sTRACE_HOOKED(statement, (0), (0))
#define sTRACE0(statement) sTRACE0_HOOKED(statement, (0), (0))
#define sTRACE1(statement) sTRACE1_HOOKED(statement, (0), (0))
#define sTRACE2(statement) sTRACE2_HOOKED(statement, (0), (0))
#define sTRACE3(statement) sTRACE3_HOOKED(statement, (0), (0))
#define sTRACE4(statement) sTRACE4_HOOKED(statement, (0), (0))

/* ------------------------------------------------------------------------- */
/* Stuff for sETRACE */

#define sETRACE_HOOKED(expr, on_entry, on_exit) /* */ \
	SKIT_TRACE_INTERNAL(expr, sETRACE_ASSIGNMENT(expr), sETRACE_RETURN(expr), on_entry, on_exit)


#define sETRACE0_HOOKED(statement, on_entry, on_exit) sETRACE_HOOKED(statement, on_entry, on_exit)

#if sTRACE_LEVEL >= 1
#define sETRACE1_HOOKED(statement, on_entry, on_exit) sETRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sETRACE1_HOOKED(statement, on_entry, on_exit) statement
#endif

#if sTRACE_LEVEL >= 2
#define sETRACE2_HOOKED(statement, on_entry, on_exit) sETRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sETRACE2_HOOKED(statement, on_entry, on_exit) statement
#endif

#if sTRACE_LEVEL >= 3
#define sETRACE3_HOOKED(statement, on_entry, on_exit) sETRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sETRACE3_HOOKED(statement, on_entry, on_exit) statement
#endif

#if sTRACE_LEVEL >= 4
#define sETRACE4_HOOKED(statement, on_entry, on_exit) sETRACE_HOOKED(statement, on_entry, on_exit)
#else
#define sETRACE4_HOOKED(statement, on_entry, on_exit) statement
#endif

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

sETRACE0-sETRACE4 are available to complement sTRACE0-sTRACE4.  These are
still affected by the sTRACE_LEVEL define.  
*/
#define sETRACE(statement) sETRACE_HOOKED(statement, (0), (0))
#define sETRACE0(statement) sETRACE0_HOOKED(statement, (0), (0))
#define sETRACE1(statement) sETRACE1_HOOKED(statement, (0), (0))
#define sETRACE2(statement) sETRACE2_HOOKED(statement, (0), (0))
#define sETRACE3(statement) sETRACE3_HOOKED(statement, (0), (0))
#define sETRACE4(statement) sETRACE4_HOOKED(statement, (0), (0))

#endif
