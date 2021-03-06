
#ifdef __DECC
#pragma module skit_init
#endif

#include "survival_kit/init.h"
#include "survival_kit/cstr.h"
#include "survival_kit/feature_emulation/init.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/parsing/peg.h"
#include "survival_kit/path.h"
#include "survival_kit/signal_handling.h"
#include "survival_kit/streams/init.h"
#include "survival_kit/trie.h"

#include <pthread.h>

static int skit__init_called = 0;

pthread_key_t skit__thread_init_called;
static void skit_thread_dummy_dtor(void *context) {}

void skit_init()
{
	if ( skit_init_was_called() )
		return;

	skit_init_exceptions();
	skit_features_init();
	
	// We consider global initialization to be "done" upon entry to all of
	// the module initializers.  This prevents things like
	// SKIT_USE_FEATURE_EMULATION from infinitely recursing when they try
	// to lazily initialize by calling skit_init().
	// The exceptions/features initialization calls (above this line) should
	// never use the SKIT_USE_FEATURE_EMULATION macro or anything that could
	// call skit_init().  That would be a bad circular dependency.
	skit__init_called = 1;
	
	skit_cstr_init();
	skit_sig_init();
	skit_path_module_init();
	skit_trie_module_init();
	skit_stream_module_init_all();
	skit_peg_module_init();
	pthread_key_create(&skit__thread_init_called, &skit_thread_dummy_dtor);
}

int skit_init_was_called()
{
	return skit__init_called;
}

void skit__thread_module_init()
{
	if ( skit_thread_init_was_called() )
		return;

	skit_cstr_thread_init();
	skit_path_thread_init();
	pthread_setspecific(skit__thread_init_called, (void*)1);
}

void skit__thread_module_cleanup()
{
	/* BUG: possible memory leak due to assymetrical init with skit_cstr_thread_init() */
	pthread_setspecific(skit__thread_init_called, (void*)0);
}

int skit_thread_init_was_called()
{
	/* If skit_init wasn't called, then we don't even have a way of */
	/* determining if skit_thread_init was called.  We can assume */
	/* that it was not. */
	if ( !skit_init_was_called() )
		return 0;

	/* Due to standard behavior of pthread_getspecific, NULL will be */
	/* returned if there was never any data associated with that key */
	/* in this thread. */
	/* See http://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_setspecific.html */
	return (size_t)pthread_getspecific(skit__thread_init_called);
}


