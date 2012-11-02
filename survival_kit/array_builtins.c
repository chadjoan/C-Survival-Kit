
#ifdef __DECC
#pragma module skit_array_builtins
#endif

#define SKIT_T_HEADER "survival_kit/templates/array.h"
#include "survival_kit/array_builtins.h"
#undef SKIT_T_HEADER

#define SKIT_T_HEADER "survival_kit/templates/array.c"
#include "survival_kit/array_builtins.h"
#undef SKIT_T_HEADER

#include "survival_kit/assert.h"

#include <stdio.h>

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE int
#define SKIT_T_PREFIX utest_int
#define SKIT_T_FUNC_ATTR static
#include "survival_kit/templates/array.h"
#include "survival_kit/templates/array.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX
#undef SKIT_T_FUNC_ATTR

static void skit_slice_sanity_check()
{
	
	skit_loaf loaf = skit_loaf_alloc(4);
	skit_utf8c *ptr = skit_loaf_ptr(loaf);
	ptr[0] = 1;
	ptr[1] = 3;
	ptr[2] = 3;
	ptr[3] = 7;
	skit_loaf_resize(&loaf, 5);
	ptr = skit_loaf_ptr(loaf);
	ptr[4] = 3;
	sASSERT_EQ(ptr[0], 1, "%d");
	sASSERT_EQ(ptr[1], 3, "%d");
	sASSERT_EQ(ptr[2], 3, "%d");
	sASSERT_EQ(ptr[3], 7, "%d");
	
	skit_slice slice = skit_slice_of(loaf.as_slice, 2, SKIT_EOT);
	ptr = skit_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3, "%d");
	sASSERT_EQ(ptr[1], 7, "%d");
	sASSERT_EQ(ptr[2], 3, "%d");
	
	slice = skit_slice_of(loaf.as_slice, 2, 3);
	ptr = skit_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3, "%d");
	sASSERT_EQ(ptr[1], 7, "%d");
}

void skit_array_unittest()
{
	printf("skit_array_unittest()\n");
	
	skit_slice_sanity_check();
	
	skit_utest_int_loaf loaf = skit_utest_int_loaf_alloc(4);
	int *ptr = skit_utest_int_loaf_ptr(loaf);
	ptr[0] = 1;
	ptr[1] = 3;
	ptr[2] = 3;
	ptr[3] = 7;
	skit_utest_int_loaf_resize(&loaf, 5);
	ptr = skit_utest_int_loaf_ptr(loaf);
	ptr[4] = 3;
	sASSERT_EQ(ptr[0], 1, "%d");
	sASSERT_EQ(ptr[1], 3, "%d");
	sASSERT_EQ(ptr[2], 3, "%d");
	sASSERT_EQ(ptr[3], 7, "%d");
	
	skit_utest_int_slice slice = skit_utest_int_slice_of(loaf.as_slice, 2, SKIT_EOT);
	ptr = skit_utest_int_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3, "%d");
	sASSERT_EQ(ptr[1], 7, "%d");
	sASSERT_EQ(ptr[2], 3, "%d");
	
	slice = skit_utest_int_slice_of(loaf.as_slice, 2, 3);
	ptr = skit_utest_int_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3, "%d");
	sASSERT_EQ(ptr[1], 7, "%d");
	
	printf("  skit_array_unittest passed!\n");
	printf("\n");
}