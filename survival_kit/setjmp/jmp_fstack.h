
#ifndef SKIT_JMP_FLIST_INCLUDED
#define SKIT_JMP_FLIST_INCLUDED

#include "survival_kit/setjmp/jmp_stack.h"

#include <setjmp.h>

/**
Define skit_jmp_fstack. 
This is a list of jmp_buf objects used with the setjmp and longjmp functions
  defined in setjmp.h.
*/
#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#endif
