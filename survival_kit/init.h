
#ifndef SKIT_INIT_INCLUDED
#define SKIT_INIT_INCLUDED

#include <pthread.h>

/** Call this at the beginning of main(). */
void skit_init();

/**
Returns 1 if skit_init() was called.
Returns 0 if it was not.
*/
int skit_init_was_called();

/**
Call this at the beginning of every new thread, except for in main().
*/
void skit_thread_init();

/**
Returns 1 if skit_thread_init() was called.
Returns 0 if it was not.
*/
int skit_thread_init_was_called();

/* Internal use only. */
extern pthread_key_t __skit_thread_init_called;

#endif
