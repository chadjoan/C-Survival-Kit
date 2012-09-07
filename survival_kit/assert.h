
#ifndef SKIT_ASSERT_INCLUDED
#define SKIT_ASSERT_INCLUDED

#include "survival_kit/misc.h" /* for skit_die */

#define SKIT_ASSERT( val ) \
	do { \
		if ( !(val) ) skit_die( \
			"%s: at line %d in function %s: ASSERT(" #val ") failed.", \
			__FILE__, __LINE__, __func__); \
	} while(0)
	
#define SKIT_ASSERT_MSG( val, msg ) \
	do { \
		if ( !(val) ) skit_die( \
			"%s: at line %d in function %s: ASSERT(" #val ") failed.\n" \
			"  Message: %s", \
			__FILE__, __LINE__, __func__, \
			(msg)); \
	} while(0)

#define SKIT_ASSERT_FMT( val, val_str ) \
	do { \
		if ( !(val) ) skit_die( \
			"%s: at line %d in function %s: ASSERT(" #val ") failed.\n" \
			"  val == %s", \
			__FILE__, __LINE__, __func__, \
			(val_str)); \
	} while(0)

#define SKIT_ASSERT_EQ( lhs, rhs, lhs_str, rhs_str ) \
	do { \
		if ( (lhs) != (rhs) ) skit_die( \
			"%s: at line %d in function %s: ASSERT_EQ(" #lhs "," #rhs ") failed.\n" \
			"  lhs == %s\n" \
			"  rhs == %s", \
			__FILE__, __LINE__, __func__, \
			(lhs_str), \
			(rhs_str)); \
	} while(0)

#define SKIT_ASSERT_NEQ( lhs, rhs, lhs_str, rhs_str ) \
	do { \
		if ( (lhs) == (rhs) ) skit_die( \
			"%s: at line %d in function %s: ASSERT_NEQ(" #lhs "," #rhs ") failed.\n" \
			"  lhs == %s\n" \
			"  rhs == %s", \
			__FILE__, __LINE__, __func__, \
			(lhs_str), \
			(rhs_str)); \
	} while(0)

#endif
