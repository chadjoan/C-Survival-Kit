
#ifdef __DECC
#pragma module skit_math
#endif

#include <stdio.h>

#include "survival_kit/math.h"
#include "survival_kit/assert.h"

void skit_math_unittest()
{
	printf("skit_math_unittest\n");
	int foo = 5;
	double bar;
	bar = SKIT_MAX(foo, 6, foo+3.5, 1.2);
	sASSERT_EQ(bar, 8.5, "%f");
	bar = SKIT_MIN(foo, 6, foo+3.5, 1.2);
	sASSERT_EQ(bar, 1.2, "%f");
	printf("  skit_math_unittest passed!\n");
	printf("\n");
}
