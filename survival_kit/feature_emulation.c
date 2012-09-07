
#include <stdarg.h>
#include <unistd.h> /* For ssize_t */
#include <string.h>
#include <stdio.h>

#include <pthread.h>

#include "survival_kit/misc.h"
#include "survival_kit/feature_emulation.h"

/*
frame_info  __frame_info_stack[FRAME_INFO_STACK_SIZE];
jmp_buf     __frame_context_stack[FRAME_INFO_STACK_SIZE];
ssize_t     __frame_info_end = 0;

jmp_buf     __try_context_stack[TRY_CONTEXT_STACK_SIZE];
ssize_t     __try_context_end = 0;
*/



#if 0
exception *__thrown_exception = NULL;

exception *no_exception()
{
	exception *result = malloc(sizeof(exception));
	/* result->error_code = 0; */
	result->error_text = NULL;
	return result;
}

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
#endif



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