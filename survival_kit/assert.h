
#ifndef SKIT_ASSERT_INCLUDED
#define SKIT_ASSERT_INCLUDED

#include <string.h>
#include <inttypes.h>
#include "survival_kit/misc.h" /* for skit_die */
#include "survival_kit/macro.h"
#include "survival_kit/feature_emulation/stack_trace.h"

#define SKIT__ENFORCE( check_name, val ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: " check_name "(" #val ") failed.", \
				__FILE__, __LINE__, __func__); \
		} \
	} while(0)
	
#define SKIT__ENFORCE_MSG( check_name, val, msg ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: " check_name "(" #val ") failed.\n" \
				"  Message: %s", \
				__FILE__, __LINE__, __func__, \
				(msg)); \
		} \
	} while(0)

#define SKIT__ENFORCE_MSGF( check_name, val, ... ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: " check_name "(" #val ") failed.\n" \
				"  Message: " SKIT_FIRST_VARG(__VA_ARGS__), \
				__FILE__, __LINE__, __func__, \
				__VA_ARGS__); \
		} \
	} while(0)

#define SKIT__ENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs ) \
	do { \
		if ( !(comparison_expr) ) { \
			skit_print_stack_trace(); \
			char fmtstr[] = \
				"%%s: at line %%d in function %%s: " check_name "(" #printf_arg_lhs "," #printf_arg_rhs ") failed.\n" \
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

#define SKIT__ENFORCE_EQ( check_name, lhs, rhs, fmt ) SKIT__ENFORCE_COMPLICATED(check_name "_EQ", (lhs) == (rhs), (fmt), (fmt), lhs, rhs)
#define SKIT__ENFORCE_NE( check_name, lhs, rhs, fmt ) SKIT__ENFORCE_COMPLICATED(check_name "_NE", (lhs) != (rhs), (fmt), (fmt), lhs, rhs)
#define SKIT__ENFORCE_GE( check_name, lhs, rhs, fmt ) SKIT__ENFORCE_COMPLICATED(check_name "_GE", (lhs) >= (rhs), (fmt), (fmt), lhs, rhs)
#define SKIT__ENFORCE_LE( check_name, lhs, rhs, fmt ) SKIT__ENFORCE_COMPLICATED(check_name "_LE", (lhs) <= (rhs), (fmt), (fmt), lhs, rhs)
#define SKIT__ENFORCE_GT( check_name, lhs, rhs, fmt ) SKIT__ENFORCE_COMPLICATED(check_name "_GT", (lhs) >  (rhs),  (fmt), (fmt), lhs, rhs)
#define SKIT__ENFORCE_LT( check_name, lhs, rhs, fmt ) SKIT__ENFORCE_COMPLICATED(check_name "_LT", (lhs) <  (rhs),  (fmt), (fmt), lhs, rhs)

#define SKIT__ENFORCE_EQ_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_EQ_CSTR", strcmp(lhs,rhs) == 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_NE_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_NE_CSTR", strcmp(lhs,rhs) != 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_GE_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_GE_CSTR", strcmp(lhs,rhs) >= 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_LE_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_LE_CSTR", strcmp(lhs,rhs) <= 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_GT_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_GT_CSTR", strcmp(lhs,rhs) > 0,  "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_LT_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_LT_CSTR", strcmp(lhs,rhs) < 0,  "'%s'", "'%s'", lhs, rhs)

/* ------------------------------------------------------------------------- */

#define sENFORCE( val)               SKIT__ENFORCE( "sENFORCE", val )
#define sENFORCE_MSG( val, msg )     SKIT__ENFORCE_MSG( "sENFORCE_MSG", val, msg )
#define sENFORCE_MSGF( val, ... )    SKIT__ENFORCE_MSGF( "sENFORCE_MSGF", val, __VA_ARGS__ )
#define sENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs ) \
	SKIT__ENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs )

#define sENFORCE_EQ( lhs, rhs, fmt ) SKIT__ENFORCE_EQ( "sENFORCE", lhs, rhs, fmt )
#define sENFORCE_NE( lhs, rhs, fmt ) SKIT__ENFORCE_NE( "sENFORCE", lhs, rhs, fmt )
#define sENFORCE_GE( lhs, rhs, fmt ) SKIT__ENFORCE_GE( "sENFORCE", lhs, rhs, fmt )
#define sENFORCE_LE( lhs, rhs, fmt ) SKIT__ENFORCE_LE( "sENFORCE", lhs, rhs, fmt )
#define sENFORCE_GT( lhs, rhs, fmt ) SKIT__ENFORCE_GT( "sENFORCE", lhs, rhs, fmt )
#define sENFORCE_LT( lhs, rhs, fmt ) SKIT__ENFORCE_LT( "sENFORCE", lhs, rhs, fmt )	

#define sENFORCE_EQ_CSTR( lhs, rhs ) SKIT__ENFORCE_EQ_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_NE_CSTR( lhs, rhs ) SKIT__ENFORCE_NE_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_GE_CSTR( lhs, rhs ) SKIT__ENFORCE_GE_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_LE_CSTR( lhs, rhs ) SKIT__ENFORCE_LE_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_GT_CSTR( lhs, rhs ) SKIT__ENFORCE_GT_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_LT_CSTR( lhs, rhs ) SKIT__ENFORCE_LT_CSTR( "sENFORCE", lhs, rhs )

/* ------------------------------------------------------------------------- */

#define sASSERT( val)                SKIT__ENFORCE( "sASSERT", val )
#define sASSERT_MSG( val, msg )      SKIT__ENFORCE_MSG( "sASSERT_MSG", val, msg )
#define sASSERT_MSGF( val, ... )     SKIT__ENFORCE_MSGF( "sASSERT_MSGF", val, __VA_ARGS__ )
#define sASSERT_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs ) \
	SKIT__ENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs )

#define sASSERT_EQ( lhs, rhs, fmt ) SKIT__ENFORCE_EQ( "sASSERT", lhs, rhs, fmt )
#define sASSERT_NE( lhs, rhs, fmt ) SKIT__ENFORCE_NE( "sASSERT", lhs, rhs, fmt )
#define sASSERT_GE( lhs, rhs, fmt ) SKIT__ENFORCE_GE( "sASSERT", lhs, rhs, fmt )
#define sASSERT_LE( lhs, rhs, fmt ) SKIT__ENFORCE_LE( "sASSERT", lhs, rhs, fmt )
#define sASSERT_GT( lhs, rhs, fmt ) SKIT__ENFORCE_GT( "sASSERT", lhs, rhs, fmt )
#define sASSERT_LT( lhs, rhs, fmt ) SKIT__ENFORCE_LT( "sASSERT", lhs, rhs, fmt )	

#define sASSERT_EQ_CSTR( lhs, rhs ) SKIT__ENFORCE_EQ_CSTR( "sASSERT", lhs, rhs )
#define sASSERT_NE_CSTR( lhs, rhs ) SKIT__ENFORCE_NE_CSTR( "sASSERT", lhs, rhs )
#define sASSERT_GE_CSTR( lhs, rhs ) SKIT__ENFORCE_GE_CSTR( "sASSERT", lhs, rhs )
#define sASSERT_LE_CSTR( lhs, rhs ) SKIT__ENFORCE_LE_CSTR( "sASSERT", lhs, rhs )
#define sASSERT_GT_CSTR( lhs, rhs ) SKIT__ENFORCE_GT_CSTR( "sASSERT", lhs, rhs )
#define sASSERT_LT_CSTR( lhs, rhs ) SKIT__ENFORCE_LT_CSTR( "sASSERT", lhs, rhs )


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
