
/** 
These macros are a usable implementation of try-catch exception handling in C.

They are named sTRY, sCATCH, and sEND_TRY instead of the normal names because
other libraries (and the OpenVMS runtime in particular) may also define macros
with names like TRY, CATCH, ENDTRY, or END_TRY.  

By default, including this module will undefine any previously existing 
TRY-CATCH macros.  This is done to prevent dangerous typos.  If you wish to
be able to use other TRY-CATCH implementations, then #define the 
SKIT_ALLOW_OTHER_TRY_CATCH macro symbol before #including this file.
TRY-CATCH implementations from other parties will NOT communicate with this
one, so any exceptions thrown in those cannot be caught by sTRY-sCATCH, and
vica-versa.  Furthermore, throwing exceptions from a different exceptions
implementation may cause very nasty runtime behavior and memory corruption
because it could easily unbalance the tracking of setjmp/longjmp calls
within the sTHROW-sTRY-sCATCH chain.

NOTE: Do not attempt to branch out of a sTRY-sCATCH block.  Example:
    int foo, len;
    len = 10;
    while ( foo < len )
    {
        sTRY
            foo++;
            if ( foo > len )
                break;  // The stack now permanently contains a reference
                        //   to this sTRY statement.  Any sEND_TRY's
                        //   encountered in calling code might actually
                        //   end up at the sTRY above.
            else
                continue; // ditto
        sEND_TRY
    }

DO NOT DO THE ABOVE ^^.
The "break" and "continue" statements will leave the sTRY-sEND_TRY block
without restoring stack information to the expected values.  

TODO: It'd be nice if there was some way to make the compiler forbid
such local jumps or maybe even make them work in sensible ways.
(Later note: it's possible, see compile_time_errors.h.  It has the
undesirable side-effect of forbiding ALL local jumps in the try-catch,
not just the ones that escape.  It's used for return/goto though.)

TODO: what if an exception is raised from within a sCATCH block? (untested!)

TODO: sFINALLY
*/

#ifndef SKIT_TRY_CATCH_INCLUDED
#define SKIT_TRY_CATCH_INCLUDED

#include <stdio.h> /* Incase printf debugging happens. */
#include <unistd.h> /* For ssize_t */
#include <inttypes.h>
#include <setjmp.h>
#include <limits.h> /* For INT_MIN and the like. */

#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/throw.h"
	
/* __SKIT_TRY_SAFE_EXIT is an implementation detail.
It allows execution to exit an sTRY-sCATCH block by jumping to a state distinct
from the dangerous ways of exiting a sTRY-sCATCH block.  The "safe" way is
for execution to fall to the bottom of a given block in the sTRY-sCATCH
statement.  The "dangerous" was are by using local jumping constructs like
"break" and "continue" to leave a sTRY-sCATCH statement.  
*/
#define __SKIT_TRY_SAFE_EXIT         INT_MIN+1

/* __SKIT_TRY_EXCEPTION_CLEANUP is an implementation detail.
It is jumped to at the end of sCATCH blocks as a way of cleaning up the
exception allocated in the code that threw the exception.
*/
#define __SKIT_TRY_EXCEPTION_CLEANUP INT_MIN

/* TODO: Maybe we should use different macro names since OpenVMS DECC already has a kind of TRY-CATCH structure. */
#if defined(__DECC) && !defined(SKIT_ALLOW_OTHER_TRY_CATCH)
#undef TRY
#undef CATCH
#undef ENDTRY
#endif

#define SKIT_USE_TRY_CATCH_EMULATION \
	int __skit_try_catch_nesting_level = 0; \
	do {} while(0)

