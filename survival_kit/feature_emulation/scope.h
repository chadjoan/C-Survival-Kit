/**
This module defines scope guards that allow the cleanup of resources to be
placed next to the acquisition of resources without necessarily executing the
two at the same time.  This greatly simplifies resource management in a number
of cases.

It is inspired by D's scope guards: http://dlang.org/statement.html#ScopeGuardStatement
Like D's scope guards, these scope guards may not exit with a throw, 
goto, break, continue, or return; nor may they be entered with a goto. 

================================================================================
sSCOPE guard hygeine:

--------------------------------------------------------------------------------
Do not exit scope guards with throw, return, break, continue, or goto 
statements.

--------------------------------------------------------------------------------
This version of resource cleanup:

skit_stream *resource1 = skit_new_stream("foo.txt","w");
sSCOPE_EXIT(skit_stream_free(resource1));
skit_stream *resource2 = skit_new_stream("bar.txt","w");
sSCOPE_EXIT(skit_stream_free(resource2));

is preferred over this version:

skit_stream *resource1 = skit_new_stream("foo.txt","w");
skit_stream *resource2 = skit_new_stream("bar.txt","w");
sSCOPE_EXIT(skit_stream_free(resource1));
sSCOPE_EXIT(skit_stream_free(resource2));

The reason is that if resource2's initialization throws an exception, then the
former example will cleanup resource1 but the latter will not.

--------------------------------------------------------------------------------
Since scope guards are implemented with setjmp/longjmp, it is possible to write
constructs like this:

  mytype *val = (mytype*)malloc(sizeof(mytype);
  if ( val != NULL )
  {
      sSCOPE_EXIT
          free(val));
          val = NULL;
      sEND_SCOPE_EXIT
  }
  ... code that uses val ...

This is highly discouraged.  Consider what the similar-looking D code would do:

  mytype* val = cast(mytype*)malloc(mytype.sizeof);
  if ( val != null )
  {
      scope(exit)
      {
          free(val);
          val = null;
      }
  }
  ... code that segfaults when attempting to use val ...

The scope(exit)'s enclosing scope is the if-statement.  When the if statement
exits, the scope(exit) statement is immediately executed.  Anything that happens
after the if-statement will see a null value for the 'val' variable.  

The C code will not do the same thing, because the C code's scope guard is only
defined when it is /executed/ and not when it is /compiled/.  This is actually
strange behavior and may cause bugs if the code is ever ported to other
languages or frameworks.  The recommended way to accomplish this is to throw
an exception if the malloc call returns NULL.  This can be made convenient
by wrapping the allocation in a function that handles the error detection and
exception throwing.

The above D code is actually closer in meaning to this C code:

  mytype *val = (mytype*)malloc(sizeof(mytype);
  if ( val != NULL )
  sSCOPE
      sSCOPE_EXIT
          free(val));
          val = NULL;
      sEND_SCOPE_EXIT
  sEND_SCOPE
  ... code that segfaults when attempting to use val ...

It would be a shame if this version were confused with the one where {} brackets
were used instead of sSCOPE/sEND_SCOPE!

--------------------------------------------------------------------------------
*/

#include <setjmp.h>
#include <unistd.h>

#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"

#define SKIT_SCOPE_NOT_EXITING   0x00
#define SKIT_SCOPE_SUCCESS_EXIT  0x01
#define SKIT_SCOPE_FAILURE_EXIT  0x02
#define SKIT_SCOPE_ANY_EXIT      0x03 /* success | exit */

/* These define messages that show at compile-time if the caller does something wrong. */
/* They are wrapped in macros to make it easier to alter the text. */
#define SKIT_SCOPE_EXIT_HAS_USE_TXT \
	The_SKIT_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_sSCOPE_EXIT_statement

#define SKIT_SCOPE_SUCCESS_HAS_USE_TXT \
	The_SKIT_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_sSCOPE_SUCCESS_statement

#define SKIT_SCOPE_FAILURE_HAS_USE_TXT \
	The_SKIT_USE_FEATURE_EMULATION_statement_must_be_in_the_same_scope_as_a_sSCOPE_FAILURE_statement

#define SKIT_SCOPE_GUARD_IS_IN_A_SCOPE_TXT \
	Scope_guards_must_be_placed_between_sSCOPE_and_sEND_SCOPE_statements

#define SKIT_END_SCOPE_WITHOUT_SCOPE_TXT \
	sEND_SCOPE_found_without_preceding_sSCOPE

#define SKIT_NO_NESTING_SCOPES_TXT \
	sSCOPE_statement_blocks_may_not_be_nested_inside_each_other
/* End of error message definitions. */



