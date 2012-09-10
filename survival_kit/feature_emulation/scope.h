/**
This module defines scope guards that allow the cleanup of resources to be
placed next to the acquisition of resources without necessarily executing the
two at the same time.  This greatly simplifies resource management in a number
of cases.
It is inspired by D's scope guards: http://dlang.org/statement.html#ScopeGuardStatement
Like D's scope guards, these scope guards may not exit with a throw, 
goto, break, continue, or return; nor may they be entered with a goto. 
*/

#include <setjmp.h>
#include <unistd.h>

#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"

/* These define messages that show at compile-time if the caller does something wrong. */
/* They are wrapped in macros to make it easier to alter the text. */
#define SKIT_SCOPE_EXIT_HAS_USE_TXT \
	The_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_SCOPE_EXIT_statement

#define SKIT_SCOPE_SUCCESS_HAS_USE_TXT \
	The_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_SCOPE_SUCCESS_statement

#define SKIT_SCOPE_FAILURE_HAS_USE_TXT \
	The_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_SCOPE_FAILURE_statement

#define SKIT_SCOPE_GUARD_IS_IN_A_SCOPE_TXT \
	Scope_guards_must_be_placed_between_SCOPE_and_END_SCOPE_statements

#define SKIT_END_SCOPE_WITHOUT_SCOPE_TXT \
	END_SCOPE_found_without_preceding_SCOPE

#define SKIT_NO_NESTING_SCOPES_TXT \
	SCOPE_statement_blocks_may_not_be_nested_inside_each_other
/* End of error message definitions. */


#define SCOPE_EXIT(expr) \
	do { \
		SCOPE_EXIT_BEGIN \
			(expr); \
		END_SCOPE_EXIT \
	} while (0)

#define SCOPE_EXIT_BEGIN \
	if(SKIT_SCOPE_GUARD_IS_IN_A_SCOPE_TXT && \
	   The_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_SCOPE_EXIT_statement ){ \
		if ( !__skit_scope_guards_used ) \
		{ \
			/* Placing this fstack_alloc has been tricky. */ \
			/* It can't go in the SCOPE statement because skit_thread_ctx isn't defined at that point. */ \
			/* It can't go in the USE_FEATURE_EMULATION statement because it will unbalance */ \
			/*   the scope_jmp_stack if the caller never uses scope guards and never uses the */ \
			/* special RETURN statement. */ \
			/* We can't execute it EVERY time a scope guard begins. */ \
			/* What we CAN do is run the allocation just before the very first scope guard */ \
			/*   is encountered. */ \
			/* The existence of a scope guard will force SCOPE/END_SCOPE to be used and thus */ \
			/*   also force usage of RETURN or THROW statements that will properly unwind */ \
			/*   the scope_jmp_stack. */ \
			__skit_scope_fn_exit = skit_jmp_fstack_alloc( &skit_thread_ctx->scope_jmp_stack, &skit_malloc ); \
			__skit_scope_guards_used = 1; \
		} \
		if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->scope_jmp_stack, &skit_malloc)) != 0 ) \
		{ \
			/* We set another jump point to catch any exceptions that might be \
			thrown while in the scope guard.  This is important because exiting \
			the scope guard with a thrown exception is forbidden. */ \
			if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack, &skit_malloc)) == 0 ) \
			{

#define END_SCOPE_EXIT \
			} \
			else \
			{ \
				/* An exception was thrown! */ \
				/* It is safe to write to stderr here because we are crashing no matter what. */ \
				fprintf(stderr, "An exception was thrown in a SCOPE_EXIT block."); \
				fprintf(stderr, "SCOPE_EXIT may not exit due to thrown exceptions."); \
				fprintf(stderr, "The exception thrown is as follows:\n"); \
				skit_print_exception(&skit_thread_ctx->exc_instance_stack.used.front->val); \
				SKIT_ASSERT(0); \
			} \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->scope_jmp_stack),1); \
		} \
	}

#define SCOPE \
{ \
	SKIT_DECLARE_CHECK(SKIT_NO_NESTING_SCOPES_TXT,1); \
	/* SKIT_DECLARE_CHECK(Use_NSCOPE_and_NSCOPE_END_to_nest_SCOPE_statements,1); */ \
	SKIT_DECLARE_CHECK(SKIT_SCOPE_GUARD_IS_IN_A_SCOPE_TXT, 1); \
	SKIT_DECLARE_CHECK(SKIT_END_SCOPE_WITHOUT_SCOPE_TXT, 0); \
	/* Use (void) to silence misleading "warning: unused variable" messages from the compiler. */ \
	char __skit_scope_guards_used = 0; \
	(void)__skit_scope_guards_used;

#if 0
/* 
Add an extra layer of macro indirection to expand any macro definitions the
caller might pass into the macro.
Ex: 
#define FOO 3
NSCOPE(FOO)
should create __skit_scope_exit_3
instead of __skit_scope_exit_FOO
*/
#define NSCOPE_EXPAND(__skit_unique_nscope_id) \
	END_SCOPE_##__skit_unique_nscope_id##_found_without_preceding_SCOPE

#define NSCOPE(__skit_unique_nscope_id) \
	NSCOPE_EXPAND(__skit_unique_nscope_id)

TODO: How do I make RETURN macros that acknowledge nested scopes?  They would have to know to use __skit_scope_exit_3 instead of __skit_scope_fn_exit somehow.
#endif

#define USE_SCOPE_EMULATION \
	/* This first context will be used as a return address when the exit point */ \
	/*   scans all of the scope guards. */ \
	/* Use (void) to silence misleading "warning: unused variable" messages from the compiler. */ \
	jmp_buf *__skit_scope_fn_exit; \
	(void)__skit_scope_fn_exit; \
	SKIT_DECLARE_CHECK(SKIT_SCOPE_EXIT_HAS_USE_TXT, 1); \
	SKIT_DECLARE_CHECK(SKIT_SCOPE_SUCCESS_HAS_USE_TXT, 1); \
	SKIT_DECLARE_CHECK(SKIT_SCOPE_FAILURE_HAS_USE_TXT, 1);

#define __SKIT_SCAN_SCOPE_GUARDS \
		do { \
			if ( __skit_scope_guards_used ) \
			{ \
				/* This setjmp tells the scan how to return to this point. */ \
				if ( setjmp(*__skit_scope_fn_exit) == 0 )\
				{ \
					/* Now scan all scope guards, executing their contents. */ \
					/* Because of the above setjmp, the last scope guard's longjmp */ \
					/*   will automatically return execution just past this point. */ \
					longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->scope_jmp_stack),1); \
				} \
			} \
			else \
			{ \
				skit_jmp_fstack_pop(&skit_thread_ctx->scope_jmp_stack); \
			} \
		} while(0)

#define END_SCOPE \
	/* Check to make sure there is a corresponding SCOPE statement. */ \
	if ( !SKIT_END_SCOPE_WITHOUT_SCOPE_TXT ) \
	{ \
		__SKIT_SCAN_SCOPE_GUARDS; \
	} \
}
