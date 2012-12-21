
#ifndef SKIT_INIT_INCLUDED
#define SKIT_INIT_INCLUDED

#include <pthread.h>

#include "survival_kit/feature_emulation/init.h"
#include "survival_kit/feature_emulation/thread_context.h"

/**
Initializes all C Survival Kit modules.

It is not necessary to call this if SKIT_USE_FEATURE_EMULATION appears before
any calls into C Survival Kit functions.  The SKIT_USE_FEATURE_EMULATION header
will call this on an as-needed basis.

It can be used to initialize C Survival Kit modules without allowing feature 
emulation use, which allows calling various skit_* functions without crashing 
from lack of initialization.  This strategy does not prevent indirect use of
feature emulation: functions within C Survival Kit may use feature emulation
on their own anyways.
*/
void skit_init();

/**
Returns 1 if global module initialization has been performed on all 
  C Survival Kit modules.
Returns 0 if it was not.
*/
int skit_init_was_called();

/* Used internally to establish the start of the stack and does thread-local module init. */
#define SKIT_THREAD_ENTRY_POINT(skit_thread_ctx) \
	( \
		SKIT_THREAD_CONTEXT_INIT(skit_thread_ctx), \
		_skit_thread_module_init(), \
		1 \
	)
	
#define SKIT_THREAD_CHECK_ENTRY(skit_thread_ctx) \
	( \
		( skit_thread_ctx == NULL ) ? \
		( \
			SKIT_THREAD_ENTRY_POINT(skit_thread_ctx), \
			1 \
		) : ( 1 ), \
		1 \
	)

void _skit_thread_module_init();

/* Used internally to clean up when the stack is being exited. */
#define SKIT_THREAD_EXIT_POINT(skit_thread_ctx) \
	( \
		SKIT_THREAD_CONTEXT_CLEANUP(skit_thread_ctx), \
		_skit_thread_module_cleanup(), \
		1 \
	)

#define SKIT_THREAD_CHECK_EXIT(skit_thread_ctx) \
	( \
		( skit_stack_depth(skit_thread_ctx) == 0 ) ? \
		( \
			SKIT_THREAD_EXIT_POINT(skit_thread_ctx), \
			1 \
		) : ( 1 ), \
		1 \
	)

void _skit_thread_module_cleanup();

/**
Using SKIT_THREAD_WAS_ENTERED() is probably a faster check that also considers
  the presence of the thread context used to store debug info and exceptions.
Returns 1 if the various skit modules have been initialized for this thread.
Returns 0 if it was not.
*/
int skit_thread_init_was_called();

/**
Returns 1 if a thread context exists.  This also implies the initialization of
  all skit modules at a thread-local level (that is, 
  skit_thread_init_was_called() == 1).  
Returns 0 if it was not.
*/
#define SKIT_THREAD_WAS_ENTERED() \
	( \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1, \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY, \
		skit_thread_ctx != NULL, \
	)

#endif