#define sSCOPE_GUARD_BEGIN(macro_arg_exit_status) \
	if(SKIT_SCOPE_GUARD_IS_IN_A_SCOPE_TXT && SKIT_SCOPE_EXIT_HAS_USE_TXT ){ \
		if ( !skit_scope_ctx->scope_guards_used ) \
		{ \
			/* Placing this fstack_alloc has been tricky. */ \
			/* It can't go in the sSCOPE statement because skit_thread_ctx isn't defined at that point. */ \
			/* It can't go in the SKIT_USE_FEATURE_EMULATION statement because it will unbalance */ \
			/*   the scope_jmp_stack if the caller never uses scope guards and never uses the */ \
			/* special sRETURN statement. */ \
			/* We can't execute it EVERY time a scope guard begins. */ \
			/* What we CAN do is run the allocation just before the very first scope guard */ \
			/*   is encountered. */ \
			/* The existence of a scope guard will force sSCOPE/sEND_SCOPE to be used and thus */ \
			/*   also force usage of sRETURN or sTHROW statements that will properly unwind */ \
			/*   the scope_jmp_stack. */ \
			skit_scope_ctx->scope_fn_exit = skit_jmp_fstack_alloc( &skit_thread_ctx->scope_jmp_stack, &skit_malloc ); \
			skit_scope_ctx->scope_guards_used = 1; \
			\
			/* We set a  jump point to catch any exceptions that might be */ \
			/* thrown from a function that wasn't called with the sCALL macro. */ \
			/* This is important because we'll need to ensure that the scope */ \
			/*   guards get scanned during abnormal exit. */ \
			int skit_jmp_code = setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack, &skit_malloc)); \
			if ( skit_jmp_code != 0 ) \
			{ \
				__SKIT_SCAN_SCOPE_GUARDS(SKIT_SCOPE_FAILURE_EXIT); \
				longjmp(skit_thread_ctx->exc_jmp_stack.used.front->val, skit_jmp_code); \
			} \
		} \
		if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->scope_jmp_stack, &skit_malloc)) != 0 ) \
		{ \
			if ( (skit_scope_ctx->exit_status) & (macro_arg_exit_status) ) \
			{ \
				SKIT_COMPILE_TIME_CHECK(SKIT_NO_GOTO_FROM_SCOPE_GUARDS_PTR,0); \
				SKIT_COMPILE_TIME_CHECK(SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_PTR,0); \
				SKIT_COMPILE_TIME_CHECK(SKIT_NO_BREAK_FROM_SCOPE_GUARDS_PTR,0); \
				\
				/* We set another jump point to catch any exceptions that might be \
				thrown while in the scope guard.  This is important because exiting \
				the scope guard with a thrown exception is forbidden. */ \
				if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack, &skit_malloc)) == 0 ) \
				{

#define sEND_SCOPE_GUARD \
				} \
				else \
				{ \
					/* An exception was thrown! */ \
					/* It is safe to write to stderr here because we are crashing no matter what. */ \
					fprintf(stderr, "An exception was thrown in a sSCOPE_EXIT block."); \
					fprintf(stderr, "sSCOPE_EXIT may not exit due to thrown exceptions."); \
					fprintf(stderr, "The exception thrown is as follows:\n"); \
					skit_print_exception(&skit_thread_ctx->exc_instance_stack.used.front->val); \
					sASSERT(0); \
				} \
				skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			} \
			longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->scope_jmp_stack),1); \
		} \
	}


#define sSCOPE_EXIT_BEGIN    sSCOPE_GUARD_BEGIN(SKIT_SCOPE_ANY_EXIT)
#define sSCOPE_SUCCESS_BEGIN sSCOPE_GUARD_BEGIN(SKIT_SCOPE_SUCCESS_EXIT)
#define sSCOPE_FAILURE_BEGIN sSCOPE_GUARD_BEGIN(SKIT_SCOPE_FAILURE_EXIT)

#define sEND_SCOPE_EXIT    sEND_SCOPE_GUARD
#define sEND_SCOPE_SUCCESS sEND_SCOPE_GUARD
#define sEND_SCOPE_FAILURE sEND_SCOPE_GUARD

#define sSCOPE_EXIT(expr) \
	do { \
		sSCOPE_EXIT_BEGIN \
			(expr); \
		sEND_SCOPE_EXIT \
	} while (0)

#define sSCOPE_FAILURE(expr) \
	do { \
		sSCOPE_FAILURE_BEGIN \
			(expr); \
		sEND_SCOPE_FAILURE \
	} while (0)

#define sSCOPE_SUCCESS(expr) \
	do { \
		sSCOPE_SUCCESS_BEGIN \
			(expr); \
		sEND_SCOPE_SUCCESS \
	} while (0)

#define sSCOPE \
{ \
	SKIT_COMPILE_TIME_CHECK(SKIT_NO_NESTING_SCOPES_TXT,1); \
	/* SKIT_COMPILE_TIME_CHECK(Use_NSCOPE_and_NSCOPE_END_to_nest_SCOPE_statements,1); */ \
	SKIT_COMPILE_TIME_CHECK(SKIT_SCOPE_GUARD_IS_IN_A_SCOPE_TXT, 1); \
	SKIT_COMPILE_TIME_CHECK(SKIT_END_SCOPE_WITHOUT_SCOPE_TXT, 0); \
	SKIT_COMPILE_TIME_CHECK(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR, 0);

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

TODO: How do I make sRETURN macros that acknowledge nested scopes?  They would have to know to use __skit_scope_exit_3 instead of __skit_scope_fn_exit somehow.
#endif

#define SKIT_USE_SCOPE_EMULATION \
	/* This first context will be used as a return address when the exit point */ \
	/*   scans all of the scope guards. */ \
	/* Create a scope context and pointerize it so that macros as well as */ \
	/*   other functions can use -> notation uniformly. */ \
	skit_scope_context __skit_scope_ctx; \
	skit_scope_context *skit_scope_ctx = &__skit_scope_ctx; \
	/* Use (void) to silence misleading "warning: unused variable" messages from the compiler. */ \
	(void) __skit_scope_ctx; \
	(void) skit_scope_ctx; \
	__skit_scope_ctx.scope_fn_exit = NULL; \
	__skit_scope_ctx.scope_guards_used = 0; \
	__skit_scope_ctx.exit_status = SKIT_SCOPE_NOT_EXITING; \
	SKIT_COMPILE_TIME_CHECK(SKIT_SCOPE_EXIT_HAS_USE_TXT, 1); \
	SKIT_COMPILE_TIME_CHECK(SKIT_SCOPE_SUCCESS_HAS_USE_TXT, 1); \
	SKIT_COMPILE_TIME_CHECK(SKIT_SCOPE_FAILURE_HAS_USE_TXT, 1);

/*
WARNING: Do not expand this macro in any place besides the function with
  the scope guards that will be scanned by this.
Rationale: The __SKIT_SCAN_SCOPE_GUARDS macro expands to a statement that 
includes a setjmp command that remembers the place where the 
__SKIT_SCAN_SCOPE_GUARDS was.  It will then instruct the last scope guard 
to longjmp to that macro later.  The implication is that if the 
__SKIT_SCAN_SCOPE_GUARDS macro appears in a different function stack
frame than the one that contains the scope guards, then it will longjmp
back to a potentially obliterated stack frame.  
Example call sequence:
foo()      : Define some scopes.
foo->bar() : __SKIT_SCAN_SCOPE_GUARDS (setjmp in bar(), then jump back to foo())
foo()      : sSCOPE_EXIT : call baz()
foo->baz() : Do something.  Anything.  bar()'s stack frame is now trashed.
foo()      : baz() returned normally.
foo()      : sSCOPE_EXIT finishes, longjmp back to __SKIT_SCAN_SCOPE_GUARDS in bar()
foo->bar() : Access baz()'s data... CRASH!
The solution is that bar() should have never invoked __SKIT_SCAN_SCOPE_GUARDS.
foo() should have instead called __SKIT_SCAN_SCOPE_GUARDS right after bar() 
returned.
*/
#define __SKIT_SCAN_SCOPE_GUARDS(macro_arg_exit_status) \
		do { \
			if ( __skit_try_catch_nesting_level == 0 && skit_scope_ctx->scope_guards_used ) \
			{ \
				skit_scope_ctx->exit_status = macro_arg_exit_status; \
				\
				/* This setjmp tells the scan how to return to this point. */ \
				if ( setjmp(*skit_scope_ctx->scope_fn_exit) == 0 )\
				{ \
					/* Now scan all scope guards, executing their contents. */ \
					/* Because of the above setjmp, the last scope guard's longjmp */ \
					/*   will automatically return execution just past this point. */ \
					longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->scope_jmp_stack),1); \
				} \
				/* Pop the extra jmp frame that was allocated to catch any unplanned exits. */ \
				/* The code that allocated the frame will handle further longjmp'ing. */ \
				skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			} \
		} while(0)

#define sEND_SCOPE \
	/* Check to make sure there is a corresponding sSCOPE statement. */ \
	if ( !SKIT_END_SCOPE_WITHOUT_SCOPE_TXT ) \
	{ \
		__SKIT_SCAN_SCOPE_GUARDS(SKIT_SCOPE_SUCCESS_EXIT); \
	} \
	SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
	(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
}
