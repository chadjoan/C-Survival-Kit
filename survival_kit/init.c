
#ifdef __DECC
#pragma module skit_init
#endif

#include "survival_kit/init.h"
#include "survival_kit/cstr.h"
#include "survival_kit/feature_emulation/init.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/signal_handling.h"
#include "survival_kit/streams/init.h"

#include <pthread.h>

static int __skit_init_called = 0;

pthread_key_t __skit_thread_init_called;
static void skit_thread_dummy_dtor(void *context) {}

void skit_init()
{
	if ( skit_init_was_called() )
		return;

	skit_init_exceptions();
	skit_features_init();
	skit_cstr_init();
	skit_sig_init();
	skit_stream_static_init_all();
	pthread_key_create(&__skit_thread_init_called, &skit_thread_dummy_dtor);
	__skit_init_called = 1;
}

int skit_init_was_called()
{
	return __skit_init_called;
}

void _skit_thread_module_init()
{
	if ( skit_thread_init_was_called() )
		return;

	skit_cstr_thread_init();
	pthread_setspecific(__skit_thread_init_called, (void*)1);
}

void _skit_thread_module_cleanup()
{
	/* BUG: possible memory leak due to assymetrical init with skit_cstr_thread_init() */
	pthread_setspecific(__skit_thread_init_called, (void*)0);
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
	return (size_t)pthread_getspecific(__skit_thread_init_called);
}


