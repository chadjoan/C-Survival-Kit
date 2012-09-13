
/** 
These macros are a usable implementation of try-catch exception handling in C.

They are named STRY, SCATCH, and END_STRY instead of the normal names because
other libraries (and the OpenVMS runtime in particular) may also define macros
with names like TRY, CATCH, ENDTRY, or END_TRY.  

By default, including this module will undefine any previously existing 
TRY-CATCH macros.  This is done to prevent dangerous typos.  If you wish to
be able to use other TRY-CATCH implementations, then #define the 
SKIT_ALLOW_OTHER_TRY_CATCH macro symbol before #including this file.
TRY-CATCH implementations from other parties will NOT communicate with this
one, so any exceptions thrown in those cannot be caught by STRY-SCATCH, and
vica-versa.  Furthermore, throwing exceptions from a different exceptions
implementation may cause very nasty runtime behavior and memory corruption
because it could easily unbalance the tracking of setjmp/longjmp calls
within the STHROW-STRY-SCATCH chain.

NOTE: Do not attempt to branch out of a STRY-SCATCH block.  Example:
    int foo, len;
    len = 10;
    while ( foo < len )
    {
        STRY
            foo++;
            if ( foo > len )
                break;  // The stack now permanently contains a reference
                        //   to this STRY statement.  Any END_STRY's
                        //   encountered in calling code might actually
                        //   end up at the STRY above.
            else
                continue; // ditto
        END_STRY
    }

DO NOT DO THE ABOVE ^^.
The "break" and "continue" statements will leave the STRY-END_STRY block
without restoring stack information to the expected values.  

TODO: It'd be nice if there was some way to make the compiler forbid
such local jumps or maybe even make them work in sensible ways.
(Later note: it's possible, see compile_time_errors.h.  It has the
undesirable side-effect of forbiding ALL local jumps in the try-catch,
not just the ones that escape.  It's used for return/goto though.)

TODO: what if an exception is raised from within a SCATCH block? (untested!)

TODO: SFINALLY
*/

#ifndef SKIT_TRY_CATCH_INCLUDED
#define SKIT_TRY_CATCH_INCLUDED

#include <stdio.h> /* Incase printf debugging happens. */
#include <unistd.h> /* For ssize_t */
#include <inttypes.h>
#include <setjmp.h>
#include <limits.h> /* For INT_MIN and the like. */

#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/throw.h"
	
/* __SKIT_TRY_SAFE_EXIT is an implementation detail.
It allows execution to exit an STRY-SCATCH block by jumping to a state distinct
from the dangerous ways of exiting a STRY-SCATCH block.  The "safe" way is
for execution to fall to the bottom of a given block in the STRY-SCATCH
statement.  The "dangerous" was are by using local jumping constructs like
"break" and "continue" to leave a STRY-SCATCH statement.  
*/
#define __SKIT_TRY_SAFE_EXIT         INT_MIN+1  

/* __SKIT_TRY_EXCEPTION_CLEANUP is an implementation detail.
It is jumped to at the end of SCATCH blocks as a way of cleaning up the
exception allocated in the code that threw the exception.
*/
#define __SKIT_TRY_EXCEPTION_CLEANUP INT_MIN

/* TODO: Maybe we should use different macro names since OpenVMS DECC already has a kind of TRY-CATCH structure. */
#if defined(__DECC) && !defined(SKIT_ALLOW_OTHER_TRY_CATCH)
#undef TRY
#undef CATCH
#undef ENDTRY
#endif

