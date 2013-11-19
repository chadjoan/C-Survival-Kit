
#ifndef SKIT_ASSERT_INCLUDED
#define SKIT_ASSERT_INCLUDED

#include <string.h>
#include <inttypes.h>
#include "survival_kit/misc.h" /* for skit_die */
#include "survival_kit/macro.h"
#include "survival_kit/portable_ints.h"
#include "survival_kit/feature_emulation/stack_trace.h"

/* ------------------------------------------------------------------------- */
// Single-expression assertions/enforcers.

/* Internal use.  Please do not call directly. */
#define SKIT__ENFORCE( check_name, val ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: " check_name "(" #val ") failed.", \
				__FILE__, __LINE__, __func__); \
		} \
	} while(0)

/* Internal use.  Please do not call directly. */
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

/* Internal use.  Please do not call directly. */
#define SKIT__ENFORCE_MSGF( check_name, val, ... ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: " check_name "(" #val ") failed.\n" \
				"  Message: " SKIT_FIRST_VARG(__VA_ARGS__), \
				__FILE__, __LINE__, __func__, \
				SKIT_REST_VARGS(__VA_ARGS__)); \
		} \
	} while(0)

/// Single-expression enforcers.
#define sENFORCE( val)               SKIT__ENFORCE( "sENFORCE", val )
#define sENFORCE_MSG( val, msg )     SKIT__ENFORCE_MSG( "sENFORCE_MSG", val, msg )
#define sENFORCE_MSGF( val, ... )    SKIT__ENFORCE_MSGF( "sENFORCE_MSGF", val, __VA_ARGS__ )

/// Single-expression assertions.
#define sASSERT( val)                SKIT__ENFORCE( "sASSERT", val )
#define sASSERT_MSG( val, msg )      SKIT__ENFORCE_MSG( "sASSERT_MSG", val, msg )
#define sASSERT_MSGF( val, ... )     SKIT__ENFORCE_MSGF( "sASSERT_MSGF", val, __VA_ARGS__ )

/* ------------------------------------------------------------------------- */
// Common code for handling those pesky integer/pointer comparisons.

/* Internal use.  Please do not call directly. */
#define SKIT__ASSERT_INT_TO_STR(hexfmt, in_val, in_buf, out_len) \
	do { \
		if ( ((__typeof__((in_val)))-1) > ((__typeof__((in_val)))0) ) /* unsigned integers */ \
			out_len = snprintf(in_buf, sizeof(in_buf), hexfmt ? skit_uintmax_hexfmt : skit_uintmax_fmt, (skit_uintmax) (in_val)); \
		else /* signed integers */ \
			out_len = snprintf(in_buf, sizeof(in_buf), hexfmt ? skit_intmax_hexfmt  : skit_intmax_fmt,  (skit_intmax)  (in_val)); \
	} while (0)

/* Internal use.  Please do not call directly. */
#define SKIT__CHECK_NUM_CMP_STRINGIZE( cmp_op, hexfmt, check_name, lhs, rhs, lhs_evaluated, rhs_evaluated, RESPONSE ) \
	do { \
		/* These buffers should handle up to 128-bit integers.  Good enough, I hope! */ \
		char sASSERT__lhs_buf[40]; \
		char sASSERT__rhs_buf[40]; \
		int sASSERT__lhs_len = 0; \
		int sASSERT__rhs_len = 0; \
		SKIT__ASSERT_INT_TO_STR(hexfmt, lhs_evaluated, sASSERT__lhs_buf, sASSERT__lhs_len); \
		SKIT__ASSERT_INT_TO_STR(hexfmt, rhs_evaluated, sASSERT__rhs_buf, sASSERT__rhs_len); \
		\
		RESPONSE(cmp_op, check_name, lhs, rhs, \
			sASSERT__lhs_buf, sASSERT__rhs_buf, \
			sASSERT__lhs_len, sASSERT__rhs_len); \
	} while(0)

/* Internal use.  Please do not call directly. */
#define SKIT__CHECK_NUM_CMP_EVAL( cmp_op, hexfmt, check_name, upcased_suffix, lhs, rhs, RESPONSE ) \
	do { \
		/* Portability: Compilers that don't support __typeof__ will need an alternative path. */ \
		/* __typeof__ allows us to evaluate each expression only once and */ \
		/* store it in a way that will compare identically to the original */ \
		/* expression.  We then test for signedness and cast into the largest */ \
		/* integer type and format it as that.  This allows the caller to */ \
		/* not have to worry about telling the assertion what format string */ \
		/* to use to print the components of the expression. */ \
		__typeof__((lhs)) sASSERT__lhs_evaluated = (lhs); \
		__typeof__((rhs)) sASSERT__rhs_evaluated = (rhs); \
		\
		if ( !((sASSERT__lhs_evaluated) cmp_op (sASSERT__rhs_evaluated)) ) \
			SKIT__CHECK_NUM_CMP_STRINGIZE( cmp_op, hexfmt, check_name "_" #upcased_suffix, lhs, rhs, \
				sASSERT__lhs_evaluated, sASSERT__rhs_evaluated, RESPONSE ); \
	} while(0)

