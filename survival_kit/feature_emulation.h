#ifndef SKIT_FEATURE_EMULATION_INCLUDED
#define SKIT_FEATURE_EMULATION_INCLUDED

#include <stdlib.h>
#include <unistd.h> /* For ssize_t */
#include <inttypes.h>
#include <limits.h> /* For INT_MIN and the like. */
#include <setjmp.h>
#include <assert.h>

#include "survival_kit/misc.h"
#include "survival_kit/assert.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/generated_exception_defs.h"
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/feature_emulation/unittest.h"

/* TODO: what if an exception is raised from within a CATCH block? */

/** Exception definitions.  TODO: these should have a definition syntax that allows a separate tool to sweep all source files and automatically allocate non-colliding error codes for all exceptions and place them in a file that can be included from ERR_UTIL.H. */
/* GENERIC exceptions are the root of all catchable exceptions. */
/* @define_exception(GENERIC_EXCEPTION, "An exception was thrown.") */
/* @define_exception(BREAK_IN_TRY_CATCH : GENERIC_EXCEPTION,
"Code has attempted to use a 'break' statement from within a TRY-CATCH block.\n"
"This could easily corrupt program execution and corrupt debugging data.\n"
"Do not do this, ever!") */
/* @define_exception(CONTINUE_IN_TRY_CATCH : GENERIC_EXCEPTION,
"Code has attempted to use a 'continue' statement from within a TRY-CATCH block.\n"
"This could easily corrupt program execution and corrupt debugging data.\n"
"Do not do this, ever!" ) */

/*
--------------------------------------------------------------------------------
SCOPE guard hygeine:

--------------------------------------------------------------------------------
This version of resource cleanup:

skit_stream *resource1 = skit_new_stream("foo.txt","w");
SCOPE_EXIT(skit_stream_free(resource1));
skit_stream *resource2 = skit_new_stream("bar.txt","w");
SCOPE_EXIT(skit_stream_free(resource2));

is preferred over this version:

skit_stream *resource1 = skit_new_stream("foo.txt","w");
skit_stream *resource2 = skit_new_stream("bar.txt","w");
SCOPE_EXIT(skit_stream_free(resource1));
SCOPE_EXIT(skit_stream_free(resource2));

The reason is that if resource2's initialization throws an exception, then the
former example will cleanup resource1 but the latter will not.

--------------------------------------------------------------------------------
Since scope guards are implemented with setjmp/longjmp, it is possible to write
constructs like this:

  mytype *val = (mytype*)malloc(sizeof(mytype);
  if ( val != NULL )
  {
      SCOPE_EXIT
          free(val));
          val = NULL;
      END_SCOPE_EXIT
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
  SCOPE
      SCOPE_EXIT
          free(val));
          val = NULL;
      END_SCOPE_EXIT
  END_SCOPE
  ... code that segfaults when attempting to use val ...

It would be a shame if this version were confused with the one where {} brackets
were used instead of SCOPE/END_SCOPE!

--------------------------------------------------------------------------------



*/

/** Place this at the top of function bodies that use language feature emulation. */
#define USE_FEATURE_EMULATION \
	do { \
		SKIT_ASSERT(init_was_called); \
	}while(0); \
	char Place_the_USE_FEATURES_macro_at_the_top_of_function_bodies_to_use_features_like_TRY_CATCH_and_SCOPE; \
	char *goto_statements_are_not_allowed_in_SCOPE_EXIT_blocks; \
	char *goto_statements_are_not_allowed_in_SCOPE_SUCCESS_blocks; \
	char *goto_statements_are_not_allowed_in_SCOPE_FAILURE_blocks; \
	skit_thread_context *skit_thread_ctx = skit_thread_context_get(); \
	SKIT_ASSERT_MSG(skit_thread_ctx != NULL, "This can happen if the thread's skit_thread_init() function was not called."); \
	(void)skit_thread_ctx; \
	(void)Place_the_USE_FEATURES_macro_at_the_top_of_function_bodies_to_use_features_like_TRY_CATCH_and_SCOPE; \
	(void)goto_statements_are_not_allowed_in_SCOPE_EXIT_blocks; \
	(void)goto_statements_are_not_allowed_in_SCOPE_SUCCESS_blocks; \
	(void)goto_statements_are_not_allowed_in_SCOPE_FAILURE_blocks; \
	do {} while(0)

/* TODO: redefine 'break', 'continue', 'goto', and 'return' so that they always
   compile fail when used in places where they shouldn't appear. */

