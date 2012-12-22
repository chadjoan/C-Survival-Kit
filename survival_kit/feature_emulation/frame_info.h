
#ifndef SKIT_FEMU_FRAME_INFO_INCLUDED
#define SKIT_FEMU_FRAME_INFO_INCLUDED

#include <inttypes.h>

/* Implementation details. */
typedef struct skit_frame_info skit_frame_info;
struct skit_frame_info
{
	uint32_t line_number;
	const char *file_name;
	const char *func_name;
	const char *specifics;
};

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_NAME debug
#include "survival_kit/templates/stack.h"
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME

/* Internal: used in macros the need to convert debug info info frame_info objects. */
void skit_debug_info_store( skit_frame_info *dst, int line, const char *file, const char *func, const char *specifics );

#endif
