
#ifdef __DECC
#pragma module skit_signal_handling
#endif

#include <signal.h>
#include <pthread.h>
#include <string.h> /* strerror_r */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h> /* getpid() */

#include "survival_kit/feature_emulation.h" /* For sCALL; it's important for unittesting. */
#include "survival_kit/feature_emulation/funcs.h" /* For skit_print_stack_trace() */
#include "survival_kit/signal_handling.h"
#include "survival_kit/assert.h"


static void skit_setup_sigaction_struct( struct sigaction *act );
static void skit_signal_handler( int signo );

/* TODO: more fatal exceptions should be caught, not just null deref. */
#ifdef __VMS
#define NULL_DEREF SIGBUS
#else
#define NULL_DEREF SIGSEGV
#endif

static void skit_sigaction( int sig, const struct sigaction *act, struct sigaction *oact )
{
	int success = sigaction(sig, act, oact);
	if ( success != 0 )
	{
		#ifdef __VMS
			/* strerror_r is not provided on OpenVMS, but supposedly strerror is thread-safe on VMS.
			source: http://www.mail-archive.com/boost-cvs@lists.sourceforge.net/msg08433.html
			(Search for "VMS doesn't provide strerror_r, but on this platform")
			*/
			char *buf = strerror(errno);
		#elif defined(__linux__)
			/* Linux: do it the normal way. */
			char buf[1024];
			strerror_r(errno, buf, 1024);
		#else
		#	error "This requires porting."
		#endif
		sASSERT_MSG( success == 0, buf );
	}
}

static void skit_cleanup_signal_save(void *context)
{
	struct sigaction *old_action = (struct sigaction*)context;
	if ( old_action != NULL )
		free((void*)old_action);
}

static pthread_key_t skit_signal_save_key;

void skit_sig_init()
{
	pthread_key_create(&skit_signal_save_key, &skit_cleanup_signal_save);
	pthread_setspecific(skit_signal_save_key, NULL);
}

static void skit_setup_sigaction_struct( struct sigaction *act )
{
	memset(act, '\0', sizeof(*act));
	act->sa_handler = &skit_signal_handler;
}

/* VMS does not have the sa_sigaction member of the sigaction struct. */
/* This makes it impossible to portably use the handler that has more info passed into it. */
/* static void skit_signal_handler( int signo, siginfo_t *info, void *context ) */
static void skit_signal_handler( int signo )
{
	struct sigaction new_action;
	sigset_t sigset;
	char error_message[] = "\nA signal was caught!\n";
	
	/* TODO: stack trace printing should block signals. */
	write(STDERR_FILENO, error_message, sizeof(error_message));
	skit_print_stack_trace();
	
	/* Initialize new_action.  Don't use stdlib functions. */
	sigemptyset(&sigset); /* OK because it's on the async list. */
	new_action.sa_handler = SIG_DFL;
	new_action.sa_mask = sigset;
	new_action.sa_flags = 0;
	#ifndef __VMS
	new_action.sa_sigaction = NULL;
	#endif
	
	skit_sigaction(signo, &new_action, NULL); /* maybe calls strerror_r, which is safe because it's reentrant. */
	kill(getpid(), signo); /* getpid() is on the async list. Safe! (sorta) */
}

void skit_push_tracing_sig_handler()
{
	struct sigaction new_action;
	struct sigaction *old_action;
	
	old_action = pthread_getspecific(skit_signal_save_key);
	if ( old_action != NULL )
	{
		sASSERT_MSG(0,
			"skit_push_tracing_sig_handler() was called more than once without popping first.\n"
			"This is currently unimplmented.\n");
	}
	
	old_action = (struct sigaction*)malloc(sizeof(struct sigaction));
	
	/* TODO: handle more than just NULL dereferencing. */
	skit_setup_sigaction_struct(&new_action);
	skit_sigaction(NULL_DEREF, &new_action, old_action);
	pthread_setspecific(skit_signal_save_key, old_action);
}

void skit_pop_tracing_sig_handler()
{
	struct sigaction *old_action;
	
	old_action = pthread_getspecific(skit_signal_save_key);
	if ( old_action == NULL )
	{
		sASSERT_MSG(0,
			"skit_pop_tracing_sig_handler() was called without a corresponding call to\n"
			"  skit_push_tracing_sig_handler.\n");
	}
	
	pthread_setspecific(skit_signal_save_key, NULL);
}

/* TODO: unit testing this would be weird and require script automation at a shell level. */
void skit_unittest_signal_subfunc2()
{
	/* NOTE: it would be possible to get more specific debugging
	information by wrapping the dereference in a sCALL statement:
	    int explode;
	    sCALL(explode = *ptr);
	This is understandably inconvenient, especially when it is unknown
	  in advance which dereferences could be problematic.
	*/
	printf("Now in the function that dereferences NULL.\n");
	int *ptr = NULL;
	int explode = *ptr;
	printf("VMS is OK with this operation.  What happened? %d\n", explode);
}

void skit_unittest_signal_subfunc()
{
	SKIT_USE_FEATURE_EMULATION;
	sCALL(skit_unittest_signal_subfunc2());
}

void skit_unittest_signal_handling()
{
	SKIT_USE_FEATURE_EMULATION;
	printf("\n");
	printf("The unittester will now trigger a segfault by dereferencing NULL.\n");
	printf("This signal should get handled and print a stacktrace.\n");
	skit_push_tracing_sig_handler();
	sCALL(skit_unittest_signal_subfunc());
	printf("This should never get printed.\n");
}