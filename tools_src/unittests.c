
#include "survival_kit/stack_builtins.h"
#include "survival_kit/fstack_builtins.h"
#include "survival_kit/feature_emulation.h"

#include <stdio.h> /* incase printf is needed. */

int main(int argc, char *argv[])
{
	skit_stack_unittest();
	skit_fstack_unittest();
	skit_unittest_features();
	return 0;
}
