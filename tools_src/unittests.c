#include "survival_kit/features.h"

int main(int argc, char *argv[])
{
	skit_unittest_features();
	print_exception(new_exception(GENERIC_EXCEPTION,"foo"));
	return 0;
}