/* 
break:
if ( skit_thread_ctx->try_jmp_stack.used.length > 0 )
{
	fprintf(stderr,"%s, %d: in function %s: Used 'break' statement while in TRY-CATCH block.", 
		__FILE__, __LINE__, __func__);
	skit_jmp_buf_fstack_pop(&skit_thread_ctx->try_jmp_stack);
}

continue:
(ditto)

goto:
if (0)
{
	(void) *goto_statements_are_not_allowed_in_SCOPE_EXIT_blocks;
	(void) *goto_statements_are_not_allowed_in_SCOPE_SUCCESS_blocks;
	(void) *goto_statements_are_not_allowed_in_SCOPE_FAILURE_blocks;
}

if ( skit_thread_ctx->try_jmp_stack.used.length > 0 )
{
	RAISE(...);
}


SCOPE_EXIT:
	char goto_statements_are_not_allowed_in_SCOPE_EXIT_blocks; \
	char goto_statements_are_not_allowed_in_SCOPE_SUCCESS_blocks; \
	char goto_statements_are_not_allowed_in_SCOPE_FAILURE_blocks; \
*/

/*
FATAL exceptions should never be caught.
They signal potentially irreversable corruption, such as memory corruption
that may cause things like unexpected null pointers or access violations.
*/
/* @define_exception(FATAL_EXCEPTION, "A fatal exception was thrown.") */

/* TODO: these should be stored in thread-local storage. */
#if 0
#define FRAME_INFO_STACK_SIZE 1024
extern frame_info __frame_info_stack[FRAME_INFO_STACK_SIZE];
extern jmp_buf    __frame_context_stack[FRAME_INFO_STACK_SIZE];
extern ssize_t    __frame_info_end;

#define TRY_CONTEXT_STACK_SIZE 256 /* One would hope this is enough. */
extern jmp_buf    __try_context_stack[TRY_CONTEXT_STACK_SIZE];
extern ssize_t    __try_context_end;

/* Implementation detail. */
extern exception *__thrown_exception;

/** Allocates a new exception on the exception stack. */
skit_exception *skit_new_exception(skit_thread_context *ctx, err_code_t error_code, char *mess, ...);


/** Allocates a new exception. */
exception *skit_new_exception(err_code_t error_code, char *mess, ...);

/** Call this to deallocate the memory used by an exception. */
/** This will be called automatically in TRY_CATCH(expr) ... ENDTRY blocks. */
exception *free_exception(exception *e);

/* Macro implementation details.  Do not use directly. */
char *__stack_trace_to_str_expr( uint32_t line, const char *file, const char *func );
/*jmp_buf *__push_stack_info(size_t line, const char *file, const char *func);
frame_info __pop_stack_info();

jmp_buf *__push_try_context();
jmp_buf *__pop_try_context();
*/
#endif

#if 0
#define __PROPOGATE_THROWN_EXCEPTIONS /* */ \
	do { \
		SKIT_FEATURE_TRACE("%s, %d.117: __PROPOGATE\n", __FILE__, __LINE__); \
		SKIT_FEATURE_TRACE("frame_info_index: %li\n",__frame_info_end-1); \
		if ( __frame_info_end-1 >= 0 ) \
			longjmp( \
				__frame_context_stack[__frame_info_end-1], \
				__thrown_exception->error_code); \
		else \
		{ \
			skit_print_exception(__thrown_exception); /* TODO: this is probably a dynamic allocation and should be replaced by fprint_exception or something. */ \
			skit_die("Exception thrown with no handlers left in the stack."); \
		} \
	} while (0)
#endif

#if 0
#define CALL(expr) /* */ \
	do { \
		if ( setjmp(*__push_stack_info(__LINE__,__FILE__,__func__)) == 0 ) { \
			SKIT_FEATURE_TRACE("%s, %d.182: CALL.setjmp\n", __FILE__, __LINE__); \
			(expr); \
		} else { \
			SKIT_FEATURE_TRACE("%s, %d.186: CALL.longjmp\n", __FILE__, __LINE__); \
			__pop_stack_info(); \
			__PROPOGATE_THROWN_EXCEPTIONS; \
		} \
		SKIT_FEATURE_TRACE("%s, %d.190: CALL.success\n", __FILE__, __LINE__); \
		__pop_stack_info(); \
	} while (0)
#endif

