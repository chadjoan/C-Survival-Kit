
#ifdef __DECC
#pragma module skit_feature_emu_frame_info
#endif

#include <inttypes.h>

#include "survival_kit/misc.h" /* skit_die */
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/debug.h"

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_NAME debug
#include "survival_kit/templates/stack.c"
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_DIE_ON_ERROR

void skit_debug_info_store( skit_frame_info *dst, int line, const char *file, const char *func )
{
	SKIT_FEATURE_TRACE("%s, %d: skit_debug_info_store(...,%d, %s, %s)\n", file, line, line, file, func);
	
	dst->line_number = line;
	dst->file_name = file;
	dst->func_name = func;
}
