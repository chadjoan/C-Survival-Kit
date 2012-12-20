
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
};

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_NAME debug
#include "survival_kit/templates/stack.h"
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME

#endif
