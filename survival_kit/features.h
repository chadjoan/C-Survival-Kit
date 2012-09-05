#ifndef ERR_UTIL_INCLUDED
#define ERR_UTIL_INCLUDED

#include <stdlib.h>
#include <unistd.h> /* For ssize_t */
#include <inttypes.h>
#include <limits.h> /* For INT_MIN and the like. */
#include <setjmp.h>
#include <assert.h>

#include "survival_kit/macro.h"
#include "survival_kit/generated_exception_defs.h"

/* TODO: what if an exception is raised from within a CATCH block? */

/* Debugging twiddly knobs.  Useful if you have the misfortune of needing to
// debug the code that is supposed to make debugging easier. */
#define DO_ERR_UTIL_TRACING 0
#if DO_ERR_UTIL_TRACING == 1
	#define ERR_UTIL_TRACE(...) printf(__VA_ARGS__)
#else
	#define ERR_UTIL_TRACE(...)
#endif

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

/** Unit test. */
void skit_unittest_features();

/* */
#define USE_FEATURES \
	char Place_the_USE_FEATURES_macro_at_the_top_of_function_bodies_to_use_features_like_TRY_CATCH_and_SCOPE; \
	char *goto_statements_are_not_allowed_in_SCOPE_EXIT_blocks; \
	char *goto_statements_are_not_allowed_in_SCOPE_SUCCESS_blocks; \
	char *goto_statements_are_not_allowed_in_SCOPE_FAILURE_blocks; \
	skit_func_context skit_func_ctx; \
	skit_func_context_init(&skit_func_ctx); \
	skit_thread_context skit_thread_ctx; \
	skit_thread_context_get(&skit_thread_ctx); \
	do {} while(0)

/* TODO: redefine 'break', 'continue', 'goto', and 'return' so that they always
   compile fail when used in places where they shouldn't appear. */

/* 
break:
if ( skit_func_ctx.try_stack.used.length > 0 )
{
	fprintf(stderr,"%s, %d: in function %s: Used 'break' statement while in TRY-CATCH block.", 
		__FILE__, __LINE__, __func__);
	skit_jmp_buf_flist_pop(&skit_func_ctx.try_stack);
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

if ( skit_func_ctx.try_stack.used.length > 0 )
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

/* Implementation details. */
typedef struct frame_info frame_info;
struct frame_info
{
	uint32_t line_number;
	const char *file_name;
	const char *func_name;
	jmp_buf *jmp_context;
};

/* TODO: these should be stored in thread-local storage. */
#define FRAME_INFO_STACK_SIZE 1024
extern frame_info __frame_info_stack[FRAME_INFO_STACK_SIZE];
extern jmp_buf    __frame_context_stack[FRAME_INFO_STACK_SIZE];
extern ssize_t    __frame_info_end;

#define TRY_CONTEXT_STACK_SIZE 256 /* One would hope this is enough. */
extern jmp_buf    __try_context_stack[TRY_CONTEXT_STACK_SIZE];
extern ssize_t    __try_context_end;

/** */
typedef struct exception exception;
struct exception
{
	/* Implementation note: keep this small, it will be returned by-value from functions a lot. */
	err_code_t error_code;  /** 0 should always mean "no error". TODO: Values of 0 don't make sense anymore.  It was useful for an inferior exceptions implementation.  Followup needed? */
	char *error_text;    /** READ ONLY: a description for the error. */
	ssize_t frame_info_index; /** Points to the point in the frame info stack where the exception happened. */
};

/* Implementation detail. */
extern exception *__thrown_exception;

/** Prints the given exception to stdout. */
void print_exception(exception *e);

/** Prints the current stack trace to a string and returns it.
// For now, this uses statically allocated memory for the returned string.
// It will eventually allocate dynamic memory for the string and the caller
// will be responsible for free'ing the string.
// TODO: dynamically allocate the string.
*/
#define stack_trace_to_str() __stack_trace_to_str_expr(__LINE__,__FILE__,__func__)

/** Allocates a new exception. */
exception *new_exception(err_code_t error_code, char *mess, ...);

