
#include <setjmp.h>

#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/stack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

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
