
#include "ERR_UTIL.H"
#include <stdlib.h>
#include <unistd.h> /* For ssize_t */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <lib$routines.h> /* lib$signal */

frame_info  __frame_info_stack[FRAME_INFO_STACK_SIZE];
jmp_buf     __frame_context_stack[FRAME_INFO_STACK_SIZE];
ssize_t     __frame_info_end = 0;

jmp_buf     __try_context_stack[TRY_CONTEXT_STACK_SIZE];
ssize_t     __try_context_end = 0;

void unittest_err_util()
{
	if( !exception_is_a(GENERIC_EXCEPTION, GENERIC_EXCEPTION) )  RAISE(GENERIC_EXCEPTION,"Assertion failed.");
	if( !exception_is_a(BREAK_IN_TRY_CATCH, GENERIC_EXCEPTION) ) RAISE(GENERIC_EXCEPTION,"Assertion failed.");
	if(  exception_is_a(FATAL_EXCEPTION, GENERIC_EXCEPTION) )    RAISE(GENERIC_EXCEPTION,"Assertion failed.");
}
	
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

char *__stack_trace_to_str_expr( uint32_t line, const char *file, const char *func )
{
	__push_stack_info(line,file,func);
	char *result = stack_trace_to_str_internal(__frame_info_end);
	__pop_stack_info();
	return result;
}

char error_text_buffer[ERROR_BUFFER_SIZE];
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

exception *new_exception(ssize_t error_code, char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	vsnprintf(error_text_buffer, ERROR_BUFFER_SIZE, mess, vl);
	va_end(vl);
	
	exception *result = malloc(sizeof(exception));
	result->error_code = error_code;
	result->error_text = error_text_buffer;
	return result;
}

void die(char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	vsnprintf(error_text_buffer, ERROR_BUFFER_SIZE, mess, vl);
	va_end(vl);
	
	perror(error_text_buffer);
	exit(1);
	/*lib$signal(EXIT_FAILURE);*/ /* This produced too much spam when exiting after a bunch of setjmp/longjmps. */
}

jmp_buf *__push_stack_info(size_t line, const char *file, const char *func)
{
	ERR_UTIL_TRACE("%s, %d: __push_stack_info(%d, %s, %s)\n", file, line, line, file, func);
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
		die("Attempt to pop stack info at stack bottom.\r\n"		
			"(probably) Happened in file %s at line %d in function %s\r\n",
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
		die("Exceeded TRY stack size of %d.\n%s\n", TRY_CONTEXT_STACK_SIZE, stack_trace_to_str());
	}
	
	return &__try_context_stack[__try_context_end++];
}

jmp_buf *__pop_try_context()
{
	__try_context_end--;
	if ( __try_context_end < 0 )
	{
		die("TRY context stack pop with no matching push.\n%s\n", stack_trace_to_str());
	}
	
	return &__try_context_stack[__try_context_end];
}