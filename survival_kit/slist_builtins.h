
#ifndef SLIST_BUILTINS_INCLUDED
#define SLIST_BUILTINS_INCLUDED

#include <inttypes.h>

/* SKIT_T_HEADER allows the template header file to be overridden. */
/* It is used by the corresponding _builtins.c file to provide linkable */
/*   implementations for all of these template instances. */
/* It is not recommended that any calling code use SKIT_T_HEADER. */
#ifndef SKIT_T_HEADER
#	define SKIT_T_HEADER "survival_kit/templates/slist.h"
#	define SKIT_CLEAN_SLIST_BUILTIN 1
#endif

/** Define skit_u32_slist */
#define SKIT_T_ELEM_TYPE uint32_t
#define SKIT_T_PREFIX u32
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_u16_slist */
#define SKIT_T_ELEM_TYPE uint16_t
#define SKIT_T_PREFIX u16
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_u8_slist */
#define SKIT_T_ELEM_TYPE uint8_t
#define SKIT_T_PREFIX u8
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i32_slist */
#define SKIT_T_ELEM_TYPE int32_t
#define SKIT_T_PREFIX i32
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i16_slist */
#define SKIT_T_ELEM_TYPE int16_t
#define SKIT_T_PREFIX i16
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_i8_slist */
#define SKIT_T_ELEM_TYPE int8_t
#define SKIT_T_PREFIX i8
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_double_slist */
#define SKIT_T_ELEM_TYPE double
#define SKIT_T_PREFIX double
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_float_slist */
#define SKIT_T_ELEM_TYPE float
#define SKIT_T_PREFIX float
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** Define skit_cstring_slist  (SKIT_T_ELEM_TYPE==char*) */
#define SKIT_T_ELEM_TYPE char*
#define SKIT_T_PREFIX cstring
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/** */
void skit_slist_unittest();

#ifdef SKIT_CLEAN_SLIST_BUILTINS
#	undef SKIT_T_HEADER
#	undef SKIT_CLEAN_SLIST_BUILTINS
#endif

#endif