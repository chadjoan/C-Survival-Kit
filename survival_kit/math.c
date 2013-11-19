
#ifdef __DECC
#pragma module skit_math
#endif

#include <inttypes.h>
#include <stdio.h>

#include "survival_kit/math.h"
#include "survival_kit/assert.h"

static void skit_math_macro_test()
{
	int foo = 5;
	double bar;
	bar = SKIT_MAX(foo, 6, foo+3.5, 1.2);
	sASSERT(bar == 8.5);
	bar = SKIT_MIN(foo, 6, foo+3.5, 1.2);
	sASSERT(bar == 1.2);
	printf("  skit_math_macro_test passed.\n");
}

int64_t skit_ipow(int64_t base, int exp)
{
    int64_t result = 1;
	sASSERT_GE(exp, 0); // exponents <0 are untested.
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

static void skit_math_ipow_test()
{
	sASSERT_EQ( skit_ipow(2,0), 1 );
	sASSERT_EQ( skit_ipow(2,2), 4 );
	sASSERT_EQ( skit_ipow(3,2), 9 );
	sASSERT_EQ( skit_ipow(3,3), 27 );
	sASSERT_EQ( skit_ipow(-2,2), 4 );
	sASSERT_EQ( skit_ipow(-2,0), 1 );
	printf("  skit_math_ipow_test passed.\n");
}


void skit_math_unittest()
{
	printf("skit_math_unittest\n");
	skit_math_macro_test();
	skit_math_ipow_test();
	printf("  skit_math_unittest passed!\n");
	printf("\n");
}
