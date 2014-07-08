
#ifdef __DECC
#pragma module skit_array_builtins
#endif

#define SKIT_T_HEADER "survival_kit/templates/array.h"
#include "survival_kit/array_builtins.h"
#undef SKIT_T_HEADER
#undef SKIT_ARRAY_BUILTINS_INCLUDED

#define SKIT_T_HEADER "survival_kit/templates/array.c"
#include "survival_kit/array_builtins.h"
#undef SKIT_T_HEADER

#include "survival_kit/assert.h"
#include "survival_kit/feature_emulation.h"

#include <stdio.h>

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE int
#define SKIT_T_NAME utest_int
#define SKIT_T_FUNC_ATTR static
#include "survival_kit/templates/array.h"
#include "survival_kit/templates/array.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_FUNC_ATTR

static void skit_slice_sanity_check()
{
	SKIT_USE_FEATURE_EMULATION;
	
	skit_loaf loaf = skit_loaf_alloc(4);
	skit_utf8c *ptr = skit_loaf_ptr(loaf);
	ptr[0] = 1;
	ptr[1] = 3;
	ptr[2] = 3;
	ptr[3] = 7;
	skit_loaf_resize(&loaf, 5);
	ptr = skit_loaf_ptr(loaf);
	ptr[4] = 3;
	sASSERT_EQ(ptr[0], 1);
	sASSERT_EQ(ptr[1], 3);
	sASSERT_EQ(ptr[2], 3);
	sASSERT_EQ(ptr[3], 7);
	sASSERT_EQ(ptr[4], 3);
	
	skit_slice slice = skit_slice_of(loaf.as_slice, 2, SKIT_EOT);
	ptr = skit_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3);
	sASSERT_EQ(ptr[1], 7);
	sASSERT_EQ(ptr[2], 3);
	
	slice = skit_slice_of(loaf.as_slice, 2, 4);
	ptr = skit_slice_ptr(slice);
	sASSERT_EQ(sSLENGTH(slice), 2);
	sASSERT_EQ(ptr[0], 3);
	sASSERT_EQ(ptr[1], 7);
	
	skit_slice_buffered_append( &loaf, &slice, sSLICE("\x07") );
	ptr = skit_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3);
	sASSERT_EQ(ptr[1], 7);
	sASSERT_EQ(ptr[2], 7);
	
	ptr = skit_loaf_ptr(loaf);
	sASSERT_EQ(ptr[0], 1);
	sASSERT_EQ(ptr[1], 3);
	sASSERT_EQ(ptr[2], 3);
	sASSERT_EQ(ptr[3], 7);
	sASSERT_EQ(ptr[4], 7);
	
	skit_loaf_free(&loaf);
}

static void skit_array_bfd_new_el_test()
{
	SKIT_USE_FEATURE_EMULATION;
	
	skit_utest_int_loaf loaf = skit_utest_int_loaf_alloc(4);
	skit_utest_int_slice slice = skit_utest_int_slice_of(loaf.as_slice,0,0);
	sASSERT_EQ(skit_utest_int_slice_len(slice),0);
	int *ptr = skit_utest_int_loaf_ptr(loaf);
	int *new_elem = NULL;
	
	new_elem = skit_utest_int_slice_bfd_new_el(&loaf, &slice);
	sASSERT_EQ(skit_utest_int_slice_len(slice),1);
	*new_elem = 42;
	sASSERT_EQ(*new_elem, ptr[0]);
	
	new_elem = skit_utest_int_slice_bfd_new_el(&loaf, &slice);
	sASSERT_EQ(skit_utest_int_slice_len(slice),2);
	*new_elem = 9;
	sASSERT_EQ(*new_elem, ptr[1]);
	
	new_elem = skit_utest_int_slice_bfd_new_el(&loaf, &slice);
	sASSERT_EQ(skit_utest_int_slice_len(slice),3);
	*new_elem = -13;
	sASSERT_EQ(*new_elem, ptr[2]);
	
	skit_utest_int_loaf_free(&loaf);
}

static int skit_array_filter_rm2( void *ctx_that_is_null, int *elem )
{
	if ( (*elem) == 2 )
		return 0;
	else
		return 1;
}

