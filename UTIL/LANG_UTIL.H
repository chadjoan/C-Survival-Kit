#ifndef LANG_UTIL_INCLUDED
#define LANG_UTIL_INCLUDED

#include <inttypes.h>
#include <stdlib.h>

typedef struct frame_info frame_info;
struct frame_info
{
	uint32_t line_number;
	const char *file_name;
	const char *func_name;
};

#define FRAME_INFO_STACK_SIZE 1024
extern frame_info __frame_info_stack[FRAME_INFO_STACK_SIZE];
extern size_t     __frame_info_end;

#define ARGS(...) \
	( ##__VA_ARGS__ \
	uint32_t __caller_line = __LINE__, \
	const char *__caller_file = __FILE__) 

#define FBEGIN \
	do {\
		frame_info __frame_info; \
		__frame_info.line_number = __caller_line; \
		__frame_info.file_name = __caller_file; \
		__frame_info.func_name = __func__; \
		__frame_info_stack[__frame_info_end++] = __frame_info; \
	} while(0)

#define FEND \
	__frame_info_end--

#define FRETURN(val) \
	do { \
		__frame_info_end--; \
		return val; \
	} while(0)

// Creates human-readable text representing the current stack.
char *lang_util_stack_to_str();

#endif