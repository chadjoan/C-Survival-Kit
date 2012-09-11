
#ifndef SKIT_FEATURE_EMU_CT_ERRORS_INCLUDED
#define SKIT_FEATURE_EMU_CT_ERRORS_INCLUDED

/* These define messages that show at compile-time if the caller does something wrong. */
/* They are wrapped in macros to make it easier to alter the text. */
#define SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT \
	The_builtin_return_statement_cannot_be_used_between_SCOPE_and_END_SCOPE__Use_RETURN_instead

#define SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT \
	goto_is_disallowed_from_scope_guards_because_leaving_SCOPE_guards_with_goto_can_be_disastrous

/* TODO: use me */
#define SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT \
	The_builtin_return_statement_cannot_be_used_in_TRY_CATCH_blocks__Use_RETURN_instead

/* TODO: use me */
#define SKIT_NO_GOTO_FROM_TRY_TXT \
	goto_is_disallowed_from_TRY_CATCH_blocks_because_leaving_TRY_CATCH_blocks_with_goto_can_be_disastrous

/* End of error message definitions. */

/* These have to be different variables so that the enclosing code can control
which of these gets redefined to non-pointers to prevent certain builtins
from compiling in the target blocks of code.
They must also be no longer than 31 characters in length because of certain
old linkers having a problem with that. */
extern char *SKIT_NO_RET_FROM_SCOPE_PTR;
extern char *SKIT_NO_RET_FROM_TRY_PTR;
extern char *SKIT_NO_GOTO_FROM_GUARD_PTR;
extern char *SKIT_NO_GOTO_FROM_TRY_PTR;

/* unused is used to shut the compiler up when this module is included but calls
to the error-catching functions never appear. */
#ifndef __GNUC__
#define __attribute__(arg)
#endif

__attribute__ ((unused)) static void SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT(char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT  (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT   (char *ptr) { }
__attribute__ ((unused)) static void SKIT_NO_GOTO_FROM_TRY_TXT            (char *ptr) { }


/* Try to catch some common mistakes by redefining keywords. */
#define return \
	while( \
	SKIT_NO_BUILTIN_RETURN_FROM_SCOPE_TXT(SKIT_NO_RET_FROM_SCOPE_PTR), \
	SKIT_NO_BUILTIN_RETURN_FROM_TRY_TXT  (SKIT_NO_RET_FROM_TRY_PTR), \
	(void)*SKIT_NO_RET_FROM_SCOPE_PTR, \
	(void)*SKIT_NO_RET_FROM_TRY_PTR, \
	1) \
		return

#define goto \
	while( \
	SKIT_NO_GOTO_FROM_SCOPE_GUARDS_TXT(SKIT_NO_GOTO_FROM_GUARD_PTR), \
	SKIT_NO_GOTO_FROM_TRY_TXT         (SKIT_NO_GOTO_FROM_TRY_PTR), \
	(void)*SKIT_NO_GOTO_FROM_GUARD_PTR, \
	(void)*SKIT_NO_GOTO_FROM_TRY_PTR, \
	1) \
		goto

/* 
Used internally by macros to simplify the creation of error messages that are
printed at compile-time when the macros are used incorrectly.
*/
#define SKIT_COMPILE_TIME_CHECK(errtxt, initial_val) \
        char errtxt = initial_val; \
        (void)(errtxt); /* Silence misleading "warning: unused variable" messages from the compiler. */ \
        do {} while(0)

#endif
