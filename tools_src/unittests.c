
#include "survival_kit/init.h"
#include "survival_kit/stack_builtins.h"
#include "survival_kit/fstack_builtins.h"
#include "survival_kit/feature_emulation.h"
#include "survival_kit/signal_handling.h"

#include <stdio.h> /* incase printf is needed. */

int main(int argc, char *argv[])
{
	skit_init();
	skit_stack_unittest();
	skit_fstack_unittest();
	skit_unittest_features();
	skit_unittest_signal_handling();
	return 0;
}
