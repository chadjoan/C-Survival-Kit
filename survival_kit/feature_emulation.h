#ifndef SKIT_FEATURE_EMULATION_INCLUDED
#define SKIT_FEATURE_EMULATION_INCLUDED

#include "survival_kit/assert.h"
#include "survival_kit/init.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/exceptions.h"
#include "survival_kit/feature_emulation/call.h"
#include "survival_kit/feature_emulation/try_catch.h"
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/unittest.h"


/** Exception definitions.  TODO: these should have a definition syntax that allows a separate tool to sweep all source files and automatically allocate non-colliding error codes for all exceptions and place them in a file that can be included from ERR_UTIL.H. */
/* GENERIC exceptions are the root of all catchable exceptions. */
/* @define_exception(GENERIC_EXCEPTION, "An exception was thrown.") */
/* @define_exception(BREAK_IN_TRY_CATCH : GENERIC_EXCEPTION,
"Code has attempted to use a 'break' statement from within a sTRY-sCATCH block.\n"
"This could easily corrupt program execution and corrupt debugging data.\n"
"Do not do this, ever!") */
/* @define_exception(CONTINUE_IN_TRY_CATCH : GENERIC_EXCEPTION,
"Code has attempted to use a 'continue' statement from within a sTRY-sCATCH block.\n"
"This could easily corrupt program execution and corrupt debugging data.\n"
"Do not do this, ever!" ) */

/*
FATAL exceptions should never be caught.
They signal potentially irreversable corruption, such as memory corruption
that may cause things like unexpected null pointers or access violations.
*/
/* @define_exception(FATAL_EXCEPTION, "A fatal exception was thrown.") */

/** Place this at the top of function bodies that use language feature emulation. */
#define SKIT_USE_FEATURE_EMULATION \
	do { \
		sASSERT(skit_init_was_called()); \
	}while(0); \
	SKIT_COMPILE_TIME_CHECK(SKIT_USE_FEATURES_IN_FUNC_BODY, 0); \
	SKIT_COMPILE_TIME_CHECK(SKIT_RETURN_HAS_USE_TXT, 1); \
	skit_thread_context *skit_thread_ctx = skit_thread_context_get(); \
	sASSERT(skit_thread_init_was_called()); \
	sASSERT(skit_thread_ctx != NULL); \
	SKIT_USE_SCOPE_EMULATION; \
	SKIT_USE_TRY_CATCH_EMULATION; \
	(void)skit_thread_ctx; \
	skit_thread_context_pos __skit_thread_ctx_pos; /* Declare here so that sCALL doesn't have to. */ \
	(void)__skit_thread_ctx_pos; \
	void *__skit_sTRACE_return_value; \
	(void)__skit_sTRACE_return_value; \
	skit_exception *__skit_propogate_exception_tmp; \
	(void)__skit_propogate_exception_tmp; \
	do {} while(0)
	
#endif