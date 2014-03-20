
#ifndef SKIT_FEATURE_EMU_CT_ERRORS_INCLUDED
#define SKIT_FEATURE_EMU_CT_ERRORS_INCLUDED

#define SKIT_COMPILE_TIME_ERRORS_JOIN2(sym1, sym2) sym1##sym2
#define SKIT_COMPILE_TIME_ERRORS_JOIN(sym1, sym2) \
	SKIT_COMPILE_TIME_ERRORS_JOIN2(sym1, sym2)

/* These define messages that show at compile-time if the caller does something wrong. */
/* They are wrapped in macros to make it easier to alter the text. */
#define SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT \
	The_builtin_return_statement_cannot_be_used_between_sSCOPE_and_sEND_SCOPE__Use_sRETURN_instead
#define SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT,_)

#define SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT \
	The_sRETURN_statement_macro_cannot_be_used_in_scope_guards
#define SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT,_)
	
#define SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT \
	goto_is_disallowed_from_scope_guards_because_leaving_sSCOPE_guards_with_goto_can_be_disastrous
#define SKIT_NO_GOTO_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT,_)

#define SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_TXT \
	continue_is_disallowed_from_scope_guards_because_leaving_sSCOPE_guards_with_continue_can_be_disastrous
#define SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_TXT,_)

#define SKIT_NO_BREAK_FROM_SCOPE_GUARDS_TXT \
	break_is_disallowed_from_scope_guards_because_leaving_SCOPE_guards_with_break_can_be_disastrous
#define SKIT_NO_BREAK_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_BREAK_FROM_SCOPE_GUARDS_TXT,_)

#define SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT \
	The_builtin_return_statement_cannot_be_used_in_sTRY_sCATCH_blocks__Use_sRETURN_instead
#define SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT,_)

#define SKIT_sENDTRY_IS_TYPO_TXT \
	sENDTRY_is_probably_a_typo__Did_you_mean_sEND_TRY
#define SKIT_sENDTRY_IS_TYPO_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_sENDTRY_IS_TYPO_TXT,_)

#define SKIT_NO_GOTO_FROM_TRY_TXT \
	goto_is_disallowed_from_sTRY_sCATCH_blocks_because_leaving_sTRY_sCATCH_blocks_with_goto_can_be_disastrous
#define SKIT_NO_GOTO_FROM_TRY_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_GOTO_FROM_TRY_TXT,_)

#define SKIT_RETURN_HAS_USE_TXT \
	This_sRETURN_statement_macro_is_in_a_function_missing_a_SKIT_USE_FEATURE_EMULATION_statement

#define SKIT_USE_FEATURES_IN_FUNC_BODY \
	Place_the_SKIT_USE_FEATURE_EMULATION_macro_at_the_top_of_function_bodies_to_use_features_like_sTRY_sCATCH_and_sSCOPE
/* End of error message definitions. */


/* unused is used to shut the compiler up when this module is included but calls
to the error-catching functions never appear. */
#ifndef __GNUC__
#define __attribute__(arg)
#endif

/* These have to be different variables so that the enclosing code can control
which of these gets redefined to non-pointers to prevent certain builtins
from compiling in the target blocks of code.
They must also be no longer than 31 characters in length because of certain
old linkers having a problem with that. */
__attribute__ ((unused)) static char *SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR;
__attribute__ ((unused)) static char *SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR;
__attribute__ ((unused)) static char *SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR;
__attribute__ ((unused)) static char *SKIT_NO_GOTO_FROM_SCOPE_GUARDS_PTR;
__attribute__ ((unused)) static char *SKIT_NO_GOTO_FROM_TRY_PTR;
__attribute__ ((unused)) static char *SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_PTR;
__attribute__ ((unused)) static char *SKIT_NO_BREAK_FROM_SCOPE_GUARDS_PTR;

