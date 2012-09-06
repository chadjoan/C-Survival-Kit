
#include <stdarg.h>
#include <unistd.h> /* For ssize_t */
#include <string.h>
#include <stdio.h>

#include <pthread.h>

#include "survival_kit/misc.h"
#include "survival_kit/features.h"

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_PREFIX debug
#include "survival_kit/templates/stack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_PREFIX debug
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_PREFIX exc
#include "survival_kit/templates/stack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_PREFIX exc
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/*
frame_info  __frame_info_stack[FRAME_INFO_STACK_SIZE];
jmp_buf     __frame_context_stack[FRAME_INFO_STACK_SIZE];
ssize_t     __frame_info_end = 0;

jmp_buf     __try_context_stack[TRY_CONTEXT_STACK_SIZE];
ssize_t     __try_context_end = 0;
*/


/* Unittesting functions. */
static int baz()
{
	printf("Baz doesn't throw.\n");
	printf("It does print a stack trace before returning, though:\n");
	printf("%s",stack_trace_to_str());
	printf("\n");
	return 0;
}

static float qux()
{
	RAISE(GENERIC_EXCEPTION,"qux does throw!\n");
	printf("qux should not print this.\n");
	return 0.0;
}

static void bar(int val1, int val2)
{
	printf("This shouldn't get executed.\n");
}

static int foo()
{
	/* TODO: bar(TRYR(baz()),TRYR(qux())); */
	CALL(bar(baz(), qux()));
	printf("This shouldn't get executed either.\n");
	return 42;
}