static void skit_array_filter_test()
{
	SKIT_USE_FEATURE_EMULATION;

	skit_utest_int_loaf loaf = skit_utest_int_loaf_alloc(4);
	int *ptr = skit_utest_int_loaf_ptr(loaf);
	ptr[0] = 1;
	ptr[1] = 2;
	ptr[2] = 3;
	ptr[3] = 4;

	skit_utest_int_slice slice = skit_utest_int_slice_of(loaf.as_slice, 0, 3);
	sASSERT_EQ(skit_utest_int_slice_len(slice), 3);

	skit_utest_int_slice_bfd_filter(&loaf, &slice, NULL, &skit_array_filter_rm2);
	sASSERT_EQ(skit_utest_int_slice_len(slice), 2);
	sASSERT_EQ(ptr[0], 1); // sorted, in slice
	sASSERT_EQ(ptr[1], 3); // sorted, in slice
	sASSERT_EQ(ptr[2], 2); // unsorted, not sliced
	sASSERT_EQ(ptr[3], 4); // unsorted, not sliced

	skit_utest_int_loaf_free(&loaf);
}

void skit_array_unittest()
{
	SKIT_USE_FEATURE_EMULATION;
	printf("skit_array_unittest()\n");
	
	sTRACE(skit_slice_sanity_check());
	
	skit_utest_int_loaf loaf = skit_utest_int_loaf_alloc(4);
	int *ptr = skit_utest_int_loaf_ptr(loaf);
	ptr[0] = 1;
	ptr[1] = 3;
	ptr[2] = 3;
	ptr[3] = 7;
	skit_utest_int_loaf_resize(&loaf, 5);
	ptr = skit_utest_int_loaf_ptr(loaf);
	ptr[4] = 3;
	sASSERT_EQ(ptr[0], 1);
	sASSERT_EQ(ptr[1], 3);
	sASSERT_EQ(ptr[2], 3);
	sASSERT_EQ(ptr[3], 7);
	sASSERT_EQ(ptr[4], 3);
	
	skit_utest_int_slice slice = skit_utest_int_slice_of(loaf.as_slice, 2, SKIT_EOT);
	ptr = skit_utest_int_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3);
	sASSERT_EQ(ptr[1], 7);
	sASSERT_EQ(ptr[2], 3);
	
	slice = skit_utest_int_slice_of(loaf.as_slice, 2, 4);
	ptr = skit_utest_int_slice_ptr(slice);
	sASSERT_EQ(skit_utest_int_slice_len(slice), 2);
	sASSERT_EQ(ptr[0], 3);
	sASSERT_EQ(ptr[1], 7);
	
	skit_utest_int_slice_bfd_put( &loaf, &slice, 7 );
	ptr = skit_utest_int_slice_ptr(slice);
	sASSERT_EQ(ptr[0], 3);
	sASSERT_EQ(ptr[1], 7);
	sASSERT_EQ(ptr[2], 7);
	
	ptr = skit_utest_int_loaf_ptr(loaf);
	sASSERT_EQ(ptr[0], 1);
	sASSERT_EQ(ptr[1], 3);
	sASSERT_EQ(ptr[2], 3);
	sASSERT_EQ(ptr[3], 7);
	sASSERT_EQ(ptr[4], 7);
	
	skit_utest_int_loaf_put(&loaf, 0);
	ptr = skit_utest_int_loaf_ptr(loaf);
	sASSERT_EQ(ptr[0], 1);
	sASSERT_EQ(ptr[1], 3);
	sASSERT_EQ(ptr[2], 3);
	sASSERT_EQ(ptr[3], 7);
	sASSERT_EQ(ptr[4], 7);
	sASSERT_EQ(ptr[5], 0);
	
	skit_utest_int_loaf_put(&loaf, 8);
	ptr = skit_utest_int_loaf_ptr(loaf);
	sASSERT_EQ(ptr[0], 1);
	sASSERT_EQ(ptr[1], 3);
	sASSERT_EQ(ptr[2], 3);
	sASSERT_EQ(ptr[3], 7);
	sASSERT_EQ(ptr[4], 7);
	sASSERT_EQ(ptr[5], 0);
	sASSERT_EQ(ptr[6], 8);
	
	int *elem = skit_utest_int_loaf_index(loaf, 6);
	sASSERT_EQ(*elem, 8);
	
	skit_utest_int_loaf_free(&loaf);
	
	sTRACE(skit_array_bfd_new_el_test());
	sTRACE(skit_array_filter_test());
	
	printf("  skit_array_unittest passed!\n");
	printf("\n");
}
