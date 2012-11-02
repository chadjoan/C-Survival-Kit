
#ifndef SKIT_ARRAY_BUILTINS_INCLUDED
#define SKIT_ARRAY_BUILTINS_INCLUDED

#include <inttypes.h>

#include "survival_kit/string.h"

/* SKIT_T_HEADER allows the template header file to be overridden. */
/* It is used by the corresponding _builtins.c file to provide linkable */
/*   implementations for all of these template instances. */
/* It is not recommended that any calling code use SKIT_T_HEADER. */
#ifndef SKIT_T_HEADER
#	define SKIT_T_HEADER "survival_kit/templates/array.h"
#	define SKIT_CLEAN_ARRAY_BUILTINS 1
#endif

/** Define skit_u32_slice and skit_u32_loaf */
#define SKIT_T_ELEM_TYPE uint32_t
#define SKIT_T_PREFIX u32
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_u16_slice and skit_u16_loaf */
#define SKIT_T_ELEM_TYPE uint16_t
#define SKIT_T_PREFIX u16
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_u8_slice and skit_u8_loaf */
#define SKIT_T_ELEM_TYPE uint8_t
#define SKIT_T_PREFIX u8
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i32_slice and skit_i32_loaf */
#define SKIT_T_ELEM_TYPE int32_t
#define SKIT_T_PREFIX i32
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i16_slice and skit_i16_loaf */
#define SKIT_T_ELEM_TYPE int16_t
#define SKIT_T_PREFIX i16
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i8_slice and skit_i8_loaf */
#define SKIT_T_ELEM_TYPE int8_t
#define SKIT_T_PREFIX i8
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_double_slice and skit_double_loaf */
#define SKIT_T_ELEM_TYPE double
#define SKIT_T_PREFIX double
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_float_slice and skit_double_loaf */
#define SKIT_T_ELEM_TYPE float
#define SKIT_T_PREFIX float
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_cstr_slice and skit_cstr_loaf  (SKIT_T_ELEM_TYPE==char*) */
#define SKIT_T_ELEM_TYPE char*
#define SKIT_T_PREFIX cstr
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_slice_slice and skit_slice_loaf  (SKIT_T_ELEM_TYPE=slice) */
#define SKIT_T_ELEM_TYPE skit_slice
#define SKIT_T_PREFIX slice
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_loaf_slice and skit_loaf_loaf  (SKIT_T_ELEM_TYPE=slice) */
#define SKIT_T_ELEM_TYPE skit_loaf
#define SKIT_T_PREFIX loaf
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/* Double guard this: stack_builtins.h will #undef the normal include guards
because it needs to expand this file twice: once for the definitions and again
for the implementations. */
#ifndef SKIT_STACK_UNITTEST_INCLUDED
#define SKIT_STACK_UNITTEST_INCLUDED
/** */
void skit_array_unittest();
#endif

#ifdef SKIT_CLEAN_ARRAY_BUILTINS
#	undef SKIT_T_HEADER
#	undef SKIT_CLEAN_ARRAY_BUILTINS
#endif

#endif
