
#include "survival_kit/setjmp/jmp_slist.h"

#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/flist.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX
