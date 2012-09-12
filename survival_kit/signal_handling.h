

/* Do not call this.  Call skit_init or skit_thread_init instead. */
/* See "survival_kit/init.h" */
void skit_sig_init();

/** 
These functions enable the C Survival Kit library to print stack traces when
signals occur: things like NULL pointer dereferencing or the user hitting
CTRL-C.  This is very useful for debugging and is highly recommended.  It is
also optional and off by default because people unfamiliar with C Survival Kit
might already have their own signal handlers and this functional could
interfere with them.
*/
void skit_push_tracing_sig_handler();
void skit_pop_tracing_sig_handler();

/* Warning: This will intentionally crash the unittest. */
void skit_unittest_signal_handling();