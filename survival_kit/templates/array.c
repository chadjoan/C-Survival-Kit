/* TODO: ideally all of the string.h functions would move here and become
generic.  string.h itself could be a special case of array.h: instantiated
as an array of bytes.  It would still provide extra functions that are 
string specific (ex: newline matching, whitespace trimming).
The documentation should probably stay the same: it is easier to learn the
simpler string API first, and then learn the array API by analogy. */

#include "survival_kit/misc.h"
#include "survival_kit/memory.h"
#include "survival_kit/assert.h"
#include "survival_kit/string.h"

#include <stdio.h>

#ifndef SKIT_T_NAMESPACE
#define SKIT_T_NAMESPACE skit
#define SKIT_T_NAMESPACE_IS_DEFAULT 1
#endif

/*const int SKIT_T(loaf_stride) = sizeof(SKIT_T_ELEM_TYPE);*/

static SKIT_T(slice) SKIT_T(slice_templated)(skit_slice slice)
{
	SKIT_T(slice) tslice;
	tslice.as_skit_slice = slice;
	return tslice;
}

static SKIT_T(loaf) SKIT_T(loaf_templated)(skit_loaf loaf)
{
	SKIT_T(loaf) tloaf;
	tloaf.as_skit_loaf = loaf;
	return tloaf;
}

SKIT_T(slice) SKIT_T(slice_null)() { return SKIT_T(slice_templated)(skit_slice_null()); }
SKIT_T(loaf)  SKIT_T(loaf_null)()  { return SKIT_T(loaf_templated)(skit_loaf_null()); }

SKIT_T(loaf) SKIT_T(loaf_new)()    { return SKIT_T(loaf_templated)(skit_loaf_new()); }

SKIT_T(loaf) SKIT_T(loaf_copy_carr)(const SKIT_T_ELEM_TYPE array[], size_t length)
{
	SKIT_T(loaf) result = SKIT_T(loaf_alloc)(length);
	SKIT_T(loaf_store_carr)(&result,array,length);
	return result;
}

SKIT_T(loaf) SKIT_T(loaf_alloc)(size_t length)
{
	return SKIT_T(loaf_templated)(skit_loaf_alloc(length * sizeof(SKIT_T_ELEM_TYPE)));
}

SKIT_T(loaf) SKIT_T(loaf_emplace)( void *mem, size_t mem_size )
{
	return SKIT_T(loaf_templated)(skit_loaf_emplace(mem, mem_size));
}

ssize_t SKIT_T(loaf_len) (SKIT_T(loaf) loaf)
{
	return skit_loaf_len(loaf.as_skit_loaf) / sizeof(SKIT_T_ELEM_TYPE);
}

ssize_t SKIT_T(slice_len)(SKIT_T(slice) slice)
{
	return skit_slice_len(slice.as_skit_slice) / sizeof(SKIT_T_ELEM_TYPE);
}

SKIT_T_ELEM_TYPE *SKIT_T(loaf_ptr) ( SKIT_T(loaf)  loaf )
{
	return (SKIT_T_ELEM_TYPE*)skit_loaf_ptr(loaf.as_skit_loaf);
}

SKIT_T_ELEM_TYPE *SKIT_T(slice_ptr)( SKIT_T(slice) slice )
{
	return (SKIT_T_ELEM_TYPE*)skit_slice_ptr(slice.as_skit_slice);
}

int SKIT_T(loaf_is_null) (SKIT_T(loaf) loaf)   { return skit_loaf_is_null(loaf.as_skit_loaf); }
int SKIT_T(slice_is_null)(SKIT_T(slice) slice) { return skit_slice_is_null(slice.as_skit_slice); }

int SKIT_T(loaf_check_init) (SKIT_T(loaf) loaf)   { return skit_loaf_check_init(loaf.as_skit_loaf); }
int SKIT_T(slice_check_init)(SKIT_T(slice) slice) { return skit_slice_check_init(slice.as_skit_slice); }

SKIT_T(slice) SKIT_T(slice_of_carr)(const SKIT_T_ELEM_TYPE array[], int length)
{
	return SKIT_T(slice_templated)(skit_slice_of_cstrn((const char*)array, length * sizeof(SKIT_T_ELEM_TYPE)));
}

SKIT_T(loaf) *SKIT_T(loaf_resize)(SKIT_T(loaf) *loaf, size_t length)
{
	return (SKIT_T(loaf)*)skit_loaf_resize((skit_loaf*) loaf, length * sizeof(SKIT_T_ELEM_TYPE));
}

