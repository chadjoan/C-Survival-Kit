#include <assert.h>
#include <stdio.h>

#include "survival_kit/macro.h"

void skit_macro_unittest()
{
	printf("skit_macro_unittest()\n");
	
	assert( SKIT_NARGS()        == 0 );
	assert( SKIT_NARGS(9000)    == 1 );
	assert( SKIT_NARGS(1,2,3,4) == 4 );
	printf("  SKIT_NARGS_test passed.\n");
	
	assert( SKIT_NARGS1()        == 0 );
	assert( SKIT_NARGS1(9000)    == 1 );
	assert( SKIT_NARGS1(1,2,3,4) == 1 );
	printf("  SKIT_NARGS1_test passed.\n");
	
	assert( SKIT_NARGS2()        == 0 );
	assert( SKIT_NARGS2(9000)    == 1 );
	assert( SKIT_NARGS2(1,2,3,4) == 2 );
	printf("  SKIT_NARGS2_test passed.\n");
	
	assert( SKIT_NARGS3()        == 0 );
	assert( SKIT_NARGS3(9000)    == 1 );
	assert( SKIT_NARGS3(1,2,3,4) == 3 );
	printf("  SKIT_NARGS3_test passed.\n");
	
	#define MAX(...) SKIT_MACRO_DISPATCHER(MAX, __VA_ARGS__)(__VA_ARGS__)
	#define MAX1(a) a
	#define MAX2(a, b) ((a)>(b)?(a):(b))
	#define MAX3(a, b, c) MAX2(MAX2(a, b), c)
	assert( MAX3(1,2,3) == 3 );
	printf("  SKIT_MACRO_DISPATCHER_test passed.\n");
	
	printf("  skit_macro_unittest passed!\n");
	printf("\n");
}