/** Call this to deallocate the memory used by an exception. */
/** This will be called automatically in TRY_CATCH(expr) ... ENDTRY blocks. */
exception *free_exception(exception *e);

/* Macro implementation details.  Do not use directly. */
char *__stack_trace_to_str_expr( uint32_t line, const char *file, const char *func );
jmp_buf *__push_stack_info(size_t line, const char *file, const char *func);
frame_info __pop_stack_info();

jmp_buf *__push_try_context();
jmp_buf *__pop_try_context();

/* __PROPOGATE_THROWN_EXCEPTIONS is an implementation detail.
// It does as the name suggests.  Do not call it from code that is not a part
// of this exception handling module.  It may change in the future if needed
// to fix bugs or add new features.
*/
#define __PROPOGATE_THROWN_EXCEPTIONS /* */ \
	do { \
		ERR_UTIL_TRACE("%s, %d.117: __PROPOGATE\n", __FILE__, __LINE__); \
		ERR_UTIL_TRACE("frame_info_index: %li\n",__frame_info_end-1); \
		if ( __frame_info_end-1 >= 0 ) \
			longjmp( \
				__frame_context_stack[__frame_info_end-1], \
				__thrown_exception->error_code); \
		else \
		{ \
			print_exception(__thrown_exception); /* TODO: this is probably a dynamic allocation and should be replaced by fprint_exception or something. */ \
			skit_die("Exception thrown with no handlers left in the stack."); \
		} \
	} while (0)

/**
Throws the exception 'e'.
This is usually used with the new_exception function, and allows for
  exceptions to be created in one place and then thrown in another.
While this is the strictly more powerful way to throw exceptions, the more
  convenient way for common cases would be to use the RAISE macro.
This macro expands to a statement and may not be nested inside expressions.
Example usage:
	THROW(new_exception(GENERIC_EXCEPTION,"Something bad happened!")); 
*/
#define THROW(e) \
	do { \
		ERR_UTIL_TRACE("%s, %d.136: THROW\n", __FILE__, __LINE__); \
		__thrown_exception = (e); \
		if ( __thrown_exception == NULL ) \
			__thrown_exception = new_exception(-1,"NULL was thrown."); \
		__push_stack_info(__LINE__,__FILE__,__func__); \
		__thrown_exception->frame_info_index = __frame_info_end; \
		__pop_stack_info(); \
		__PROPOGATE_THROWN_EXCEPTIONS; \
	} while (0)

/** 
Creates an exception of the type given and then throws it.
This is a more concise variant of the THROW macro.
The single argument version accepts an exception type as the first argument
  and uses the default message for that exception type.
  (TODO: The single arg version is not implemented yet.)
The double (or more) argument version accepts an exception type as the first 
  argument and a message as its second argument.  The message may be a C
  format string with substitution values given in subsequent arguments.
This macro expands to a statement and may not be nested inside expressions.
Example usage:
	RAISE(GENERIC_EXCEPTION); // Use the exception type's default message.
	RAISE(GENERIC_EXCEPTION,"Something bad happened!"); // More convenient syntax.
	RAISE(GENERIC_EXCEPTION,"Bad index: %d", index);    // Formatting is allowed.
*/
#define RAISE(...) MACRO_DISPATCHER3(RAISE, __VA_ARGS__)(__VA_ARGS__)

#define RAISE1(e) \
	THROW(new_exception(etype))
	
#define RAISE2(etype, emsg) \
	THROW(new_exception(etype, emsg))

#define RAISE3(etype, emsg, ...) \
	THROW(new_exception(etype, emsg, __VA_ARGS__))

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
		if ( setjmp(*__push_stack_info(__LINE__,__FILE__,__func__)) == 0 ) { \
			ERR_UTIL_TRACE("%s, %d.182: CALL.setjmp\n", __FILE__, __LINE__); \
			(expr); \
		} else { \
			ERR_UTIL_TRACE("%s, %d.186: CALL.longjmp\n", __FILE__, __LINE__); \
			__pop_stack_info(); \
			__PROPOGATE_THROWN_EXCEPTIONS; \
		} \
		ERR_UTIL_TRACE("%s, %d.190: CALL.success\n", __FILE__, __LINE__); \
		__pop_stack_info(); \
	} while (0)

