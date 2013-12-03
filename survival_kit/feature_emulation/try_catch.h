
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

#include "survival_kit/init.h"
#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/throw.h"
	
/* SKIT__TRY_SAFE_EXIT is an implementation detail.
It allows execution to exit an sTRY-sCATCH block by jumping to a state distinct
from the dangerous ways of exiting a sTRY-sCATCH block.  The "safe" way is
for execution to fall to the bottom of a given block in the sTRY-sCATCH
statement.  The "dangerous" was are by using local jumping constructs like
"break" and "continue" to leave a sTRY-sCATCH statement.  
*/
#define SKIT__TRY_SAFE_EXIT           INT_MIN

/* SKIT__TRY_EXCEPTION_CLEANUP is an implementation detail.
It is jumped to whenever an sTRY or sCATCH block handled an exception
that was thrown inside of it.
*/
#define SKIT__TRY_EXCEPTION_CLEANUP   INT_MIN+1

/* SKIT__TRY_UNHANDLED_EXCEPTION is an implementation detail.
It is jumped to whenever an sTRY or sCATCH block couldn't handle an exception
that was thrown inside of it.
*/
#define SKIT__TRY_UNHANDLED_EXCEPTION INT_MIN+2

/* SKIT__TRY__END_OF_CATCH is an implementation detail.
It is used to indicate that the code being executed is the tail-end of an
sCATCH block.
*/
#define SKIT__TRY__END_OF_CATCH       INT_MIN+4

/* SKIT__TRY__END_OF_TRY is an implementation detail.
It is used to indicate that the code being executed is the tail-end of an
sTRY block.
*/
#define SKIT__TRY__END_OF_TRY         INT_MIN+5

/* TODO: Maybe we should use different macro names since OpenVMS DECC already has a kind of TRY-CATCH structure. */
#if defined(__DECC) && !defined(SKIT_ALLOW_OTHER_TRY_CATCH)
#undef TRY
#undef CATCH
#undef ENDTRY
#endif

#define SKIT_USE_TRY_CATCH_EMULATION \
	int skit__try_catch_nesting_level = 0; \
	(void)skit__try_catch_nesting_level; \
	int skit__try_block_end_type = 0; \
	(void)skit__try_block_end_type; \
	skit_exception *skit__try_caught_exception = NULL; \
	(void)skit__try_caught_exception; \
	do {} while(0)

