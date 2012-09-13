
#ifndef SKIT_FEATURE_EMU_CT_ERRORS_INCLUDED
#define SKIT_FEATURE_EMU_CT_ERRORS_INCLUDED

#define SKIT_COMPILE_TIME_ERRORS_JOIN2(sym1, sym2) sym1##sym2
#define SKIT_COMPILE_TIME_ERRORS_JOIN(sym1, sym2) \
	SKIT_COMPILE_TIME_ERRORS_JOIN2(sym1, sym2)

/* These define messages that show at compile-time if the caller does something wrong. */
/* They are wrapped in macros to make it easier to alter the text. */
#define SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT \
	The_builtin_return_statement_cannot_be_used_between_SCOPE_and_END_SCOPE__Use_RETURN_instead
#define SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT,_)

#define SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT \
	The_RETURN_statement_macro_cannot_be_used_in_scope_guards
#define SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT,_)
	
#define SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT \
	goto_is_disallowed_from_scope_guards_because_leaving_SCOPE_guards_with_goto_can_be_disastrous
#define SKIT_NO_GOTO_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT,_)

#define SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_TXT \
	continue_is_disallowed_from_scope_guards_because_leaving_SCOPE_guards_with_continue_can_be_disastrous
#define SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_CONTINUE_FROM_SCOPE_GUARDS_TXT,_)

#define SKIT_NO_BREAK_FROM_SCOPE_GUARDS_TXT \
	break_is_disallowed_from_scope_guards_because_leaving_SCOPE_guards_with_break_can_be_disastrous
#define SKIT_NO_BREAK_FROM_SCOPE_GUARDS_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_BREAK_FROM_SCOPE_GUARDS_TXT,_)

#define SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT \
	The_builtin_return_statement_cannot_be_used_in_STRY_SCATCH_blocks__Use_RETURN_instead
#define SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT,_)

#define SKIT_NO_GOTO_FROM_TRY_TXT \
	goto_is_disallowed_from_STRY_SCATCH_blocks_because_leaving_STRY_SCATCH_blocks_with_goto_can_be_disastrous
#define SKIT_NO_GOTO_FROM_TRY_PTR \
	SKIT_COMPILE_TIME_ERRORS_JOIN(SKIT_NO_GOTO_FROM_TRY_TXT,_)

#define SKIT_RETURN_HAS_USE_TXT \
	This_RETURN_statement_macro_is_in_a_function_missing_a_USE_FEATURE_EMULATION_statement

#define SKIT_USE_FEATURES_IN_FUNC_BODY \
	Place_the_USE_FEATURES_macro_at_the_top_of_function_bodies_to_use_features_like_STRY_SCATCH_and_SCOPE
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
Define an alternative to return: RETURN.
This louder brethren will do the scope-scanning necessary to allow it to be
placed in SCOPE-END_SCOPE statements.
TODO: It should clean up STRY-SCATCH stacks too, so that it can be called
from sTRY-sCATCH blocks.
*/
#define SKIT_RETURN_INTERNAL(return_expr) \
	{ \
		(void)SKIT_RETURN_HAS_USE_TXT; \
		SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_TXT(SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR); \
		(void)*SKIT_NO_MACRO_RETURN_FROM_SCOPE_GUARDS_PTR; \
		\
		/* Redefine this (again) as a pointer so that the builtin return will work from here. */ \
		char *SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR = 0; \
		(void)SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_PTR; \
		\
		__SKIT_SCAN_SCOPE_GUARDS(SKIT_SCOPE_SUCCESS_EXIT); \
		return_expr; \
	}

#define RETURN0()     SKIT_RETURN_INTERNAL(return)
#define RETURN1(expr) SKIT_RETURN_INTERNAL(return (expr))
#define RETURN(...) MACRO_DISPATCHER1(RETURN, __VA_ARGS__)(__VA_ARGS__)
	

/* 
This macro is used internally by macros to simplify the creation of error 
messages that are printed at compile-time when the macros are used incorrectly.
*/
#define SKIT_COMPILE_TIME_CHECK(errtxt, initial_val) \
        char errtxt = initial_val; \
        (void)(errtxt); /* Silence misleading "warning: unused variable" messages from the compiler. */ \
        do {} while(0)

#endif
