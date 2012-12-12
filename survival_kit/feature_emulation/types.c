
#ifdef __DECC
#pragma module skit_feature_emulation_types
#endif

#include <setjmp.h>

#include "survival_kit/feature_emulation/types.h"

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_NAME debug
#include "survival_kit/templates/stack.c"
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME

#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_NAME exc
#include "survival_kit/templates/stack.c"
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