/** Evaluates the given expression while creating a stack entry at this point
// in the code.  The whole purpose of doing this is to provide better debug
// info in stack traces.  It is tolerable to forget to use this when calling
// into other functions because the exception handling mechanism will still
// be able to return to the nearest enclosing TRY-CATCH.  Wrapping function
// calls in this macro is desirable though, because the calling file, line,
// and function name will be absent from stack traces if this is not used.
*/
#define CALL(expr) /* */ \
	do { \
		/* Save a copies of the current stack positions. */ \
		skit_thread_context_pos __skit_thread_ctx_pos;\
		skit_save_thread_context_pos(skit_thread_ctx, &__skit_thread_ctx_pos); \
		/* This ensures that we can end up where we started in this call, */ \
		/*   even if the callee messes up the jmp_stack. */ \
		/* The jmp_stack could be messed up if someone tries to jump */ \
		/*   execution out of a TRY-CATCH or scope guard block */ \
		/*   (using break/continue/goto/return) and somehow manages to succeed, */ \
		/*   thus preventing pop calls that are supposed to match push calls. */ \
		\
		skit_debug_info_store( \
			skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc), \
			__LINE__,__FILE__,__func__); \
		\
		if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack, &skit_malloc)) == 0 ) { \
			SKIT_FEATURE_TRACE("%s, %d.182: CALL.setjmp\n", __FILE__, __LINE__); \
			(expr); \
		} else { \
			SKIT_FEATURE_TRACE("%s, %d.186: CALL.longjmp\n", __FILE__, __LINE__); \
			skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack); \
			skit_reconcile_thread_context(skit_thread_ctx, &__skit_thread_ctx_pos); \
			__PROPOGATE_THROWN_EXCEPTIONS; \
		} \
		\
		skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack); \
		skit_reconcile_thread_context(skit_thread_ctx, &__skit_thread_ctx_pos); \
		\
		SKIT_FEATURE_TRACE("%s, %d.190: CALL.success\n", __FILE__, __LINE__); \
	} while (0)
	
/* __TRY_SAFE_EXIT is an implementation detail.
// It allows execution to exit a TRY-CATCH block by jumping to a state distinct
// from the dangerous ways of exiting a TRY-CATCH block.  The "safe" way is
// for execution to fall to the bottom of a given block in the TRY-CATCH
// statement.  The "dangerous" was are by using local jumping constructs like
// "break" and "continue" to leave a TRY-CATCH statement.  
*/
#define __TRY_SAFE_EXIT         INT_MIN+1  

/* __TRY_EXCEPTION_CLEANUP is an implementation detail.
// It is jumped to at the end of CATCH blocks as a way of cleaning up the
// exception allocated in the code that threw the exception.
*/
#define __TRY_EXCEPTION_CLEANUP INT_MIN

