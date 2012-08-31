#ifndef ERR_UTIL_INCLUDED
#define ERR_UTIL_INCLUDED

#include <stdlib.h>
#include <unistd.h> /* For ssize_t */
#include <inttypes.h>
#include <limits.h> /* For INT_MIN and the like. */
#include <setjmp.h>

#include "UTIL_MACRO.H"
#include "UTIL_EXCEPTIONS_GENERATED.H"

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
void unittest_err_util();

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

/* Implementation details. */
/* TODO: these should be stored in thread-local storage. */
#define ERROR_BUFFER_SIZE 1024
extern char error_text_buffer[ERROR_BUFFER_SIZE];
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

/** Unconditionally exit the program and print the formatted string 'mess'. */
void die(char *mess, ...);

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
		ERR_UTIL_TRACE("%s, %d.93: __PROPOGATE\n", __FILE__, __LINE__); \
		ERR_UTIL_TRACE("frame_info_index: %d\n",__frame_info_end-1); \
		longjmp( \
			__frame_context_stack[__frame_info_end-1], \
			__thrown_exception->error_code); \
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
		ERR_UTIL_TRACE("%s, %d.102: THROW\n", __FILE__, __LINE__); \
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
			ERR_UTIL_TRACE("%s, %d.115: CALL.setjmp\n", __FILE__, __LINE__); \
			(expr); \
		} else { \
			ERR_UTIL_TRACE("%s, %d.118: CALL.longjmp\n", __FILE__, __LINE__); \
			__pop_stack_info(); \
			__PROPOGATE_THROWN_EXCEPTIONS; \
		} \
		ERR_UTIL_TRACE("%s, %d.122: CALL.success\n", __FILE__, __LINE__); \
		__pop_stack_info(); \
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
		ERR_UTIL_TRACE("%s, %d.153: TRY.if\n", __FILE__, __LINE__); \
		do { \
			ERR_UTIL_TRACE("%s, %d.155: TRY.do\n", __FILE__, __LINE__); \
			switch ( setjmp(*__push_stack_info(__LINE__,__FILE__,__func__)) ) \
			{ \
			case 0: { \
				ERR_UTIL_TRACE("%s, %d.159: TRY: case 0:\n", __FILE__, __LINE__); \
			case __TRY_EXCEPTION_CLEANUP: { \
					ERR_UTIL_TRACE("%s, %d.161: TRY: case CLEANUP\n", __FILE__, __LINE__); \
					if ( setjmp(*__push_try_context()) == __TRY_EXCEPTION_CLEANUP ) \
					{ \
						ERR_UTIL_TRACE("%s, %d.164: TRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
						free(__thrown_exception); \
						__thrown_exception = NULL; \
						longjmp(*__pop_try_context(), __TRY_SAFE_EXIT); \
					} \
					ERR_UTIL_TRACE("%s, %d.170: TRY: case CLEANUP: setjmp\n", __FILE__, __LINE__); \
				}

#define CATCH(__error_code, exc_name) /* */ \
				__pop_stack_info(); \
				ERR_UTIL_TRACE("%s, %d.174: TRY: case 0: longjmp\n", __FILE__, __LINE__); \
				longjmp(*__pop_try_context(), __TRY_EXCEPTION_CLEANUP); \
				} \
			case __error_code: { \
				ERR_UTIL_TRACE("%s, %d.178: TRY: case %d:\n", __FILE__, __LINE__, __error_code); \
				exception *exc_name = __thrown_exception;

#define ENDTRY /* */ \
				ERR_UTIL_TRACE("%s, %d.182: TRY: case ??: longjmp\n", __FILE__, __LINE__); \
				__pop_stack_info(); \
				longjmp(*__pop_try_context(), __TRY_EXCEPTION_CLEANUP); \
				} \
			default: { \
				ERR_UTIL_TRACE("%s, %d.187: TRY: default: longjmp\n", __FILE__, __LINE__); \
				__pop_stack_info(); \
				__pop_try_context(); \
				__pop_try_context(); \
				__PROPOGATE_THROWN_EXCEPTIONS; \
				} \
			} \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			ERR_UTIL_TRACE("%s, %d.192: TRY: break found!\n", __FILE__, __LINE__); \
			__pop_stack_info(); \
			__pop_try_context(); \
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
		ERR_UTIL_TRACE("%s, %d.198: TRY: continue found!\n", __FILE__, __LINE__); \
		__pop_stack_info(); \
		__pop_try_context(); \
		__pop_try_context(); \
		THROW(new_exception(CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a TRY-CATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" )); \
	} \
	ERR_UTIL_TRACE("%s, %d.204: TRY: done.\n", __FILE__, __LINE__);
	
#endif