/* Internal use.  Please do not call directly. */
#define SKIT__ENFORCE_NUM_CMP_RE(cmp_op, hexfmt, upcased_suffix, lhs, rhs, RESPONSE) \
	SKIT__CHECK_NUM_CMP_EVAL(cmp_op, hexfmt, "ENFORCE", upcased_suffix, lhs, rhs, RESPONSE)

#define SKIT__ASSERT_NUM_CMP_RE(cmp_op, hexfmt, upcased_suffix, lhs, rhs, RESPONSE) \
	SKIT__CHECK_NUM_CMP_EVAL(cmp_op, hexfmt, "ASSERT", upcased_suffix, lhs, rhs, RESPONSE)

/**
Assertions/enforcement with caller-defined responses to test failure.

This exists to allow other libraries or utility code to build their own
assertions or enforcements ontop of the survival kit string.h asserts/enforces.

Appropriate responses are a macro with a signature like so:
#define RESPONSE(cmp_op, check_name, lhs, rhs, lhs_strptr, rhs_strptr, lhs_len, rhs_len)
cmp_op:     Usually expands to one of ==, !=, >=, <=, >, or <.  This will be a raw C token (unquoted).
check_name: Usually expands to "ASSERT_EQ", "ENFORCE_EQ", or similar, depending on origins.
lhs:        The original left-hand-side expression.  It is provided for stringizing; do not evaluate it.
rhs:        The original right-hand-side expression.  It is provided for stringizing; do not evaluate it.
lhs_strptr: A pointer derived from a slice whose value is the result of evaluating the left-hand-side of the original assert/enforce.
rhs_strptr: A pointer derived from a slice whose value is the result of evaluating the right-hand-side of the original assert/enforce.
lhs_len:    The length of the string (in bytes) pointed to by lhs_strptr.
rhs_len:    The length of the string (in bytes) pointed to by rhs_strptr.

The RESPONSE macro should expand as a statement, so wrap it in a 'do {...} while(0)' as necessary.
*/
#define sENFORCE_EQ_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(==, 0, EQ,lhs,rhs,RESPONSE)
#define sENFORCE_NE_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(!=, 0, NE,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_GE_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(>=, 0, GE,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_LE_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(<=, 0, LE,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_GT_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(>,  0, GT,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_LT_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(<,  0, LT,lhs,rhs,RESPONSE) /// Ditto.

#define sENFORCE_EQ_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(==, 1, EQ,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_NE_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(!=, 1, NE,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_GE_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(>=, 1, GE,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_LE_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(<=, 1, LE,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_GT_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(>,  1, GT,lhs,rhs,RESPONSE) /// Ditto.
#define sENFORCE_LT_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ENFORCE_NUM_CMP_RE(<,  1, LT,lhs,rhs,RESPONSE) /// Ditto.

#define sASSERT_EQ_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(==, 0, EQ,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_NE_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(!=, 0, NE,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_GE_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(>=, 0, GE,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_LE_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(<=, 0, LE,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_GT_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(>,  0, GT,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_LT_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(<,  0, LT,lhs,rhs,RESPONSE) /// Ditto.

#define sASSERT_EQ_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(==, 1, EQ,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_NE_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(!=, 1, NE,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_GE_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(>=, 1, GE,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_LE_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(<=, 1, LE,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_GT_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(>,  1, GT,lhs,rhs,RESPONSE) /// Ditto.
#define sASSERT_LT_HEX_RE( lhs, rhs, RESPONSE ) SKIT__ASSERT_NUM_CMP_RE(<,  1, LT,lhs,rhs,RESPONSE) /// Ditto.

/* Internal use.  Please do not call directly. */
#define SKIT__ASSERT_CMP_RESPONSE(cmp_op, check_name, lhs, rhs, lhs_strptr, rhs_strptr, lhs_len, rhs_len) \
	do { \
		skit_print_stack_trace(); \
		skit_die ( \
			"%s: at line %d in function %s: " check_name "(" #lhs " " #cmp_op " " #rhs ") failed.\n" \
			"  lhs == \"%.*s\"\n" \
			"  rhs == \"%.*s\"", \
			__FILE__, __LINE__, __func__, \
			lhs_len, lhs_strptr, \
			rhs_len, rhs_strptr); \
	} while(0)

