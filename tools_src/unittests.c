
#include "survival_kit/init.h"
#include "survival_kit/math.h"
#include "survival_kit/stack_builtins.h"
#include "survival_kit/fstack_builtins.h"
#include "survival_kit/feature_emulation.h"
#include "survival_kit/signal_handling.h"
#include "survival_kit/string.h"
#include "survival_kit/array_builtins.h"
#include "survival_kit/streams/text_stream.h"
#include "survival_kit/streams/pfile_stream.h"
#include "survival_kit/streams/tcp_stream.h"

#include <stdio.h> /* incase printf is needed. */

int main(int argc, char *argv[])
{
	skit_init();
	skit_macro_unittest();
	skit_math_unittest();
	skit_stack_unittest();
	skit_fstack_unittest();
	skit_unittest_features();
	skit_string_unittest();
	skit_array_unittest();
	skit_text_stream_unittests();
	skit_pfile_stream_unittests();
	skit_tcp_stream_unittests();
	skit_unittest_signal_handling(); /* Must go LAST.  This test crashes. */
	return 0;
}