SKIT_T(loaf) *SKIT_T(loaf_append)(SKIT_T(loaf) *loaf1, SKIT_T(slice) str2)
{
	return (SKIT_T(loaf)*)skit_loaf_append((skit_loaf*)loaf1, str2.as_skit_slice);
}

SKIT_T(loaf) *SKIT_T(loaf_put)(SKIT_T(loaf) *loaf1, SKIT_T_ELEM_TYPE elem)
{
	return (SKIT_T(loaf)*)skit_loaf_append((skit_loaf*)loaf1, skit_slice_of_cstrn((char*)&elem, sizeof(elem)));
}

SKIT_T(loaf) SKIT_T(slice_concat)(SKIT_T(slice) str1, SKIT_T(slice) str2)
{
	return SKIT_T(loaf_templated)(skit_slice_concat(str1.as_skit_slice, str2.as_skit_slice));
}

SKIT_T(slice) *SKIT_T(slice_bfd_resize)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	ssize_t       new_buf_slice_length)
{
	return (SKIT_T(slice)*)skit_slice_buffered_resize(
		(skit_loaf*)buffer,
		(skit_slice*)buf_slice,
		new_buf_slice_length * sizeof(SKIT_T_ELEM_TYPE));
}

SKIT_T(slice) SKIT_T(slice_bfd_append)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	SKIT_T(slice) suffix)
{
	return SKIT_T(slice_templated)(skit_slice_buffered_append(
		(skit_loaf*)buffer,
		(skit_slice*)buf_slice,
		suffix.as_skit_slice));
}

SKIT_T_ELEM_TYPE *SKIT_T(slice_bfd_new_el)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice)
{
	size_t new_offset = sSLENGTH(*(skit_slice*)buf_slice);
	skit_slice_buffered_resize(
		(skit_loaf*)buffer,
		(skit_slice*)buf_slice,
		new_offset + sizeof(SKIT_T_ELEM_TYPE));
	return (SKIT_T_ELEM_TYPE*)(sSPTR(*(skit_slice*)buf_slice) + new_offset);
}

SKIT_T(slice) SKIT_T(slice_bfd_put)(
	SKIT_T(loaf)     *buffer,
	SKIT_T(slice)    *buf_slice,
	SKIT_T_ELEM_TYPE elem)
{
	return SKIT_T(slice_templated)(skit_slice_buffered_append(
		(skit_loaf*)buffer,
		(skit_slice*)buf_slice,
		skit_slice_of_cstrn((char*)&elem, sizeof(elem))));
}

SKIT_T(loaf) SKIT_T(loaf_dup)(SKIT_T(slice) slice)
{
	return SKIT_T(loaf_templated)(skit_loaf_dup(slice.as_skit_slice));
}

SKIT_T(slice) SKIT_T(loaf_store_carr)(SKIT_T(loaf) *loaf, const SKIT_T_ELEM_TYPE array[], size_t length)
{
	SKIT_T(slice) carr_as_slice = SKIT_T(slice_of_carr)(array, length);
	return SKIT_T(loaf_store_slice)(loaf, carr_as_slice);
}

SKIT_T(slice) SKIT_T(loaf_xfer_carr)(SKIT_T(loaf) *loaf, const SKIT_T_ELEM_TYPE array[], size_t length)
{
	SKIT_T(slice) carr_as_slice = SKIT_T(slice_of_carr)(array, length);
	return SKIT_T(loaf_xfer_slice)(loaf, carr_as_slice);
}

SKIT_T(slice) SKIT_T(loaf_store_slice)(SKIT_T(loaf) *loaf, SKIT_T(slice) slice)
{
	return SKIT_T(slice_templated)(skit_loaf_store_slice((skit_loaf*)loaf, slice.as_skit_slice));
}

SKIT_T(slice) SKIT_T(loaf_xfer_slice)(SKIT_T(loaf) *loaf, SKIT_T(slice) slice)
{
	return SKIT_T(slice_templated)(skit_loaf_assign_slice((skit_loaf*)loaf, slice.as_skit_slice));
}

SKIT_T(slice) SKIT_T(slice_of)(SKIT_T(slice) slice, ssize_t index1, ssize_t index2)
{
	/* Handle SKIT_EOT by preventing multiplication on it. */
	ssize_t m_index2;
	if ( index2 == SKIT_EOT )
		m_index2 = index2;
	else
		m_index2 = index2 * sizeof(SKIT_T_ELEM_TYPE);
	
	/* With EOT safe, we can forward the call. */
	return SKIT_T(slice_templated)(skit_slice_of(
		slice.as_skit_slice,
		index1 * sizeof(SKIT_T_ELEM_TYPE),
		m_index2));
}

