#ifndef SKIT_FEATURE_EMULATION_INCLUDED
#define SKIT_FEATURE_EMULATION_INCLUDED

#include "survival_kit/assert.h"
#include "survival_kit/init.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/stack_trace.h"
#include "survival_kit/feature_emulation/trace.h"
#include "survival_kit/feature_emulation/try_catch.h"
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/unittest.h"

#define SKIT_NO_HEADER_IN_CONDITIONAL_TXT \
	The_SKIT_USE_FEATURE_EMULATION_header_must_not_appear_in_conditional_statements_like_if_or_while

/** Place this at the top of function bodies that use language feature emulation. */
#define SKIT_USE_FEATURE_EMULATION /* */ \
	char SKIT_NO_HEADER_IN_CONDITIONAL_TXT; /* error if some does "if (x) SKIT_USE_FEATURE_EMULATION; */ \
	(void)SKIT_NO_HEADER_IN_CONDITIONAL_TXT; \
	if ( skit_init_was_called() ) \
		skit_init(); \
	SKIT_COMPILE_TIME_CHECK(SKIT_USE_FEATURES_IN_FUNC_BODY, 0); \
	SKIT_COMPILE_TIME_CHECK(SKIT_RETURN_HAS_USE_TXT, 1); \
	skit_thread_context *skit_thread_ctx = skit_thread_context_get(); \
	sASSERT(skit_thread_init_was_called()); \
	sASSERT(skit_thread_ctx != NULL); \
	SKIT_USE_SCOPE_EMULATION; \
	SKIT_USE_TRY_CATCH_EMULATION; \
	(void)skit_thread_ctx; \
	skit_thread_context_pos __skit_thread_ctx_pos; /* Declare here so that sTRACE doesn't have to. */ \
	(void)__skit_thread_ctx_pos; \
	void *__skit_sTRACE_return_value; \
	(void)__skit_sTRACE_return_value; \
	skit_exception *__skit_propogate_exception_tmp; \
	(void)__skit_propogate_exception_tmp; \
	do {} while(0)
	
#endif