
#ifndef SKIT_ASSERT_INCLUDED
#define SKIT_ASSERT_INCLUDED

#include <string.h>
#include <inttypes.h>
#include "survival_kit/misc.h" /* for skit_die */
#include "survival_kit/feature_emulation/stack_trace.h"

#define sASSERT( val ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: sASSERT(" #val ") failed.", \
				__FILE__, __LINE__, __func__); \
		} \
	} while(0)

/* TODO: This should be variadic. */
#define sASSERT_MSG( val, msg ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: sASSERT(" #val ") failed.\n" \
				"  Message: %s", \
				__FILE__, __LINE__, __func__, \
				(msg)); \
		} \
	} while(0)

#define sASSERT_COMPLICATED( assert_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs ) \
	do { \
		if ( !(comparison_expr) ) { \
			skit_print_stack_trace(); \
			char fmtstr[] = \
				"%%s: at line %%d in function %%s: " assert_name "(" #printf_arg_lhs "," #printf_arg_rhs ") failed.\n" \
				"  lhs == %s\n" \
				"  rhs == %s"; \
			char fmtbuf[sizeof(fmtstr)+64]; \
			snprintf(fmtbuf, sizeof(fmtbuf), fmtstr, (printf_fmt_lhs), (printf_fmt_rhs)); \
			skit_die ( fmtbuf, \
				__FILE__, __LINE__, __func__, \
				(printf_arg_lhs), \
				(printf_arg_rhs)); \
		} \
	} while(0)

#define sASSERT_EQ( lhs, rhs, fmt ) sASSERT_COMPLICATED("sASSERT_EQ", (lhs) == (rhs), (fmt), (fmt), lhs, rhs)
#define sASSERT_NE( lhs, rhs, fmt ) sASSERT_COMPLICATED("sASSERT_NE", (lhs) != (rhs), (fmt), (fmt), lhs, rhs)
#define sASSERT_GE( lhs, rhs, fmt ) sASSERT_COMPLICATED("sASSERT_GE", (lhs) >= (rhs), (fmt), (fmt), lhs, rhs)
#define sASSERT_LE( lhs, rhs, fmt ) sASSERT_COMPLICATED("sASSERT_LE", (lhs) <= (rhs), (fmt), (fmt), lhs, rhs)
#define sASSERT_GT( lhs, rhs, fmt ) sASSERT_COMPLICATED("sASSERT_GT", (lhs) >  (rhs),  (fmt), (fmt), lhs, rhs)
#define sASSERT_LT( lhs, rhs, fmt ) sASSERT_COMPLICATED("sASSERT_LT", (lhs) <  (rhs),  (fmt), (fmt), lhs, rhs)

#define sASSERT_EQ_CSTR( lhs, rhs ) sASSERT_COMPLICATED("sASSERT_EQ_CSTR", strcmp(lhs,rhs) == 0, "'%s'", "'%s'", lhs, rhs)
#define sASSERT_NE_CSTR( lhs, rhs ) sASSERT_COMPLICATED("sASSERT_NE_CSTR", strcmp(lhs,rhs) != 0, "'%s'", "'%s'", lhs, rhs)
#define sASSERT_GE_CSTR( lhs, rhs ) sASSERT_COMPLICATED("sASSERT_GE_CSTR", strcmp(lhs,rhs) >= 0, "'%s'", "'%s'", lhs, rhs)
#define sASSERT_LE_CSTR( lhs, rhs ) sASSERT_COMPLICATED("sASSERT_LE_CSTR", strcmp(lhs,rhs) <= 0, "'%s'", "'%s'", lhs, rhs)
#define sASSERT_GT_CSTR( lhs, rhs ) sASSERT_COMPLICATED("sASSERT_GT_CSTR", strcmp(lhs,rhs) > 0,  "'%s'", "'%s'", lhs, rhs)
#define sASSERT_LT_CSTR( lhs, rhs ) sASSERT_COMPLICATED("sASSERT_LT_CSTR", strcmp(lhs,rhs) < 0,  "'%s'", "'%s'", lhs, rhs)



	
/* Use this one if you know there won't be valid debug info.
   It must also be used by any functions called from assertion macros 
     (to prevent infinite recursion). */
#define sASSERT_NO_TRACE( val ) \
	do { \
		if ( !(val) ) skit_die( \
			"%s: at line %d in function %s: sASSERT(" #val ") failed.", \
			__FILE__, __LINE__, __func__); \
	} while(0)


#endif
