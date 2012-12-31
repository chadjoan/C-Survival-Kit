
#ifdef __DECC
#pragma module skit_feature_emu_init
#endif

#include <stdlib.h>
#include <stdio.h>

#include "survival_kit/misc.h" /* skit_die */
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/debug.h"

static void skit_thread_final_cleanup(void *ptr)
{
	/* Do nothing for now. */
	/* Thread context in C Survival Kit has no way to guarantee establishment */
	/*   on thread entry.  As such, it has to define its own entry point with */
	/*   SKIT_THREAD_ENTRY_POINT().  Those resources are cleaned up with */
	/*   corresponding calls to SKIT_THREAD_EXIT_POINT(), which are placed in */
	/*   ways that will match calls to the entry point.  THIS function that */
	/*   occurs at the end of pthread termination DOES NOT match those calls. */
	/*   Due to that unmatched nature, no cleanup is done here: it should */
	/*   have already been done in another place. */
	/* It might be acceptable to do some last-ditch cleanup, as long as it can */
	/*   ensure that normal thread closure will be unaffected. */
}

void skit_features_init()
{
	pthread_key_create(&skit_thread_context_key, skit_thread_final_cleanup);
}
