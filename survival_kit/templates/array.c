
#include "survival_kit/misc.h"
#include "survival_kit/assert.h"
#include "survival_kit/templates/array.h"
#include "survival_kit/string.h"

#include <stdio.h>

SKIT_T(slice) SKIT_T(slice_null)();
SKIT_T(loaf)  SKIT_T(loaf_null)();

SKIT_T(loaf) SKIT_T(loaf_new)();

SKIT_T(loaf) SKIT_T(loaf_copy_carr)(const SKIT_T_ELEM_TYPE array[], size_t length);

SKIT_T(loaf) SKIT_T(loaf_alloc)(size_t length);

SKIT_T(loaf) SKIT_T(loaf_emplace)( void *mem, size_t mem_size );

ssize_t SKIT_T(loaf_len) (SKIT_T(loaf) loaf);
ssize_t SKIT_T(slice_len)(SKIT_T(slice) slice);

skit_utf8c *SKIT_T(loaf_ptr) ( SKIT_T(loaf)  loaf );
skit_utf8c *SKIT_T(slice_ptr)( SKIT_T(slice) slice );

int SKIT_T(loaf_is_null) (SKIT_T(loaf) loaf);
int SKIT_T(slice_is_null)(SKIT_T(slice) slice);

int SKIT_T(slice_check_init)(SKIT_T(slice) slice);
int SKIT_T(loaf_check_init) (SKIT_T(loaf) loaf);

SKIT_T(slice) SKIT_T(slice_of_carr)(const SKIT_T_ELEM_TYPE array[], int length );

SKIT_T(loaf) *SKIT_T(loaf_resize)(SKIT_T(loaf) *loaf, size_t length);

SKIT_T(loaf) *SKIT_T(loaf)(SKIT_T(loaf) *loaf1, SKIT_T(slice) str2);

SKIT_T(loaf) SKIT_T(slice_concat)(SKIT_T(slice) str1, SKIT_T(slice) str2);

SKIT_T(slice) *SKIT_T(slice_bfd_resize)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	ssize_t    new_buf_slice_length);

SKIT_T(slice) *SKIT_T(slice_bfd_append)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	SKIT_T(slice) suffix);

SKIT_T(loaf) SKIT_T(loaf_dup)(SKIT_T(slice) slice);

SKIT_T(slice) SKIT_T(loaf_xfer_carr)(SKIT_T(loaf) *loaf, const SKIT_T_ELEM_TYPE array[]);

SKIT_T(slice) SKIT_T(loaf_xfer_slice)(SKIT_T(loaf) *loaf, SKIT_T(slice) slice);

SKIT_T(slice) SKIT_T(slice_of)(SKIT_T(slice) slice, ssize_t index1, ssize_t index);

SKIT_T_ELEM_TYPE *SKIT_T(loaf_as_carr)(SKIT_T(loaf) loaf);

char *SKIT_T(slice_dup_carr)(SKIT_T(slice) slice);

SKIT_T(loaf) *SKIT_T(loaf_free)(SKIT_T(loaf) *loaf);

/* ------------------------- array misc functions ------------------------- */

SKIT_T(slice) SKIT_T(slice_ltruncate)(const SKIT_T(slice) slice, size_t nelems);
SKIT_T(slice) SKIT_T(slice_rtruncate)(const SKIT_T(slice) slice, size_t nelems);

/*
TODO:
slice_find(slice,elem)
slice_get(slice,index)
slice_set(*slice,index,val)
slice_bsearch(slice,elem)
slice_qsort(*slice) (maybe sorting functions should be their own template?)
*/