#if 0
#define CALL(expr) /* */ \
	do { \
		SKIT_DEBUG_INFO_FLIST_GROW_MALLOC(skit_thread_ctx->debug_info_stack); \
		SKIT_JMP_FLIST_GROW_ALLOCA(skit_thread_ctx->exc_jmp_stack); \
		\
		/* Save a copy of the current jmp_stack position. */ \
		skit_jmp_flist __skit_exc_jmp_stack_save = skit_thread_ctx->exc_jmp_stack; \
		/* This ensures that we can end up where we started in this call, */ \
		/*   even if the callee messes up the jmp_stack. */ \
		/* The jmp_stack could be messed up if someone tries to jump */ \
		/*   execution out of a TRY-CATCH or scope guard block */ \
		/*   (using break/continue/goto/return) and somehow manages to succeed, */ \
		/*   thus preventing pop calls that are supposed to match push calls. */ \
		/* The debug_info_stack does not have this treatment because it is */ \
		/*   malloc'd instead of stack allocated.  It is also more resiliant */ \
		/*   to corruption because it does not appear in macros like */ \
		/*   TRY-CATCH where the programmer is likely to attempt jumping */ \
		/*   execution out of a block. */ \
		\
		*skit_debug_info_flist_push( \
			&skit_thread_ctx->debug_info_stack \
			) = skit_make_debug_info_snode(__LINE__,__FILE__,__func__); \
		\
		if ( setjmp(*skit_jmp_flist_push(&skit_thread_ctx->exc_jmp_stack)) == 0 ) { \
			ERR_UTIL_TRACE("%s, %d.182: CALL.setjmp\n", __FILE__, __LINE__); \
			(expr); \
		} else { \
			ERR_UTIL_TRACE("%s, %d.186: CALL.longjmp\n", __FILE__, __LINE__); \
			skit_thread_ctx->exc_jmp_stack = __skit_exc_jmp_stack_save; \
			skit_debug_info_flist_pop(&skit_thread_ctx->debug_info_stack); \
			__PROPOGATE_THROWN_EXCEPTIONS; \
		} \
		\
		skit_thread_ctx->exc_jmp_stack = __skit_exc_jmp_stack_save; \
		skit_debug_info_flist_pop(&skit_thread_ctx->debug_info_stack); \
		\
		ERR_UTIL_TRACE("%s, %d.190: CALL.success\n", __FILE__, __LINE__); \
	} while (0)

