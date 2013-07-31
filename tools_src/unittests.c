
#include "survival_kit/init.h"
#include "survival_kit/bag.h"
#include "survival_kit/math.h"
#include "survival_kit/inheritance_table.h"
#include "survival_kit/stack_builtins.h"
#include "survival_kit/fstack_builtins.h"
#include "survival_kit/feature_emulation.h"
#include "survival_kit/signal_handling.h"
#include "survival_kit/string.h"
#include "survival_kit/trie.h"
#include "survival_kit/regex.h"
#include "survival_kit/array_builtins.h"
#include "survival_kit/streams/text_stream.h"
#include "survival_kit/streams/pfile_stream.h"
#include "survival_kit/streams/tcp_stream.h"
#include "survival_kit/streams/ind_stream.h"

#include <stdio.h> /* incase printf is needed. */

void skit_unittest_modules()
{
	skit_macro_unittest();
	skit_flags_unittest();
	skit_math_unittest();
	skit_bag_unittest();
	skit_stack_unittest();
	skit_fstack_unittest();
	skit_string_unittest();
	skit_trie_unittest();
	skit_regex_unittest();
	skit_array_unittest();
	skit_text_stream_unittests();
	skit_pfile_stream_unittests();
	skit_tcp_stream_unittests();
	skit_ind_stream_unittests();
	skit_unittest_signal_handling(); /* Must go LAST.  This test crashes. */
}

int main(int argc, char *argv[])
{
	/* Set the signal handler to do traces, so we get stack traces on segfaults. */
	skit_push_tracing_sig_handler();
	
	/* Features should be unittested twice under different conditions: */
	/* Once without an initial thread context. */
	/* And again with a thread context established here. */

	/* First: without a thread context. */
	/* This will test the ability of the various features to keep track */
	/*   of when the thread context should be allocated and freed. */
	/* If it allocates more than freeing: memory leak. */
	/* If it frees more than allocating: hard crashes due to memory corruption. */
	/* Both are bad, and starting off with no context will force each */
	/*   feature to allocate its own and clean it up.  If one of them is */
	/*   implemented wrong, then the balance will be off and we can catch */
	/*   it. */
	skit_inheritance_table_unittest(); /* Used by the features module, and init. */
	skit_init();
	skit_unittest_features();

	/* Now we get to use a pre-initialized context for everything else. */
	SKIT_USE_FEATURE_EMULATION;
	
	/* Now test features WITH a thread context. */
	/* This context is established by the sTRACE(...) macro that wraps */
	/*   this call to skit_unittest_features(). */
	sTRACE(skit_unittest_features());
	
	/* Everything else may benefit from an already-established context. */
	/* We've already tested context management at this point, so the */
	/*   only benefit now is getting slightly better stack traces on */
	/*   assertion fails. */
	sTRACE(skit_unittest_modules());
	
	skit_pop_tracing_sig_handler();
	
	return 0;
}