static void unittest_exceptions()
{
	if( !exception_is_a(GENERIC_EXCEPTION, GENERIC_EXCEPTION) )  skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if( !exception_is_a(BREAK_IN_TRY_CATCH, GENERIC_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if(  exception_is_a(FATAL_EXCEPTION, GENERIC_EXCEPTION) )    skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	
	TRY
		baz();
		printf("No exception thrown.\n");
	CATCH(GENERIC_EXCEPTION, e)
		print_exception(e);
	ENDTRY
	
	printf("------\n");
	
	TRY
		TRY
			printf("Nested TRY-TRY-CATCH test.\n");
		CATCH(GENERIC_EXCEPTION, e)
			print_exception(e);
		ENDTRY
		
		foo();
		printf("No exception thrown.\n");
	CATCH(GENERIC_EXCEPTION, e)
		TRY
			printf("Nested CATCH-TRY-CATCH test.\n");
		CATCH(GENERIC_EXCEPTION, e2)
			printf("This shouldn't happen.\n");
			UNUSED(e2);
			/* print_exception(e2);*/
		ENDTRY
		
		print_exception(e);
	ENDTRY
	
	TRY
		while(1)
		{
			TRY
				printf("I'm in a loop.\n");
				printf("Now to bail without stopping first!\n");
				break;
			CATCH(GENERIC_EXCEPTION,e)
				UNUSED(e);
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
			ENDTRY
		}
	CATCH(BREAK_IN_TRY_CATCH, e)
		print_exception(e);
	ENDTRY
	
	
	
	TRY
		while(1)
		{
			TRY
				printf("I'm in a loop (again).\n");
				printf("Now to bail without stopping first!\n");
				continue;
			CATCH(GENERIC_EXCEPTION,e)
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
				UNUSED(e);
			ENDTRY
		}
	CATCH(CONTINUE_IN_TRY_CATCH, e)
		print_exception(e);
	ENDTRY
	
	printf("  exception_handling unittest passed!\n");
}


/* All scope unittest subfunctions should return 0. */
/*
static int unittest_scope_exit_normal(int *result)
{
	int *result = 2;
	SCOPE_EXIT(*result--);
	SCOPE_EXIT
		*result--;
	ENDSCOPE
	RETURN;
}

static void unittest_scope_exit_exceptional(int *result)
{
	*result = 2;
	SCOPE_EXIT(*result--);
	SCOPE_EXIT
		*result--;
	ENDSCOPE
	RAISE(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

static void unittest_scope_failure_noraml(int *result)
{
	*result = 0;
	SCOPE_FAILURE(*result--);
	SCOPE_FAILURE
		*result--;
	ENDSCOPE
	RETURN;
}

static void unittest_scope_failure_exceptional(int *result)
{
	int *result = 2;
	SCOPE_FAILURE(*result--);
	SCOPE_FAILURE
		*result--;
	ENDSCOPE
	RAISE(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

static void unittest_scope_success_normal(int *result)
{
	int *result = 2;
	SCOPE_SUCCESS(*result--);
	SCOPE_SUCCESS
		*result--;
	ENDSCOPE
	RETURN;
}

static void unittest_scope_success_exceptional(int *result)
{
	*result = 0;
	SCOPE_SUCCESS(*result--);
	SCOPE_SUCCESS
		*result--;
	ENDSCOPE
	RAISE(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

static void unittest_scope()
{
	int val = 1;
	
	unittest_scope_exit_normal(&val);
	assert_eq(val,0);
	
	unittest_scope_success_normal(&val);
	assert_eq(val,0);
	
	unittest_scope_failure_normal(&val);
	assert_eq(val,0);
	
	TRY
		unittest_scope_exit_exceptional(&val);
	CATCH(GENERIC_EXCEPTION,e)
		assert_eq(val,0);
	ENDTRY
	
	TRY
		unittest_scope_success_exceptional(&val);
	CATCH(GENERIC_EXCEPTION,e)
		assert_eq(val,0);
	ENDTRY
	
	TRY
		unittest_scope_failure_exceptional(&val);
	CATCH(GENERIC_EXCEPTION,e)
		assert_eq(val,0);
	ENDTRY
	
	printf("  scope unittest passed!\n");
}
*/

void skit_unittest_features()
{
	printf("skit_unittest_features()\n");
	unittest_exceptions();
	/* unittest_scope(); */
	printf("  features unittest passed!\n");
}
/* End unittests. */

static void best_effort_vms_path_parse_device(
	const char *path,
	      char **device,
	const char **rest )
{
	size_t i = 0;
	char c = path[i];
	while ( c != '\0' )
	{
		if ( c == ':' )
		{
			*device = malloc(i+1);
			memcpy(*device, path, i);
			(*device)[i] = '\0';
			*rest = path+i+1;
			return;
		}
		
		c = path[++i];
	}
	
	/* Return the empty string for device. */
	*device = malloc(1);
	(*device)[0] = '\0';
	*rest = path;
	return;
}

static void best_effort_vms_path_parse_directory(
	const char *path,
	      char **directory,
	const char **rest )
{
	size_t i = 0;
	char endchar = '\0';
	char c = path[i];
	if ( c == '[' || c == '<' )
	while ( c != '\0' )
	{
		if ( c == '[' )
		{
			endchar = ']';
		}
		else if ( c == '<' )
		{
			endchar = '>';
		}
		else if ( i > 0 && c == endchar )
		{
			*directory = malloc(i);
			memcpy(*directory, path+1, i);
			(*directory)[i] = '\0';
			*rest = path+i+1;
			return;
		}
		
		c = path[++i];
	}
	
	/* Return the empty string for directory. */
	*directory = malloc(1);
	(*directory)[0] = '\0';
	*rest = path;
	return;
}

/* Attempts to break apart an OpenVMS path. */
/* This does not make system calls, so it could become outdated. */
static void best_effort_vms_path_parse(
	const char *path,        /* input: the openvms path to parse. */
	char **device,
	char **directory,
	char **name)
{
	const char *rest = path;
	best_effort_vms_path_parse_device(rest, device, &rest);
	best_effort_vms_path_parse_directory(rest, directory, &rest);
	
	size_t rest_len = strlen(rest);
	*name = malloc(rest_len+1);
	memcpy(*name, rest, rest_len);
	(*name)[rest_len] = '\0';
	return;
}

/* TODO: use malloc instead of global/static stuff. */
#define MSG_BUF_SIZE 32768
static char msg_buf[MSG_BUF_SIZE];
static char *stack_trace_to_str_internal(ssize_t frame_end)
{
	char *msg_pos = msg_buf;
	ssize_t msg_rest_length = MSG_BUF_SIZE;
	ssize_t i;
	
	for ( i = frame_end-1; i >= 0; i-- )
	{
		frame_info fi = __frame_info_stack[i];
		
		char *device;
		char *directory;
		char *name;
		best_effort_vms_path_parse(fi.file_name, &device, &directory, &name);
		
		ssize_t nchars = snprintf(
			msg_pos, msg_rest_length,
			"%s: at line %d in function %s\r\n",
			name,
			fi.line_number,
			fi.func_name);
		
		free(device);
		free(directory);
		free(name);
		
		if ( nchars < 0 )
			continue;
		msg_pos += nchars;
		msg_rest_length -= nchars;
		if ( msg_rest_length < 0 )
			break;
	}
	
	return msg_buf;
}

pthread_key_t skit_thread_context_key;

static void skit_thread_context_init( skit_thread_context *ctx )
{
	skit_jmp_fstack_init(&ctx->try_jmp_stack);
	skit_jmp_fstack_init(&ctx->exc_jmp_stack);
	skit_jmp_fstack_init(&ctx->scope_jmp_stack);
	skit_debug_fstack_init(&ctx->debug_info_stack);
	skit_exc_fstack_init(&ctx->exc_instance_stack);
	
	if( setjmp( *skit_jmp_fstack_alloc(&ctx->exc_jmp_stack) ) != 0 )
	{
		/* Uncaught exception(s)!  We're going down! */
		while ( ctx->exc_instance_stack.used.length > 0 )
		{
			print_exception( skit_exc_fstack_pop(&ctx->exc_instance_stack) );
		}
		skit_die("Uncaught exception(s).");
	}
}

static void skit_thread_context_dtor(void *ctx_ptr)
{
	skit_thread_context *ctx = (skit_thread_context*)ctx_ptr;
	/* Do nothing for now. TODO: This will be important for multithreading. */
}

void skit_features_init()
{
	pthread_key_create(&skit_thread_context_key, skit_thread_context_dtor); 
	skit_features_thread_init();
}

void skit_features_thread_init()
{
	skit_thread_context *ctx = (skit_thread_context*)skit_malloc(sizeof(skit_thread_context));
	skit_thread_context_init(ctx);
	pthread_setspecific(skit_thread_context_key, (void*)ctx);
}

skit_thread_context *skit_thread_context_get()
{
	return (skit_thread_context*)pthread_getspecific(skit_thread_context_key);
}

void skit_save_thread_context_pos( skit_thread_context *ctx, skit_thread_context_pos *pos )
{
	pos->try_jmp_pos    = skit_thread_ctx->try_jmp_stack.used.length;
	pos->exc_jmp_pos    = skit_thread_ctx->exc_jmp_stack.used.length;
	pos->scope_jmp_pos  = skit_thread_ctx->scope_jmp_stack.used.length;
	pos->debug_info_pos = skit_thread_ctx->debug_info_stack.used.length;
}

static void skit_fstack_reconcile_warn( ssize_t expected, ssize_t got, char *name )
{
	fprintf(stderr,"Warning: %s was unbalanced after the most recent return.\n");
	fprintf(stderr,"  Expected size: %li\n", expected );
	fprintf(stderr,"  Actual size:   %li\n", got );
	fprintf(stderr,"  This may mean that a goto, break, continue, or return was made while inside\n");
	fprintf(stderr,"  a TRY-CATCH block or a SCOPE guard.  Jumping away from within a TRY-CATCH\n");
	fprintf(stderr,"  block or SCOPE guard with raw C primitives can lead to very buggy, bizarre,\n");
	fprintf(stderr,"  and inconsistent runtime behavior.  Just don't do it.\n");
	/* TODO: there should be some non-primitive control constructs that should be mentioned here
	 *   as a way of accomplishing desired logic in TRY-CATCH statements. */
	/* TODO: Although we can probably fix any problems the user creates for themselves, dieing might be better than warning. */
	
	printf("Printing stack trace:\n");
	printf("%s",stack_trace_to_str());
}

#define SKIT_FSTACK_RECONCILE(stack, prev_length, name, name_str) \
	do { \
		if ( (stack).used.length > prev_length ) \
		{ \
			skit_fstack_reconcile_warn(prev_length, (stack).used.length, name_str); \
			while ( (stack).used.length > prev_length ) \
				name##_pop((stack)); \
		} \
		else if ( (stack).used.length < prev_length ) \
		{ \
			skit_fstack_reconcile_warn(prev_length, (stack).used.length, name_str); \
			skit_die("\nStack was popped too far!  This cannot be recovered.\n"); \
		} \
	} while (0)

void skit_reconcile_thread_context( skit_thread_context *ctx, skit_thread_context_pos *pos )
{
	SKIT_FSTACK_RECONCILE(ctx->try_jmp_stack,    pos->try_jmp_pos,    skit_jmp_fstack,   "try_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->exc_jmp_stack,    pos->exc_jmp_pos,    skit_jmp_fstack,   "exc_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->scope_jmp_stack,  pos->scope_jmp_pos,  skit_jmp_fstack,   "scope_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->debug_info_stack, pos->debug_info_pos, skit_debug_fstack, "debug_info_stack");
}

char *__stack_trace_to_str_expr( uint32_t line, const char *file, const char *func )
{
	__push_stack_info(line,file,func);
	char *result = stack_trace_to_str_internal(__frame_info_end);
	__pop_stack_info();
	return result;
}

exception *__thrown_exception = NULL;

void print_exception(exception *e)
{
	printf("%s\n",e->error_text);
	printf("%s\n",stack_trace_to_str_internal(e->frame_info_index));
}

#if 0
exception *no_exception()
{
	exception *result = malloc(sizeof(exception));
	/* result->error_code = 0; */
	result->error_text = NULL;
	return result;
}
#endif

exception *skit_new_exception(ssize_t error_code, char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	vsnprintf(skit_error_text_buffer, SKIT_ERROR_BUFFER_SIZE, mess, vl);
	va_end(vl);
	
	exception *result = malloc(sizeof(exception));
	result->error_code = error_code;
	result->error_text = skit_error_text_buffer;
	return result;
}


void skit_debug_info_store( skit_frame_info *dst, int line, const char *file, const char *func )
{
	ERR_UTIL_TRACE("%s, %li: skit_debug_info_store(...,%li, %s, %s)\n", file, line, line, file, func);
	
	dst->line_number = line;
	dst->file_name = file;
	dst->func_name = func;
}

/*
jmp_buf *__push_stack_info(size_t line, const char *file, const char *func)
{
	ERR_UTIL_TRACE("%s, %li: __push_stack_info(%li, %s, %s)\n", file, line, line, file, func);
	frame_info fi;
	fi.line_number = line;
	fi.file_name = file;
	fi.func_name = func;
	fi.jmp_context = &__frame_context_stack[__frame_info_end];
	__frame_info_stack[__frame_info_end] = fi;
	__frame_info_end++;
	return fi.jmp_context;
}

frame_info __pop_stack_info()
{
	__frame_info_end--;
	frame_info fi;
	
	if ( __frame_info_end < 0 )
	{
		fi = __frame_info_stack[0];
		skit_die("Attempt to pop stack info at stack bottom.\r\n"		
			"(probably) Happened in file %s at line %li in function %s\r\n",
			fi.file_name, fi.line_number, fi.func_name);
	}
	
	fi = __frame_info_stack[__frame_info_end];
	ERR_UTIL_TRACE("__pop_stack_info(%d, %s, %s)\n", fi.line_number, fi.file_name, fi.func_name);

	return fi;
}


jmp_buf *__push_try_context()
{
	if ( __try_context_end >= TRY_CONTEXT_STACK_SIZE )
	{
		skit_die("Exceeded TRY stack size of %li.\n%s\n", TRY_CONTEXT_STACK_SIZE, stack_trace_to_str());
	}
	
	return &__try_context_stack[__try_context_end++];
}

jmp_buf *__pop_try_context()
{
	__try_context_end--;
	if ( __try_context_end < 0 )
	{
		skit_die("TRY context stack pop with no matching push.\n%s\n", stack_trace_to_str());
	}
	
	return &__try_context_stack[__try_context_end];
}
*/