#if 0
/** NOTE: Do not attempt to branch out of a TRY-CATCH block.  Example:
//    int foo, len;
//    len = 10;
//    while ( foo < len )
//    {
//        TRY
//            foo++;
//            if ( foo > len )
//                return;  // The stack now permanently contains a reference
//                         //   to this TRY statement.  Any ENDTRY's
//                         //   encountered in calling code might actually
//                         //   end up at the TRY above.
//            else
//                continue; // ditto
//        ENDTRY
//    }
//
//    DO NOT DO THE ABOVE ^^.
//    The "return" and "continue" statements will leave the TRY/ENDTRY block
//      without restoring stack information to the expected values.  Certain
//      instructions such as "break" may also jump to places you wouldn't 
//      expect.
//    TOOD: It'd be nice if there was some way to make the compiler forbid
//      such local jumps or maybe even make them work in sensible ways.
*/
#define TRY /* */ \
	if ( setjmp(*__push_try_context()) != __TRY_SAFE_EXIT ) { \
		SKIT_FEATURE_TRACE("%s, %d.236: TRY.if\n", __FILE__, __LINE__); \
		do { \
			SKIT_FEATURE_TRACE("%s, %d.238: TRY.do\n", __FILE__, __LINE__); \
			switch( setjmp(*__push_stack_info(__LINE__,__FILE__,__func__)) ) \
			{ \
			case __TRY_EXCEPTION_CLEANUP: \
			{ \
				/* Exception cleanup case. */ \
				/* Note that this case is ALWAYS reached because CASE and ENDTRY macros  */ \
				/*   may follow either the successful block or an exceptional one, so    */ \
				/*   there is no way to know from within this macro which one should be  */ \
				/*   jumped to.  The best strategy then is to always jump to the cleanup */ \
				/*   case after leaving any part of the TRY-CATCH-ENDTRY and only free   */ \
				/*   exceptions if there actually are any.                               */ \
				SKIT_FEATURE_TRACE("%s, %d.258: TRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
				if (__thrown_exception != NULL) \
				{ \
					free(__thrown_exception); \
					__thrown_exception = NULL; \
				} \
				__pop_stack_info(); \
				longjmp(*__pop_try_context(), __TRY_SAFE_EXIT); \
			} \
			default: \
			{ \
				/* This is going to look a bit Duffy. ;)                           */ \
				/* It is fairly important, however, that we nest the 'case 0:'     */ \
				/* statement inside of an if statement that is otherwise never     */ \
				/* evaluated.  This strange configuration is driven by the need to */ \
				/* have the code at the end of each block (after macro expansion)  */ \
				/* look identical but do slightly different things.  The if(0)     */ \
				/* ensures that each end-of-block can assume that the previous     */ \
				/* block started as some form of if-statement.  The 0 in the       */ \
				/* if(0) makes sure that it never gets re-evaluated when           */ \
				/* exceptions are thrown: it must only be evaluated once when      */ \
				/* the TRY block is entered, and that causes setjmp to select      */ \
				/* the 0th case. Either way, the end-of-blocks will cleanup        */ \
				/* thrown exceptions /if they exist/ and otherwise exit cleanly.   */ \
				/* An early approach was to place everything inside a switch-case  */ \
				/* statement and never use if-else.  That configuration was        */ \
				/* incapable of handling exception inheritance because the cases   */ \
				/* in a switch-case can't be computed expressions.                 */ \
				if ( 0 ) \
				{ \
				case 0: \
					/* Normal/successful case. */ \
					SKIT_FEATURE_TRACE("%s, %d.282: TRY: case 0:\n", __FILE__, __LINE__); \

#define CATCH(__error_code, exc_name) /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.288: TRY: case 0: longjmp\n", __FILE__, __LINE__); \
					longjmp( __frame_context_stack[__frame_info_end-1], __TRY_EXCEPTION_CLEANUP); \
				} \
				else if ( exception_is_a( __thrown_exception->error_code, __error_code) ) \
				{ \
					/* CATCH block. */ \
					SKIT_FEATURE_TRACE("%s, %d.294: TRY: case %d:\n", __FILE__, __LINE__, __error_code); \
					exception *exc_name = __thrown_exception;

#define ENDTRY /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.301: TRY: case ??: longjmp\n", __FILE__, __LINE__); \
					longjmp( __frame_context_stack[__frame_info_end-1], __TRY_EXCEPTION_CLEANUP); \
				} \
				else \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					SKIT_FEATURE_TRACE("%s, %d.307: TRY: default: longjmp\n", __FILE__, __LINE__); \
					__pop_stack_info(); \
					__pop_try_context(); \
					__PROPOGATE_THROWN_EXCEPTIONS; \
				} \
				assert(0); /* This should never be reached. The if-else chain above should handle all remaining cases. */ \
			} /* default: { } */ \
			} /* switch(setjmp(*__push...)) */ \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			SKIT_FEATURE_TRACE("%s, %d.319: TRY: break found!\n", __FILE__, __LINE__); \
			__pop_stack_info(); \
			__pop_try_context(); \
			THROW(new_exception(BREAK_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'break' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" )); \
		} while (0); \
		/* If execution makes it here, then the caller */ \
		/* used a "continue" statement and is trying to */ \
		/* corrupt the debug stack.  Don't let them do it! */ \
		/* Instead, throw another exception. */ \
		SKIT_FEATURE_TRACE("%s, %d.331: TRY: continue found!\n", __FILE__, __LINE__); \
		__pop_stack_info(); \
		__pop_try_context(); \
		THROW(new_exception(CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" )); \
	} \
	SKIT_FEATURE_TRACE("%s, %d.339: TRY: done.\n", __FILE__, __LINE__);
#endif

/** NOTE: Do not attempt to branch out of a TRY-CATCH block.  Example:
//    int foo, len;
//    len = 10;
//    while ( foo < len )
//    {
//        TRY
//            foo++;
//            if ( foo > len )
//                return;  // The stack now permanently contains a reference
//                         //   to this TRY statement.  Any ENDTRY's
//                         //   encountered in calling code might actually
//                         //   end up at the TRY above.
//            else
//                continue; // ditto
//        ENDTRY
//    }
//
//    DO NOT DO THE ABOVE ^^.
//    The "return" and "continue" statements will leave the TRY/ENDTRY block
//      without restoring stack information to the expected values.  Certain
//      instructions such as "break" may also jump to places you wouldn't 
//      expect.
//    TOOD: It'd be nice if there was some way to make the compiler forbid
//      such local jumps or maybe even make them work in sensible ways.
*/
#define TRY /* */ \
	SKIT_FEATURE_TRACE("%s, %d.236: TRY.start\n", __FILE__, __LINE__); \
	if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->try_jmp_stack,&skit_malloc)) != __TRY_SAFE_EXIT ) { \
		SKIT_FEATURE_TRACE("%s, %d.236: TRY.if\n", __FILE__, __LINE__); \
		do { \
			SKIT_FEATURE_TRACE("%s, %d.238: TRY.do\n", __FILE__, __LINE__); \
			switch( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack,&skit_malloc)) ) \
			{ \
			case __TRY_EXCEPTION_CLEANUP: \
			{ \
				/* Exception cleanup case. */ \
				/* Note that this case is ALWAYS reached because CASE and ENDTRY macros  */ \
				/*   may follow either the successful block or an exceptional one, so    */ \
				/*   there is no way to know from within this macro which one should be  */ \
				/*   jumped to.  The best strategy then is to always jump to the cleanup */ \
				/*   case after leaving any part of the TRY-CATCH-ENDTRY and only free   */ \
				/*   exceptions if there actually are any.                               */ \
				SKIT_FEATURE_TRACE("%s, %d.258: TRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
				while (skit_thread_ctx->exc_instance_stack.used.length > 0) \
				{ \
					skit_exc_fstack_pop(&skit_thread_ctx->exc_instance_stack); \
				} \
				skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
				longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack), __TRY_SAFE_EXIT); \
			} \
			default: \
			{ \
				/* This is going to look a bit Duffy. ;)                           */ \
				/* It is fairly important, however, that we nest the 'case 0:'     */ \
				/* statement inside of an if statement that is otherwise never     */ \
				/* evaluated.  This strange configuration is driven by the need to */ \
				/* have the code at the end of each block (after macro expansion)  */ \
				/* look identical but do slightly different things.  The if(0)     */ \
				/* ensures that each end-of-block can assume that the previous     */ \
				/* block started as some form of if-statement.  The 0 in the       */ \
				/* if(0) makes sure that it never gets re-evaluated when           */ \
				/* exceptions are thrown: it must only be evaluated once when      */ \
				/* the TRY block is entered, and that causes setjmp to select      */ \
				/* the 0th case. Either way, the end-of-blocks will cleanup        */ \
				/* thrown exceptions /if they exist/ and otherwise exit cleanly.   */ \
				/* An early approach was to place everything inside a switch-case  */ \
				/* statement and never use if-else.  That configuration was        */ \
				/* incapable of handling exception inheritance because the cases   */ \
				/* in a switch-case can't be computed expressions.                 */ \
				if ( 0 ) \
				{ \
				case 0: \
					/* Normal/successful case. */ \
					SKIT_FEATURE_TRACE("%s, %d.282: TRY: case 0:\n", __FILE__, __LINE__); \

#define CATCH(__error_code, exc_name) /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.288: TRY: case 0: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, __TRY_EXCEPTION_CLEANUP); \
				} \
				else if ( exception_is_a( skit_thread_ctx->exc_instance_stack.used.front->val.error_code, __error_code) ) \
				{ \
					/* CATCH block. */ \
					SKIT_FEATURE_TRACE("%s, %d.294: TRY: case %d:\n", __FILE__, __LINE__, __error_code); \
					skit_exception *exc_name = &skit_thread_ctx->exc_instance_stack.used.front->val;

#define ENDTRY /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.301: TRY: case ??: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, __TRY_EXCEPTION_CLEANUP); \
				} \
				else \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					SKIT_FEATURE_TRACE("%s, %d.307: TRY: default: longjmp\n", __FILE__, __LINE__); \
					skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
					skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
					__PROPOGATE_THROWN_EXCEPTIONS; \
				} \
				assert(0); /* This should never be reached. The if-else chain above should handle all remaining cases. */ \
			} /* default: { } */ \
			} /* switch(setjmp(*__push...)) */ \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			SKIT_FEATURE_TRACE("%s, %d.319: TRY: break found!\n", __FILE__, __LINE__); \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
			THROW(BREAK_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'break' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" ); \
		} while (0); \
		/* If execution makes it here, then the caller */ \
		/* used a "continue" statement and is trying to */ \
		/* corrupt the debug stack.  Don't let them do it! */ \
		/* Instead, throw another exception. */ \
		SKIT_FEATURE_TRACE("%s, %d.331: TRY: continue found!\n", __FILE__, __LINE__); \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
		skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
		THROW(CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n"); \
	} \
	SKIT_FEATURE_TRACE("%s, %d.339: TRY: done.\n", __FILE__, __LINE__);

#endif