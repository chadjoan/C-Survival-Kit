#ifndef ERR_UTIL_INCLUDED
#define ERR_UTIL_INCLUDED

#include <stdlib.h>
#include <inttypes.h>

typedef struct frame_info frame_info;
struct frame_info
{
	uint32_t line_number;
	const char *file_name;
	const char *func_name;
};

/* TODO: these should be stored in thread-local storage. */
#define FRAME_INFO_STACK_SIZE 1024
extern frame_info __frame_info_stack[FRAME_INFO_STACK_SIZE];
extern size_t     __frame_info_end;

typedef struct exception exception;
struct exception
{
	/* Implementation note: keep this small, it will be returned by-value from functions a lot. */
	size_t error_code;   /* 0 should always mean "no error" */
	char *error_text;    /* READ ONLY: a description for the error. */
};

/* TODO: these should be stored in thread-local storage. */
#define ERROR_BUFFER_SIZE 1024
extern char error_text_buffer[ERROR_BUFFER_SIZE];
extern exception *__thrown_exception;

void print_exception(exception *e);

/*exception *no_exception();*/

/* Allocates a new exception. */
exception *new_exception(char *mess, ...);

/* Call this to deallocate the memory used by an exception. */
/* This will be called automatically in TRY_CATCH(expr) ... ENDTRY blocks. */
exception *free_exception(exception *e);

void die(char *mess, ...);

int __push_stack_info(size_t line, const char *file, const char *func);

/* #define __STACK_UNWIND(__RETURN_STATEMENT) \
		__thrown_exception == NULL ? 0 : (__push_stack_info(__LINE__,__FILE__,__func__),__RETURN_STATEMENT)
*/

#define __STACK_UNWIND(__RETURN_STATEMENT) \
		if ( __thrown_exception != NULL ) \
		{ \
			__push_stack_info(__LINE__,__FILE__,__func__); \
			__RETURN_STATEMENT; \
		}

#define THROW(expr, __RETURN_STATEMENT) \
	do { \
		__thrown_exception = expr; \
		if ( __thrown_exception == NULL ) \
			__thrown_exception = new_exception("NULL was thrown."); \
		__frame_info_end = 0; \
		__STACK_UNWIND(__RETURN_STATEMENT); \
	} while (0)

#define THROW_NUM(expr) THROW(expr,return 0)
#define THROW_VOID(expr) THROW(expr,return)

#define TRY(expr, __RETURN_STATEMENT) \
	do { \
		expr; \
		__STACK_UNWIND(__RETURN_STATEMENT) \
	} while (0)

#define TRY_NUM(expr)   TRY(expr,return 0)
#define TRY_VOID(expr)  TRY(expr,return)
	
#define TRY_CATCH(expr) \
	expr;\
	if ( __thrown_exception == NULL ) {

#define CATCH(__exception_name) \
	} else { \
		__STACK_UNWIND() \
		exception *__exception_name = __thrown_exception;

#define ENDTRY \
		free(__thrown_exception); \
		__thrown_exception = NULL; \
	}

#endif