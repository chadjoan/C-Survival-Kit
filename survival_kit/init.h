
#ifndef SKIT_INIT_INCLUDED
#define SKIT_INIT_INCLUDED

/** Call this at the beginning of main(). */
void skit_init();

/**
Call this at the beginning of every new thread, except for in main().
*/
void skit_thread_init();

#endif
