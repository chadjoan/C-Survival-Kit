
#ifdef __DECC
#pragma module skit_feature_emu_frame_info
#endif

#include <inttypes.h>

#include "survival_kit/feature_emulation/frame_info.h"

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_NAME debug
#include "survival_kit/templates/stack.c"
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_DIE_ON_ERROR
