
#ifdef __DECC
#pragma module skit_feature_emu_setjmp_stack
#endif

#include <setjmp.h>

#define SKIT_T_DIE_ON_ERROR
#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/stack.h"
#include "survival_kit/templates/stack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

