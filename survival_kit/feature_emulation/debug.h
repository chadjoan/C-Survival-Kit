
#ifndef SKIT_FEMU_DEBUG_INCLUDED
#define SKIT_FEMU_DEBUG_INCLUDED

/* Debugging twiddly knobs.  Useful if you have the misfortune of needing to
// debug the code that is supposed to make debugging easier. */
#define SKIT_DO_FEATURE_EMULATION_TRACING 0
#if SKIT_DO_FEATURE_EMULATION_TRACING == 1
	#define SKIT_FEATURE_TRACE(...) (printf(__VA_ARGS__))
#else
	#define SKIT_FEATURE_TRACE(...) ((void)1)
#endif

#endif