__attribute__ ((unused)) static void SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT      (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT        (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT         (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_GOTO_FROM_TRY_TXT                  (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_TXT     (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_BREAK_FROM_SCOPE_GUARDS_TXT        (char *ptr) { }

/* Immediately define this one as non-pointer: it is always an error, regardless of where it appears. */
__attribute__ ((unused)) static char SKIT_sENDTRY_IS_TYPO_PTR;
__attribute__ ((unused)) static void SKIT_sENDTRY_IS_TYPO_TXT( char *ptr ) { }

/* Try to catch some common mistakes by redefining keywords. */

/* 
For return and goto, we nest them in a while statement.
This ensures that constructs like these will still work:
if ( foo )
	return 42;

It would be a mistake to have the macro expand like this:
if ( foo )
	SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR);
	return 42;

because the "return 42" would always be executed, even though the caller did not
intend it.

It is also undesirable to have the macro expand like this:
if ( foo )
	if ( 0 ) { SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR); }
	else return 42;

although this would technically preserve the code's semantics, it does create
unnecessary warnings with GCC.

Due to those limitations, we instead use a while statement and expand like so:
if ( foo )
	while ( SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR),1 )
		return 42;

The while loop is always entered because the comma expression evaluates to 1.
It is always exited immediately because both return and goto will jump execution
to another place.  It also doesn't print unnecessary warnings.  Perfect!
*/
#define return \
	while( \
	SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR), \
	SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT  (SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR), \
	(void)*SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR, \
	(void)*SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR, \
	1) \
		return

#define goto \
	while( \
	SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT(SKIT_NO_GOTO_FROM_SCOPE_GUARDS_PTR), \
	SKIT_NO_GOTO_FROM_TRY_TXT         (SKIT_NO_GOTO_FROM_TRY_PTR), \
	(void)*SKIT_NO_GOTO_FROM_SCOPE_GUARDS_PTR, \
	(void)*SKIT_NO_GOTO_FROM_TRY_PTR, \
	1) \
		goto

/* 
break/continue present a challenge because they won't jump execution outside
of the dummy-while-loop that was used for return/goto.
It's okay though, because fate delivered us an out:
C doesn't support labeled break/continue, so we can cheat and assume that
the macro has matched the entire statement in these cases.  It is now okay
to have a trailing } (and more) at the end.  
Now we can stick break/continue into an if/else and they will jump to where
the caller expects them to go.
We also put the if-else in a compound statement (add {} around it) to prevent
the "dangling else" warning that GCC gives, and also add a "do {} while(0)" 
at the end to make sure the caller still puts a semicolon after the 
break/continue statement.
*/
#define continue \
	{ if(0) { \
	SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_TXT(SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_PTR); \
	(void)*SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_PTR; \
	} \
	else { continue; }} \
	do {} while (0)

#define break \
	{ if(0) { \
	SKIT_NO_BREAK_FROM_SCOPE_GUARDS_TXT(SKIT_NO_BREAK_FROM_SCOPE_GUARDS_PTR); \
	(void)*SKIT_NO_BREAK_FROM_SCOPE_GUARDS_PTR; \
	} \
	else { break; }} \
	do {} while (0)

/* 
Define an alternative to return: sRETURN.
This louder brethren will do the scope-scanning necessary to allow it to be
placed in sSCOPE-sEND_SCOPE statements.
TODO: It should clean up sTRY-sCATCH stacks too, so that it can be called
from sTRY-sCATCH blocks.
*/
#define SKIT_RETURN_COMMON \
	(void)SKIT_RETURN_HAS_USE_TXT; \
	SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT(SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR); \
	(void)*SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR; \
	\
	/* Redefine this (again) as a pointer so that the builtin return will work from here. */ \
	char *SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR = 0; \
	(void)SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR;

#define SKIT_RETURN_INTERNAL0 \
	{ \
		SKIT_RETURN_COMMON \
		SKIT__SCAN_SCOPE_GUARDS(SKIT_SCOPE_SUCCESS_EXIT); \
		return; \
	}

// Note that we store the return value into a volatile variable before
// SKIT__SCAN_SCOPE_GUARDS.  This is because SKIT__SCAN_SCOPE_GUARDS calls
// longjmp, and setjmp/longjmp implementations tend to come with this caveat:
//
//   All accessible objects have values as of the time longjmp() was called, 
//   except that the values of objects of automatic storage duration which are
//   local to the function containing the invocation of the corresponding 
//   setjmp() which do not have volatile-qualified type and which are changed 
//   between the setjmp() invocation and longjmp() call are indeterminate. 
//
// (quote from: http://pubs.opengroup.org/onlinepubs/7908799/xsh/setjmp.html)
//
// Another page with warnings about this, and more importantly, also examples,
// is this one:
//   https://www.securecoding.cert.org/confluence/display/seccode/MSC22-C.+Use+the+setjmp%28%29,+longjmp%28%29+facility+securely
//
// Typically, any enregistered variable that was altered after setjmp will be
// clobbered after the call to longjmp.  This is because setjmp usually stores
// all of the registers' current values in an array (the jmp_buf), which are
// then restored when longjmp is called.  Any changes to those registers
// between the setjmp/longjmp calls are lost.
// 
// Storing 'expr' into a volatile variable prevents the caller from attempting
// to return a local variable that isn't volatile-qualified by forcing the
// return value to be volatile-qualified.  That should force the value to
// be written into memory or otherwise removed from the register bank, which
// /should/ make the value survive the subsequent longjmp.  Other values in
// the call stack have no consequence: they are going to fall out of scope
// anyways.
//
// The only doubt is this: examples that I (Chad Joan) have read so far tend to
// show volatile variables declared before setjmp is called, thus making all
// operations between setjmp/longjmp occur on a /volatile/ variable.  In this
// case the caller is potentially doing operations on a not-volatile-qualified
// variable between setjmp/longjmp, and then saving it into a volatile one at
// the last moment.  This could be slightly different, depending on how the
// above quote is understood by the C compilter's writer.  *Fingers crossed*
//
// This is tested by the skit_scope_vms_return_test() function in the
// feature emulation unittests (feature_emulation/unittests.c).
//
#define SKIT_RETURN_INTERNAL1(expr) \
	{ \
		SKIT_RETURN_COMMON \
		volatile __typeof__((expr)) SKIT__sRETURN_TMP = (expr); \
		SKIT__SCAN_SCOPE_GUARDS(SKIT_SCOPE_SUCCESS_EXIT); \
		return SKIT__sRETURN_TMP; \
	}

#define sRETURN0()     SKIT_RETURN_INTERNAL0
#define sRETURN1(expr) SKIT_RETURN_INTERNAL1(expr)
#define sRETURN(...) SKIT_MACRO_DISPATCHER1(sRETURN, __VA_ARGS__)(__VA_ARGS__)

/* This (slightly uglier) version is offered as a way to work around the
lack of an ability to determine whether a variadic macro was expanded with
1 argument or 0 arguments. */
#define sRETURN_       SKIT_RETURN_INTERNAL0

/* 
This macro is used internally by macros to simplify the creation of error 
messages that are printed at compile-time when the macros are used incorrectly.
*/
#define SKIT_COMPILE_TIME_CHECK(errtxt, initial_val) \
        char errtxt = initial_val; \
        (void)(errtxt); /* Silence misleading "warning: unused variable" messages from the compiler. */ \
        do {} while(0)

#endif