SKIT_T_ELEM_TYPE *SKIT_T(loaf_as_carr)(SKIT_T(loaf) loaf)
{
	return SKIT_T(loaf_ptr)(loaf);
}

SKIT_T_ELEM_TYPE *SKIT_T(slice_dup_carr)(SKIT_T(slice) slice)
{
	ssize_t length = SKIT_T(slice_len)(slice);
	size_t memsize = length * sizeof(SKIT_T_ELEM_TYPE);
	SKIT_T_ELEM_TYPE *result = skit_malloc(memsize);
	memcpy(result, SKIT_T(slice_ptr)(slice), memsize);
	/* This differs from the string.h version: it does not append a null byte. */
	return result;
}

SKIT_T(loaf) SKIT_T(loaf_free)(SKIT_T(loaf) *loaf)
{
	return SKIT_T(loaf_templated)(skit_loaf_free((skit_loaf*)loaf));
}

/* ------------------------- array misc functions ------------------------- */

SKIT_T(slice) SKIT_T(slice_ltruncate)(const SKIT_T(slice) slice, size_t nelems)
{
	return SKIT_T(slice_templated)(skit_slice_ltruncate(slice.as_skit_slice, nelems * sizeof(SKIT_T_ELEM_TYPE)));
}

SKIT_T(slice) SKIT_T(slice_rtruncate)(const SKIT_T(slice) slice, size_t nelems)
{
	return SKIT_T(slice_templated)(skit_slice_rtruncate(slice.as_skit_slice, nelems * sizeof(SKIT_T_ELEM_TYPE)));
}

SKIT_T_ELEM_TYPE *SKIT_T(slice_index)( const SKIT_T(slice) slice, size_t index )
{
	SKIT_T_ELEM_TYPE *ptr = SKIT_T(slice_ptr)( slice );
	return &ptr[index];
}

SKIT_T_ELEM_TYPE *SKIT_T(loaf_index)( const SKIT_T(loaf) loaf, size_t index )
{
	SKIT_T_ELEM_TYPE *ptr = SKIT_T(loaf_ptr)( loaf );
	return &ptr[index];
}

void SKIT_T(slice_bfd_filter) (
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	void *predicate_context,
	int  (*predicate)( void *predicate_context, SKIT_T_ELEM_TYPE *elem )
)
{
	SKIT_T_ELEM_TYPE *ptr = SKIT_T(slice_ptr)(*buf_slice);
	ssize_t len = SKIT_T(slice_len)(*buf_slice);

	// Make sure that the given slice actually exists within the given buffer.
	sASSERT_EQ(ptr, SKIT_T(loaf_ptr)(*buffer));
	sASSERT_LE(len, SKIT_T(loaf_len)(*buffer));

	// At the end of the loop, trailing_idx will be the length of the new slice.
	ssize_t trailing_idx = 0;
	ssize_t leading_idx = 0;
	while ( trailing_idx < len )
	{
		SKIT_T_ELEM_TYPE e1 = ptr[trailing_idx];
		if ( !predicate(predicate_context, &e1) )
		{
			// We found a hole!
			// Find the next passing element and place it here.
			SKIT_T_ELEM_TYPE e2;

			if ( leading_idx < trailing_idx+1 )
				leading_idx = trailing_idx+1;

			while ( leading_idx < len )
			{
				e2 = ptr[leading_idx];
				if ( predicate(predicate_context, &e2) )
					break;
				leading_idx++;
			}

			// If there are no more remaining elements, then we are
			// have finished condensing the slice.
			if ( leading_idx >= len )
				break; // Done!

			// There is a remaining element.  Stick it in the hole.
			ptr[trailing_idx] = e2;

			// Don't leak resources: place the 'hole' where we just got e2 from.
			ptr[leading_idx] = e1;
		}

		trailing_idx++;
	}
	*buf_slice =
		SKIT_T(slice_of)(*buf_slice, 0, trailing_idx);
}

/*
TODO:
slice_find(slice,elem)
slice_get(slice,index)
slice_set(*slice,index,val)
loaf_push(*loaf, val)  (See also: skit_T_loaf_put, which exists and might be what you really want.)
loaf_pop(*loaf, val)
slice_bsearch(slice,elem)
slice_qsort(*slice) (maybe sorting functions should be their own template?)
*/


#ifdef SKIT_T_NAMESPACE_IS_DEFAULT
#undef SKIT_T_NAMESPACE
#undef SKIT_T_NAMESPACE_IS_DEFAULT
#endif

