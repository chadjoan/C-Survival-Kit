
#ifndef SKIT_FEATURE_EMULATION_TYPES_INCLUDED
#define SKIT_FEATURE_EMULATION_TYPES_INCLUDED

#include <setjmp.h>

#include "survival_kit/feature_emulation/generated_exception_defs.h"

/* Implementation details. */
typedef struct skit_frame_info skit_frame_info;
struct skit_frame_info
{
	uint32_t line_number;
	const char *file_name;
	const char *func_name;
	/* jmp_buf *jmp_context; */
};

/** */
typedef struct skit_exception skit_exception;
struct skit_exception
{
	/* Implementation note: keep this small, it will be returned by-value from functions a lot. */
	skit_err_code error_code;  /** 0 should always mean "no error". TODO: Values of 0 don't make sense anymore.  It was useful for an inferior exceptions implementation.  Followup needed? */
	char *error_text;    /** READ ONLY: a description for the error. */
	skit_frame_info *frame_info; /** Points to the point in the frame info stack where the exception happened. */
};

/* 
Used internally to save the position of the call stack before CALLs and other
nested transfers.  The skit_thread_context_pos struct can then be used to
reconcile against the skit_thread_context later when the nested code completes.
The skit_reconcile_thread_context function is used for this.
*/
typedef struct skit_thread_context_pos skit_thread_context_pos; 
struct skit_thread_context_pos
{
	ssize_t try_jmp_pos;
	ssize_t exc_jmp_pos;
	ssize_t scope_jmp_pos;
	ssize_t debug_info_pos;
};

#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/stack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE jmp_buf
#define SKIT_T_PREFIX jmp
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_PREFIX debug
#include "survival_kit/templates/stack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_frame_info
#define SKIT_T_PREFIX debug
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_PREFIX exc
#include "survival_kit/templates/stack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_PREFIX exc
#include "survival_kit/templates/fstack.h"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX

/**
This structure contains thread-local data structures needed to implement the
emulation of various language features found in this module.
*/
typedef struct skit_thread_context skit_thread_context;
struct skit_thread_context
{
	skit_jmp_fstack   try_jmp_stack;
	skit_jmp_fstack   exc_jmp_stack;
	skit_jmp_fstack   scope_jmp_stack;
	skit_debug_fstack debug_info_stack;
	skit_exc_fstack   exc_instance_stack;
};

#endif