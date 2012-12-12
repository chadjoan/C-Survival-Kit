
#ifndef SKIT_JMP_STACK_INCLUDED
#define SKIT_JMP_STACK_INCLUDED

#include <setjmp.h>

/**
Define skit_jmp_stack. 
This is a list of jmp_buf objects used with the setjmp and longjmp functions
  defined in setjmp.h.
*/
#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_NAME jmp
#include "survival_kit/templates/stack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME

#endif
