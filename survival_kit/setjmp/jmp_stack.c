
#include <setjmp.h>

#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/stack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

