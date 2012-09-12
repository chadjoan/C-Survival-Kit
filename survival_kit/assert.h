
#ifndef SKIT_ASSERT_INCLUDED
#define SKIT_ASSERT_INCLUDED

#include <inttypes.h>
#include "survival_kit/misc.h" /* for skit_die */

/* 
skit_print_stack_trace() is defined by "survival_kit/feature_emulation/funcs.h"
These are duplicate definitions made to prevent macro recursion.
Please keep them in sync.
*/
#define skit_print_stack_trace() skit_print_stack_trace_func(__LINE__,__FILE__,__func__)
void skit_print_stack_trace_func( uint32_t line, const char *file, const char *func );

#define SKIT_ASSERT( val ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT(" #val ") failed.", \
				__FILE__, __LINE__, __func__); \
		} \
	} while(0)
	
/* Use this one if you know there won't be valid debug info.
   It must also be used by any functions called from assertion macros 
     (to prevent infinite recursion). */
#define SKIT_ASSERT_NO_TRACE( val ) \
	do { \
		if ( !(val) ) skit_die( \
			"%s: at line %d in function %s: ASSERT(" #val ") failed.", \
			__FILE__, __LINE__, __func__); \
	} while(0)
	
#define SKIT_ASSERT_MSG( val, msg ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT(" #val ") failed.\n" \
				"  Message: %s", \
				__FILE__, __LINE__, __func__, \
				(msg)); \
		} \
	} while(0)

#define SKIT_ASSERT_FMT( val, val_str ) \
	do { \
		if ( !(val) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT(" #val ") failed.\n" \
				"  val == %s", \
				__FILE__, __LINE__, __func__, \
				(val_str)); \
		} \
	} while(0)

#define SKIT_ASSERT_EQS( lhs, rhs ) \
	do { \
		if ( (lhs) != (rhs) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT_EQS(" #lhs "," #rhs ") failed.\n" \
				"  lhs == %s\n" \
				"  rhs == %s", \
				__FILE__, __LINE__, __func__, \
				(lhs), \
				(rhs)); \
		} \
	} while(0)

#define SKIT_ASSERT_NEQS( lhs, rhs ) \
	do { \
		if ( (lhs) == (rhs) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT_NEQS(" #lhs "," #rhs ") failed.\n" \
				"  lhs == %s\n" \
				"  rhs == %s", \
				__FILE__, __LINE__, __func__, \
				(lhs), \
				(rhs)); \
		} \
	} while(0)

#define SKIT_ASSERT_EQ( lhs, rhs, fmt ) \
	do { \
		if ( (lhs) != (rhs) ) { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT_EQ(" #lhs "," #rhs ") failed.\n" \
				"  lhs == " fmt "\n" \
				"  rhs == " fmt, \
				__FILE__, __LINE__, __func__, \
				(lhs), \
				(rhs)); \
		} \
	} while(0)

#define SKIT_ASSERT_NEQ( lhs, rhs, fmt ) \
	do { \
		if ( (lhs) == (rhs) )  { \
			skit_print_stack_trace(); \
			skit_die( \
				"%s: at line %d in function %s: ASSERT_NEQ(" #lhs "," #rhs ") failed.\n" \
				"  lhs == " fmt "\n" \
				"  rhs == " fmt, \
				__FILE__, __LINE__, __func__, \
				(lhs), \
				(rhs)); \
		} \
	} while(0)

#endif