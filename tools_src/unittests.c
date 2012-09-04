#include "survival_kit/features.h"
#include "survival_kit/slist_builtins.h"

#include <stdio.h> /* incase printf is needed. */

int main(int argc, char *argv[])
{
	skit_slist_unittest();
	skit_unittest_features();
	return 0;
}
