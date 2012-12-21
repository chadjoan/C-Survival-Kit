
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

void skit_features_init()
{
	pthread_key_create(&skit_thread_context_key, skit_thread_context_dtor);
}