#define SKIT__ENFORCE_CMP_RESPONSE(cmp_op, check_name, lhs, rhs, lhs_strptr, rhs_strptr, lhs_len, rhs_len) \
	do { \
		sTHROW( \
			check_name "(" #lhs " " #cmp_op " " #rhs ") failed.\n" \
			"  lhs == \"%.*s\"\n" \
			"  rhs == \"%.*s\"", \
			lhs_len, lhs_strptr, \
			rhs_len, rhs_strptr); \
	} while(0)

/**
Assertions/enforcement involving comparisons of integers.
These are good to use because they will print the numbers involved in the
comparison if anything goes wrong, thus aiding in fast debugging.

The _HEX variants will output the expected-vs-got values in hexadecimal
rather than decimal format.
*/
#define sENFORCE_EQ( lhs, rhs ) sENFORCE_EQ_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_NE( lhs, rhs ) sENFORCE_NE_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_GE( lhs, rhs ) sENFORCE_GE_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_LE( lhs, rhs ) sENFORCE_LE_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_GT( lhs, rhs ) sENFORCE_GT_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_LT( lhs, rhs ) sENFORCE_LT_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )

#define sENFORCE_EQ_HEX( lhs, rhs ) sENFORCE_EQ_HEX_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_NE_HEX( lhs, rhs ) sENFORCE_NE_HEX_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_GE_HEX( lhs, rhs ) sENFORCE_GE_HEX_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_LE_HEX( lhs, rhs ) sENFORCE_LE_HEX_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_GT_HEX( lhs, rhs ) sENFORCE_GT_HEX_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )
#define sENFORCE_LT_HEX( lhs, rhs ) sENFORCE_LT_HEX_RE( lhs, rhs, SKIT__ENFORCE_CMP_RESPONSE )

#define sASSERT_EQ( lhs, rhs ) sASSERT_EQ_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_NE( lhs, rhs ) sASSERT_NE_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_GE( lhs, rhs ) sASSERT_GE_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_LE( lhs, rhs ) sASSERT_LE_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_GT( lhs, rhs ) sASSERT_GT_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_LT( lhs, rhs ) sASSERT_LT_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )

#define sASSERT_EQ_HEX( lhs, rhs ) sASSERT_EQ_HEX_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_NE_HEX( lhs, rhs ) sASSERT_NE_HEX_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_GE_HEX( lhs, rhs ) sASSERT_GE_HEX_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_LE_HEX( lhs, rhs ) sASSERT_LE_HEX_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_GT_HEX( lhs, rhs ) sASSERT_GT_HEX_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )
#define sASSERT_LT_HEX( lhs, rhs ) sASSERT_LT_HEX_RE( lhs, rhs, SKIT__ASSERT_CMP_RESPONSE )

/* ------------------------------------------------------------------------- */
// C-string stuff.
// TODO: upgrade this to have pluggable responses.
// TODO: Better allocation of format destinations.

/* Internal use.  Please do not call directly. */
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

/// deprecated
#define sENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs ) \
	SKIT__ENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs )

/// deprecated
#define sASSERT_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs ) \
	SKIT__ENFORCE_COMPLICATED( check_name, comparison_expr, printf_fmt_lhs, printf_fmt_rhs, printf_arg_lhs, printf_arg_rhs )

#define SKIT__ENFORCE_EQ_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_EQ_CSTR", strcmp(lhs,rhs) == 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_NE_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_NE_CSTR", strcmp(lhs,rhs) != 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_GE_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_GE_CSTR", strcmp(lhs,rhs) >= 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_LE_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_LE_CSTR", strcmp(lhs,rhs) <= 0, "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_GT_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_GT_CSTR", strcmp(lhs,rhs) > 0,  "'%s'", "'%s'", lhs, rhs)
#define SKIT__ENFORCE_LT_CSTR( check_name, lhs, rhs ) SKIT__ENFORCE_COMPLICATED(check_name "_LT_CSTR", strcmp(lhs,rhs) < 0,  "'%s'", "'%s'", lhs, rhs)

#define sENFORCE_EQ_CSTR( lhs, rhs ) SKIT__ENFORCE_EQ_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_NE_CSTR( lhs, rhs ) SKIT__ENFORCE_NE_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_GE_CSTR( lhs, rhs ) SKIT__ENFORCE_GE_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_LE_CSTR( lhs, rhs ) SKIT__ENFORCE_LE_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_GT_CSTR( lhs, rhs ) SKIT__ENFORCE_GT_CSTR( "sENFORCE", lhs, rhs )
#define sENFORCE_LT_CSTR( lhs, rhs ) SKIT__ENFORCE_LT_CSTR( "sENFORCE", lhs, rhs )

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