#endif
	
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
		ERR_UTIL_TRACE("%s, %d.236: TRY.if\n", __FILE__, __LINE__); \
		do { \
			ERR_UTIL_TRACE("%s, %d.238: TRY.do\n", __FILE__, __LINE__); \
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
				ERR_UTIL_TRACE("%s, %d.258: TRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
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
					ERR_UTIL_TRACE("%s, %d.282: TRY: case 0:\n", __FILE__, __LINE__); \

#define CATCH(__error_code, exc_name) /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					ERR_UTIL_TRACE("%s, %d.288: TRY: case 0: longjmp\n", __FILE__, __LINE__); \
					longjmp( __frame_context_stack[__frame_info_end-1], __TRY_EXCEPTION_CLEANUP); \
				} \
				else if ( exception_is_a( __thrown_exception->error_code, __error_code) ) \
				{ \
					/* CATCH block. */ \
					ERR_UTIL_TRACE("%s, %d.294: TRY: case %d:\n", __FILE__, __LINE__, __error_code); \
					exception *exc_name = __thrown_exception;

#define ENDTRY /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					ERR_UTIL_TRACE("%s, %d.301: TRY: case ??: longjmp\n", __FILE__, __LINE__); \
					longjmp( __frame_context_stack[__frame_info_end-1], __TRY_EXCEPTION_CLEANUP); \
				} \
				else \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					ERR_UTIL_TRACE("%s, %d.307: TRY: default: longjmp\n", __FILE__, __LINE__); \
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
			ERR_UTIL_TRACE("%s, %d.319: TRY: break found!\n", __FILE__, __LINE__); \
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
		ERR_UTIL_TRACE("%s, %d.331: TRY: continue found!\n", __FILE__, __LINE__); \
		__pop_stack_info(); \
		__pop_try_context(); \
		THROW(new_exception(CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" )); \
	} \
	ERR_UTIL_TRACE("%s, %d.339: TRY: done.\n", __FILE__, __LINE__);

#define SKIT_JMP_BUF_GROW_ALLOCA(list) \
	do { \
		if (skit_jmp_buf_flist_full(&(list))) \
			skit_jmp_buf_flist_grow(&(list),alloca(sizeof(skit_jmp_buf_snode*))); \
	} while(0)

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
	SKIT_JMP_FLIST_GROW_ALLOCA(skit_func_ctx.try_stack);
	if ( setjmp(*skit_jmp_flist_push(&skit_func_ctx.try_stack)) != __TRY_SAFE_EXIT ) { \
		ERR_UTIL_TRACE("%s, %d.236: TRY.if\n", __FILE__, __LINE__); \
		do { \
			ERR_UTIL_TRACE("%s, %d.238: TRY.do\n", __FILE__, __LINE__); \
			SKIT_JMP_FLIST_GROW_ALLOCA(skit_thread_ctx->exc_jmp_stack); \
			switch( setjmp(*skit_jmp_flist_push(&skit_thread_ctx->exc_jmp_stack)) ) \
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
				ERR_UTIL_TRACE("%s, %d.258: TRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
				if (__thrown_exception != NULL) \
				{ \
					free(__thrown_exception); \
					__thrown_exception = NULL; \
				} \
				skit_jmp_flist_pop(&skit_thread_ctx->exc_jmp_stack); \
				longjmp(*skit_jmp_flist_pop(&skit_func_ctx.try_stack), __TRY_SAFE_EXIT); \
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
					ERR_UTIL_TRACE("%s, %d.282: TRY: case 0:\n", __FILE__, __LINE__); \

#define CATCH(__error_code, exc_name) /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					ERR_UTIL_TRACE("%s, %d.288: TRY: case 0: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.front.val, __TRY_EXCEPTION_CLEANUP); \
				} \
				else if ( exception_is_a( __thrown_exception->error_code, __error_code) ) \
				{ \
					/* CATCH block. */ \
					ERR_UTIL_TRACE("%s, %d.294: TRY: case %d:\n", __FILE__, __LINE__, __error_code); \
					exception *exc_name = __thrown_exception;

#define ENDTRY /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					ERR_UTIL_TRACE("%s, %d.301: TRY: case ??: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.front.val, __TRY_EXCEPTION_CLEANUP); \
				} \
				else \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					ERR_UTIL_TRACE("%s, %d.307: TRY: default: longjmp\n", __FILE__, __LINE__); \
					skit_jmp_flist_pop(&skit_thread_ctx->exc_jmp_stack); \
					skit_jmp_flist_pop(&skit_func_ctx.try_stack); \
					__PROPOGATE_THROWN_EXCEPTIONS; \
				} \
				assert(0); /* This should never be reached. The if-else chain above should handle all remaining cases. */ \
			} /* default: { } */ \
			} /* switch(setjmp(*__push...)) */ \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			ERR_UTIL_TRACE("%s, %d.319: TRY: break found!\n", __FILE__, __LINE__); \
			skit_jmp_flist_pop(&skit_thread_ctx->exc_jmp_stack); \
			skit_jmp_flist_pop(&skit_func_ctx.try_stack); \
			THROW(new_exception(BREAK_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'break' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" )); \
		} while (0); \
		/* If execution makes it here, then the caller */ \
		/* used a "continue" statement and is trying to */ \
		/* corrupt the debug stack.  Don't let them do it! */ \
		/* Instead, throw another exception. */ \
		ERR_UTIL_TRACE("%s, %d.331: TRY: continue found!\n", __FILE__, __LINE__); \
		skit_jmp_flist_pop(&skit_thread_ctx->exc_jmp_stack); \
		skit_jmp_flist_pop(&skit_func_ctx.try_stack); \
		THROW(new_exception(CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" )); \
	} \
	ERR_UTIL_TRACE("%s, %d.339: TRY: done.\n", __FILE__, __LINE__);
#endif
#endif