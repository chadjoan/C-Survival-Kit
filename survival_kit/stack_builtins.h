
#ifndef SKIT_STACK_BUILTINS_INCLUDED
#define SKIT_STACK_BUILTINS_INCLUDED

#include <inttypes.h>

/* SKIT_T_HEADER allows the template header file to be overridden. */
/* It is used by the corresponding _builtins.c file to provide linkable */
/*   implementations for all of these template instances. */
/* It is not recommended that any calling code use SKIT_T_HEADER. */
#ifndef SKIT_T_HEADER
#	define SKIT_T_HEADER "survival_kit/templates/stack.h"
#	define SKIT_CLEAN_STACK_BUILTINS 1
#endif

/** Define skit_u32_stack */
#define SKIT_T_ELEM_TYPE uint32_t
#define SKIT_T_PREFIX u32
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_u16_stack */
#define SKIT_T_ELEM_TYPE uint16_t
#define SKIT_T_PREFIX u16
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_u8_stack */
#define SKIT_T_ELEM_TYPE uint8_t
#define SKIT_T_PREFIX u8
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i32_stack */
#define SKIT_T_ELEM_TYPE int32_t
#define SKIT_T_PREFIX i32
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i16_stack */
#define SKIT_T_ELEM_TYPE int16_t
#define SKIT_T_PREFIX i16
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i8_stack */
#define SKIT_T_ELEM_TYPE int8_t
#define SKIT_T_PREFIX i8
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_double_stack */
#define SKIT_T_ELEM_TYPE double
#define SKIT_T_PREFIX double
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_float_stack */
#define SKIT_T_ELEM_TYPE float
#define SKIT_T_PREFIX float
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_cstr_stack  (SKIT_T_ELEM_TYPE==char*) */
#define SKIT_T_ELEM_TYPE char*
#define SKIT_T_PREFIX cstr
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/* Double guard this: stack_builtins.c will #undef the normal include guards
because it needs to expand this file twice: once for the definitions and again
for the implementations. */
#ifndef SKIT_STACK_UNITTEST_INCLUDED
#define SKIT_STACK_UNITTEST_INCLUDED
/** */
void skit_stack_unittest();
#endif

#ifdef SKIT_CLEAN_STACK_BUILTINS
#	undef SKIT_T_HEADER
#	undef SKIT_CLEAN_STACK_BUILTINS
#endif

#endif