/** */
#define sTRY /* */ \
	{ \
	SKIT_FEATURE_TRACE("%s, %d.91: sTRY.start\n", __FILE__, __LINE__); \
	SKIT_FEATURE_TRACE("type_jmp_stack_alloc\n"); \
	SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
	(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
	if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->try_jmp_stack,&skit_malloc)) != __SKIT_TRY_SAFE_EXIT ) { \
		SKIT_COMPILE_TIME_CHECK(SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR,0); \
		SKIT_COMPILE_TIME_CHECK(SKIT_NO_GOTO_FROM_TRY_PTR,0); \
		SKIT_FEATURE_TRACE("%s, %d.98: sTRY.if\n", __FILE__, __LINE__); \
		int __skit_prev_exception_stack_size = 0; \
		__skit_try_catch_nesting_level++; \
		do { \
			SKIT_FEATURE_TRACE("%s, %d.100: sTRY.do\n", __FILE__, __LINE__); \
			SKIT_FEATURE_TRACE("exc_jmp_stack_alloc\n"); \
			SKIT_FEATURE_TRACE("sTRY: exc_jmp_stack.size == %ld\n", skit_thread_ctx->exc_jmp_stack.used.length); \
			switch( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack,&skit_malloc)) ) \
			{ \
			case __SKIT_TRY_EXCEPTION_CLEANUP: \
			{ \
				/* Exception cleanup case. */ \
				/* Note that this case is ALWAYS reached because sCATCH and sEND_TRY macros  */ \
				/*   may follow either the successful block or an exceptional one, so    */ \
				/*   there is no way to know from within this macro which one should be  */ \
				/*   jumped to.  The best strategy then is to always jump to the cleanup */ \
				/*   case after leaving any part of the sTRY-sCATCH-sEND_TRY and only free   */ \
				/*   exceptions if there actually are any.                               */ \
				SKIT_FEATURE_TRACE("%s, %d.113: sTRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
				/* Cleanup any exceptions thrown during ONLY THIS sTRY-sCATCH. */ \
				while (skit_thread_ctx->exc_instance_stack.used.length > __skit_prev_exception_stack_size) \
				{ \
					skit_exception *exc = skit_exc_fstack_pop(&skit_thread_ctx->exc_instance_stack); \
					skit_exception_dtor(skit_thread_ctx, exc); \
				} \
				SKIT_FEATURE_TRACE("sTRY: exc_jmp_stack.size == %ld\n", skit_thread_ctx->exc_jmp_stack.used.length); \
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
				/* the sTRY block is entered, and that causes setjmp to select      */ \
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
					SKIT_FEATURE_TRACE("%s, %d.146: sTRY: case 0:\n", __FILE__, __LINE__); \
					__skit_prev_exception_stack_size = skit_thread_ctx->exc_instance_stack.used.length;

/** */
#define sCATCH(__skit_error_code, exc_name) /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a catch block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.153: sTRY: case 0: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, __SKIT_TRY_EXCEPTION_CLEANUP); \
				} \
				else if ( skit_exception_is_a( skit_thread_ctx->exc_instance_stack.used.front->val.error_code, __skit_error_code) ) \
				{ \
					/* sCATCH block. */ \
					SKIT_FEATURE_TRACE("%s, %d.159: sTRY: case %d:\n", __FILE__, __LINE__, __skit_error_code); \
					skit_exception *exc_name = &skit_thread_ctx->exc_instance_stack.used.front->val; \
					/* The caller may not want to actually use the named exception. */ \
					/* They might just want to prevent its propogation and do different logic. */ \
					/* Thus we'll write (void)(exc_name) to prevent "warning: unused variable" messages. */ \
					(void)(exc_name);
					

/** */
#define sEND_TRY /* */ \
					/* This end-of-block may be either the end of the normal/success case */ \
					/*   OR the end of a sCATCH block.  It must be able to do the correct */ \
					/*   thing regardless of where it comes from. */ \
					SKIT_FEATURE_TRACE("%s, %d.172: sTRY: case ??: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, __SKIT_TRY_EXCEPTION_CLEANUP); \
				} \
				else \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					SKIT_FEATURE_TRACE("%s, %d.178: sTRY: default: longjmp\n", __FILE__, __LINE__); \
					__skit_try_catch_nesting_level--; \
					SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
					SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
					__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
				} \
				sASSERT(0); /* This should never be reached. The if-else chain above should handle all remaining cases. */ \
			} /* default: { } */ \
			} /* switch(setjmp(*__push...)) */ \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			SKIT_FEATURE_TRACE("%s, %d.192: sTRY: break found!\n", __FILE__, __LINE__); \
			SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
			skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
			sTHROW(SKIT_BREAK_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'break' statement from within a sTRY-sCATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" ); \
		} while (0); \
		/* If execution makes it here, then the caller */ \
		/* used a "continue" statement and is trying to */ \
		/* corrupt the debug stack.  Don't let them do it! */ \
		/* Instead, throw another exception. */ \
		SKIT_FEATURE_TRACE("%s, %d.206: sTRY: continue found!\n", __FILE__, __LINE__); \
		SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
		SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
		skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
		sTHROW(SKIT_CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a sTRY-sCATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n"); \
	} \
	__skit_try_catch_nesting_level--; \
	SKIT_FEATURE_TRACE("%s, %d.216: sTRY: done.\n", __FILE__, __LINE__); \
	}

#define sENDTRY \
				} /* if-else */ \
			} /* default: { } */ \
			} /* switch(setjmp(*__push...)) */ \
		} while (0); \
	} \
	SKIT_sENDTRY_IS_TYPO_TXT(SKIT_sENDTRY_IS_TYPO_PTR); \
	(void)*SKIT_sENDTRY_IS_TYPO_PTR; \
	}

#endif