/** */
#define STRY /* */ \
	{ \
	SKIT_FEATURE_TRACE("%s, %d.236: STRY.start\n", __FILE__, __LINE__); \
	SKIT_FEATURE_TRACE("type_jmp_stack_alloc\n"); \
	SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
	(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
	if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->try_jmp_stack,&skit_malloc)) != __SKIT_TRY_SAFE_EXIT ) { \
		SKIT_COMPILE_TIME_CHECK(SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR,0); \
		SKIT_COMPILE_TIME_CHECK(SKIT_NO_GOTO_FROM_TRY_PTR,0); \
		SKIT_FEATURE_TRACE("%s, %d.236: STRY.if\n", __FILE__, __LINE__); \
		do { \
			SKIT_FEATURE_TRACE("%s, %d.238: STRY.do\n", __FILE__, __LINE__); \
			SKIT_FEATURE_TRACE("exc_jmp_stack_alloc\n"); \
			switch( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack,&skit_malloc)) ) \
			{ \
			case __SKIT_TRY_EXCEPTION_CLEANUP: \
			{ \
				/* Exception cleanup case. */ \
				/* Note that this case is ALWAYS reached because SCATCH and END_STRY macros  */ \
				/*   may follow either the successful block or an exceptional one, so    */ \
				/*   there is no way to know from within this macro which one should be  */ \
				/*   jumped to.  The best strategy then is to always jump to the cleanup */ \
				/*   case after leaving any part of the STRY-SCATCH-END_STRY and only free   */ \
				/*   exceptions if there actually are any.                               */ \
				SKIT_FEATURE_TRACE("%s, %d.258: STRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
				while (skit_thread_ctx->exc_instance_stack.used.length > 0) \
				{ \
					skit_exc_fstack_pop(&skit_thread_ctx->exc_instance_stack); \
				} \
				SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
				skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
				SKIT_FEATURE_TRACE("try_jmp_stack_pop\n"); \
				longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack), __SKIT_TRY_SAFE_EXIT); \
			} \
			default: \
			{ \
				/* This is going to look a bit Duffy. ;)                           */ \
				/* It is fairly important, however, that we nest the 'case 0:'     */ \
				/* statement inside of an if statement that is otherwise never     */ \
				/* evaluated.  This strange configuration is driven by the need to */ \
				/* have the code at the end of each block (after macro expansion)  */ \
				/* look identical but do slightly different things.  The if(0)     */ \
				/* ensures that each end-of-block can assume that the previous     */ \
				/* block started as some form of if-statement.  The 0 in the       */ \
				/* if(0) makes sure that it never gets re-evaluated when           */ \
				/* exceptions are thrown: it must only be evaluated once when      */ \
				/* the STRY block is entered, and that causes setjmp to select      */ \
				/* the 0th case. Either way, the end-of-blocks will cleanup        */ \
				/* thrown exceptions /if they exist/ and otherwise exit cleanly.   */ \
				/* An early approach was to place everything inside a switch-case  */ \
				/* statement and never use if-else.  That configuration was        */ \
				/* incapable of handling exception inheritance because the cases   */ \
				/* in a switch-case can't be computed expressions.                 */ \
				if ( 0 ) \
				{ \
				case 0: \
					/* Normal/successful case. */ \
					SKIT_FEATURE_TRACE("%s, %d.282: STRY: case 0:\n", __FILE__, __LINE__); \

/** */
#define SCATCH(__error_code, exc_name) /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.288: STRY: case 0: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, __SKIT_TRY_EXCEPTION_CLEANUP); \
				} \
				else if ( exception_is_a( skit_thread_ctx->exc_instance_stack.used.front->val.error_code, __error_code) ) \
				{ \
					/* SCATCH block. */ \
					SKIT_FEATURE_TRACE("%s, %d.294: STRY: case %d:\n", __FILE__, __LINE__, __error_code); \
					skit_exception *exc_name = &skit_thread_ctx->exc_instance_stack.used.front->val; \
					/* The caller may not want to actually use the named exception. */ \
					/* They might just want to prevent its propogation and do different logic. */ \
					/* Thus we'll write (void)(exc_name) to prevent "warning: unused variable" messages. */ \
					(void)(exc_name);
					

/** */
#define END_STRY /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a SCATCH block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.301: STRY: case ??: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, __SKIT_TRY_EXCEPTION_CLEANUP); \
				} \
				else \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					SKIT_FEATURE_TRACE("%s, %d.307: STRY: default: longjmp\n", __FILE__, __LINE__); \
					SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
					SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
					__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
				} \
				SKIT_ASSERT(0); /* This should never be reached. The if-else chain above should handle all remaining cases. */ \
			} /* default: { } */ \
			} /* switch(setjmp(*__push...)) */ \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			SKIT_FEATURE_TRACE("%s, %d.319: STRY: break found!\n", __FILE__, __LINE__); \
			SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
			skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
			STHROW(BREAK_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'break' statement from within a STRY-SCATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" ); \
		} while (0); \
		/* If execution makes it here, then the caller */ \
		/* used a "continue" statement and is trying to */ \
		/* corrupt the debug stack.  Don't let them do it! */ \
		/* Instead, throw another exception. */ \
		SKIT_FEATURE_TRACE("%s, %d.331: STRY: continue found!\n", __FILE__, __LINE__); \
		SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
		SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
		skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
		STHROW(CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a STRY-SCATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n"); \
	} \
	SKIT_FEATURE_TRACE("%s, %d.339: STRY: done.\n", __FILE__, __LINE__); \
	}

#endif