/** */
#define sTRY /* */ \
	{ \
	SKIT_FEATURE_TRACE("%s, %d.121: sTRY.start\n", __FILE__, __LINE__); \
	SKIT_FEATURE_TRACE("type_jmp_stack_alloc\n"); \
	SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
	(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
	SKIT_THREAD_CHECK_ENTRY(skit_thread_ctx); \
	if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->try_jmp_stack,&skit_malloc)) != SKIT__TRY_SAFE_EXIT ) { \
		SKIT_COMPILE_TIME_CHECK(SKIT_NO_BUILTIN_RETURN_FROM_TRY_PTR,0); \
		SKIT_COMPILE_TIME_CHECK(SKIT_NO_GOTO_FROM_TRY_PTR,0); \
		SKIT_FEATURE_TRACE("%s, %d.129: sTRY.if\n", __FILE__, __LINE__); \
		skit__try_catch_nesting_level++; \
		do { \
			SKIT_FEATURE_TRACE("%s, %d.132: sTRY.do\n", __FILE__, __LINE__); \
			SKIT_FEATURE_TRACE("exc_jmp_stack_alloc\n"); \
			SKIT_FEATURE_TRACE("sTRY: exc_jmp_stack.size == %ld\n", skit_thread_ctx->exc_jmp_stack.used.length); \
			\
			/* NOTE: There is currently no logic that saves the value of skit__try_setjmp_code */ \
			/*   when execution passes into the caller's part of the block. */ \
			/*   As a consequence, do not use skit__try_setjmp_code outside of the sTRY macro. */ \
			int skit__try_setjmp_code = setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack,&skit_malloc)); \
			SKIT_FEATURE_TRACE("%s, %d.140: sTRY: switch( skit__try_setjmp_code )\n", __FILE__, __LINE__); \
			switch( skit__try_setjmp_code ) \
			{ \
				case SKIT__TRY_EXCEPTION_CLEANUP: \
				{ \
					/* Exception cleanup case. */ \
					SKIT_FEATURE_TRACE("%s, %d.145: sTRY: case CLEANUP: longjmp\n", __FILE__, __LINE__); \
					SKIT_FEATURE_TRACE("sTRY: exc_jmp_stack.size == %ld\n", skit_thread_ctx->exc_jmp_stack.used.length); \
					SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
					SKIT_FEATURE_TRACE("try_jmp_stack_pop\n"); \
					longjmp(*skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack), SKIT__TRY_SAFE_EXIT); \
				} \
				\
				case SKIT__TRY_UNHANDLED_EXCEPTION: \
				{ \
					/* An exception was thrown and we can't handle it. */ \
					SKIT_FEATURE_TRACE("%s, %d.156: sTRY: SKIT__TRY_UNHANDLED_EXCEPTION: longjmp\n", __FILE__, __LINE__); \
					skit__try_catch_nesting_level--; \
					SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
					SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
					skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
					SKIT__PROPOGATE_THROWN_EXCEPTIONS; \
				} \
				\
				default: \
				\
				SKIT_FEATURE_TRACE("%s, %d.167: sTRY: default:\n", __FILE__, __LINE__); \
				if ( skit_thread_ctx->exc_instance_stack.used.length > 0 ) \
					SKIT_FEATURE_TRACE("%s, %d.170: sTRY: error code: %d\n", __FILE__, __LINE__, skit_thread_ctx->exc_instance_stack.used.front->val.error_code ); \
				if (skit__try_setjmp_code == 0) /* First time through: descend into the sTRY portion. */ \
				{ \
					skit__try_block_end_type = SKIT__TRY__END_OF_TRY; \
					if (1) \
					{ \
						/* Prevent the caller from clobbering the try_block_end_type */ \
						/* and skit__try_caught_exception variables when they do things */ \
						/* like nesting sTRY-sCATCH statements. */ \
						SKIT_FEATURE_TRACE("%s, %d.175: sTRY: case\n", __FILE__, __LINE__); \
						int skit__try_block_end_type = 0; \
						(void)skit__try_block_end_type; \
						skit_exception *skit__try_caught_exception = NULL; \
						(void)skit__try_caught_exception;

#define SKIT__TRY_END_OF_BLOCK_WRAPUP \
					else \
					{ \
						/* This path is reached due to thrown exceptions within sCATCH blocks. */ \
						SKIT_FEATURE_TRACE("%s, %d.190: sTRY: case\n", __FILE__, __LINE__); \
						SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
						skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
						longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, SKIT__TRY_UNHANDLED_EXCEPTION); \
					} \
					\
					if ( skit__try_block_end_type == SKIT__TRY__END_OF_CATCH ) \
					{ \
						SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
						skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
						skit_exception_dtor(skit_thread_ctx, skit__try_caught_exception); \
					} \
					SKIT_FEATURE_TRACE("%s, %d.202: sTRY: end of block: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, SKIT__TRY_EXCEPTION_CLEANUP); \

/** */
#define sCATCH(skit__error_code, exc_name) /* */ \
						SKIT_FEATURE_TRACE("%s, %d.207: sTRY: entering end of block\n", __FILE__, __LINE__); \
					} \
					SKIT__TRY_END_OF_BLOCK_WRAPUP \
				} \
				else if ( skit_exception_is_a( skit_thread_ctx->exc_instance_stack.used.front->val.error_code, skit__error_code) ) \
				{ \
					/* sCATCH block. */ \
					SKIT_FEATURE_TRACE("%s, %d.213: sTRY: case %d:\n", __FILE__, __LINE__, skit__error_code); \
					skit__try_caught_exception = skit_exc_fstack_pop(&skit_thread_ctx->exc_instance_stack); \
					skit_exception *exc_name = skit__try_caught_exception; \
					/* The caller may not want to actually use the named exception. */ \
					/* They might just want to prevent its propogation and do different logic. */ \
					/* Thus we'll write (void)(exc_name) to prevent "warning: unused variable" messages. */ \
					(void)(exc_name); \
					skit__try_block_end_type = SKIT__TRY__END_OF_CATCH; \
					SKIT_FEATURE_TRACE("exc_jmp_stack_alloc\n"); \
					if ( setjmp(*skit_jmp_fstack_alloc(&skit_thread_ctx->exc_jmp_stack,&skit_malloc)) == 0 ) \
					{ \
						/* Prevent the caller from clobbering the try_block_end_type */ \
						/* and skit__try_caught_exception variables when they do things */ \
						/* like nesting sTRY-sCATCH statements. */ \
						SKIT_FEATURE_TRACE("%s, %d.227: sTRY: case %d:\n", __FILE__, __LINE__, skit__error_code); \
						int skit__try_block_end_type = 0; \
						(void)skit__try_block_end_type; \
						skit_exception *skit__try_caught_exception = NULL; \
						(void)skit__try_caught_exception;

/** */
#define sEND_TRY /* */ \
						SKIT_FEATURE_TRACE("%s, %d.235: sTRY: entering end of block\n", __FILE__, __LINE__); \
					} \
					SKIT__TRY_END_OF_BLOCK_WRAPUP \
				} \
				else \
				{ \
					SKIT_FEATURE_TRACE("%s, %d.241: sTRY: Unhandled exception: longjmp\n", __FILE__, __LINE__); \
					longjmp( skit_thread_ctx->exc_jmp_stack.used.front->val, SKIT__TRY_UNHANDLED_EXCEPTION); \
				} \
				sASSERT(0); /* This should never be reached. The if-else chain above should handle all remaining cases. */ \
			} /* switch( skit__try_setjmp_code ) */ \
			/* If execution makes it here, then the caller */ \
			/* used a "break" statement and is trying to */ \
			/* corrupt the debug stack.  Don't let them do it! */ \
			/* Instead, throw another exception. */ \
			SKIT_FEATURE_TRACE("%s, %d.250: sTRY: break found!\n", __FILE__, __LINE__); \
			SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
			skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
			SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
			skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
			/* This acts as if we first leave the TRY-CATCH and then thrown. */ \
			/* In that situation, we want to nest our entry-exit checks correctly */ \
			/* during the short time where we "leave". */ \
			/* It's OK this check trashes the context.  The sTHROW statement will */ \
			/* make another. */ \
			SKIT_THREAD_CHECK_EXIT(skit_thread_ctx); \
			sTHROW(SKIT_BREAK_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'break' statement from within a sTRY-sCATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n" ); \
		} while (0); \
		/* If execution makes it here, then the caller */ \
		/* used a "continue" statement and is trying to */ \
		/* corrupt the debug stack.  Don't let them do it! */ \
		/* Instead, throw another exception. */ \
		SKIT_FEATURE_TRACE("%s, %d.270: sTRY: continue found!\n", __FILE__, __LINE__); \
		SKIT_FEATURE_TRACE("exc_jmp_stack_pop\n"); \
		skit_jmp_fstack_pop(&skit_thread_ctx->exc_jmp_stack); \
		SKIT_FEATURE_TRACE("exc_try_stack_pop\n"); \
		skit_jmp_fstack_pop(&skit_thread_ctx->try_jmp_stack); \
		/* This acts as if we first leave the TRY-CATCH and then thrown. */ \
		/* In that situation, we want to nest our entry-exit checks correctly */ \
		/* during the short time where we "leave". */ \
		/* It's OK this check trashes the context.  The sTHROW statement will */ \
		/* make another. */ \
		SKIT_THREAD_CHECK_EXIT(skit_thread_ctx); \
		sTHROW(SKIT_CONTINUE_IN_TRY_CATCH, "\n"\
"Code has attempted to use a 'continue' statement from within a sTRY-sCATCH block.\n" \
"This could easily corrupt program execution and corrupt debugging data.\n" \
"Do not do this, ever!\n"); \
	} \
	skit__try_catch_nesting_level--; \
	SKIT_THREAD_CHECK_EXIT(skit_thread_ctx); \
	\
	SKIT_FEATURE_TRACE("%s, %d.289: sTRY: done.\n", __FILE__, __LINE__); \
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
