
#ifdef __DECC
#pragma module skit_string
#endif

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h> /* For ssize_t */
#include <stddef.h> /* For ptrdiff_t */

#include "survival_kit/string.h"
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"

#define SKIT_DO_STRING_DEBUG 0
#if SKIT_DO_STRING_DEBUG != 0
#define DEBUG(x) (x)
#else
#define DEBUG(x) ((void)0)
#endif

/* 
-----------===== .meta layout =====-----------
The hi 8 bits of the 'meta' member is layed out like so:
  1 0 1 A A S S S

The highest 3 bits are always 1 0 1.  This causes the meta member to
  always be /very/ negative, and at the same time not entirely made of set
  bits.  If a string is provided that doesn't have those 3 bits in that
  arrangement, then any string handling function working with it can assume 
  that the string wans't propertly initialized and throw an exception.
  This is accessed internally with the META_CHECK_XXX defines.

The A bits (standing for 'allocation type' here) specify how the string
  content is stored:
  It has the following meanings:
  0 - The string is either a loaf or a slice of a loaf.
  1 - The string data is in a user-provided location, probably the stack.
  2 - The string is a slice of a C style string.
  Strings that are a slice of a C-style string will have a chars_handle
  pointer that points directly to string data.
  Strings that are a loaf or a slice of a loaf will have a chars_handle
  that points to a pointer that points to the string data.  This is the
  case for both 0 and 1.  The specifics of this arrangement are described
  in the ".chars_handle layout" section.
  The 1 value is used to determine if skit_free is called or not on the
  string data when skit_loaf_free is called.  It is otherwise equivalent
  to the 0 case.
  The allocation type always refers to the original block of memory held by a
  loaf.  It does not refer to handle-referenced memory that is created to
  satiate growth: that will always be dynamically allocated with skit_malloc.
  See also the skit_string_alloc_type definitions.

The three S bits are the stride of the array.  Currently, this field has the
  following possible values with the following meanings:
  1 - A utf8 encoded string.
  2 - A utf16 encoded string.
  4 - A utf32 encoded string.
  This is accessed internally with the META_STRIDE_XXX defines.
  (currently populated, but not used)

The rest of the bytes in the 'meta' member are the length of the string and its
  offset (if it's a slice).  These are accessed internally with the 
  META_LENGTH_XXX and META_OFFSET_XXX defines.  They are 28 bits each: a split
  between the remaining bits in the meta member.  A loaf will always have an
  offset of 0.  This does mean that strings can't be longer than ~268MB.
  
-----------===== .chars_handle layout =====-----------
The .chars_handle can have one of 3 meanings:

(1) It is a pointer to a pointer to the string data.  The second pointer is
called a "handle" and is allocated by loaves on an as-needed basis.

(2) It is a pointer to a null pointer, with the string data following the
null pointer/handle contiguously in memory.  

(3) It is a pointer to the string data.  The string data is a traditional
nul-terminated C string.

Cases 1&2 can be distinguished from case 3 by using the 'A' bits in the string's
metadata.  This is made easier by using the internal macro SKIT_SLICE_IS_CSTR()
macro.  

These complications are introduced because the loaf may move the string data's 
memory during resize operations and any slices that point directly to the old 
data would subsequently point to free'd/invalid memory.  To prevent this kind
of memory corruption, slices of a loaf instead point to an immovable pointer
(the handle) that is allocated and updated by the loaf and shared by all 
slices.  This is represented by case 1 above.  With the extra layer of 
indirection, the loaf is free to move the string data, because everything 
accessing it will always point to the valid and immovable pointer that always 
points to the latest location of the string data.  This kind of 
double-indirection can be slow, so case 2 is provided as an optimization, and
is distinguishable from case 1 because valid pointers are never null.  Case 2
should allow slices to freshly allocated loaves to access the data nearly as 
fast as if the extra indirection did not exist at all.  If code never needs
to resize a loaf (ex: parsing a large block of text with a known size) then
the access to the string data should still be very fast.

Notably, case 3 does not allow for the C string to be reallocated because
traditional C strings do not do the handle-management thing that loaves do.
It is mostly intended to make working with string literals easier, and
possibly also some lightweight work with short-lived C strings used in a
very temporary manner.
 */

#define META_STRIDE_SHIFT (sizeof(skit_string_meta)*8 - 8)
#define META_STRIDE_MASK  (0x0700000000000000ULL)

#define META_CHECK_SHIFT  (sizeof(skit_string_meta)*8 - 3)
#define META_CHECK_MASK   (0xE000000000000000ULL)
#define META_CHECK_VAL    (0xA000000000000000ULL)

#define META_ALLOC_SHIFT  (sizeof(skit_string_meta)*8 - 5)
#define META_ALLOC_MASK   (0x1800000000000000ULL)

#define META_LENGTH_MASK  (0x000000000FFFFFFFULL)
#define META_LENGTH_SHIFT (0)

#define META_OFFSET_MASK  (0x00FFFFFFF0000000ULL)
#define META_OFFSET_SHIFT (28)

enum skit_string_alloc_type
{
	skit_string_alloc_type_malloc = 0,  /* skit_malloc, to be exact. */
	skit_string_alloc_type_user   = 1,  /* Probably stack memory.  Don't free. */
	skit_string_alloc_type_cstr   = 2,  /* Handle doesn't exist.  Don't resize or free. */
};
typedef enum skit_string_alloc_type skit_string_alloc_type;

#define SKIT_STRING_GET_ALLOC_TYPE(meta)     (((meta) & META_ALLOC_MASK) >> META_ALLOC_SHIFT)
#define SKIT_STRING_SET_ALLOC_TYPE(meta,val) \
	( (meta) = \
		(((meta) & ~META_ALLOC_MASK) | \
		 (((0ULL | (val)) << META_ALLOC_SHIFT) & META_ALLOC_MASK)) \
	)

#define SKIT_SLICE_IS_CSTR(slice) (SKIT_STRING_GET_ALLOC_TYPE((slice).meta) == skit_string_alloc_type_cstr)

#define skit_string_init_meta() \
	(META_CHECK_VAL | (1ULL << META_STRIDE_SHIFT))

static void skit_slice_set_length(skit_slice *slice, uint64_t len)
{
	slice->meta = 
		(slice->meta & ~META_LENGTH_MASK) | 
		((len << META_LENGTH_SHIFT) & META_LENGTH_MASK);
}

static void skit_slice_set_offset(skit_slice *slice, uint64_t offset)
{
	slice->meta = 
		(slice->meta & ~META_OFFSET_MASK) | 
		((offset << META_OFFSET_SHIFT) & META_OFFSET_MASK);
}

static size_t skit_slice_get_offset(skit_slice slice)
{
	return (slice.meta & META_OFFSET_MASK) >> META_OFFSET_SHIFT;
}

/* ------------------------------------------------------------------------- */

/* 
This is an important step in all loaf allocations:
This function defines how memory is stored by loaves, including where the
handle pointer goes and ensuring that there is always a null byte at the end.
It should be called whenever a new loaf is needed.  The memory should have
already been allocated by the point this is called, and all loaves must have
their allocation type tracked.  Tracking allocation is important for ensuring
correct cleanup of resources (so that malloc'd memory is free'd, and
non-malloc'd memory isn't free'd).
*/
static skit_loaf skit_loaf_emplace_internal( 
	void *mem, size_t mem_size, skit_string_alloc_type type )
{
	skit_loaf result = skit_loaf_null();
	
	/* Give the loaf access to the memory.  The first pointer's worth is */
	/*   turned into the handle itself, followed by the string data, */
	/*   followed by a null-terminating byte that is not counted in the */
	/*   loaf's length. */
	result.chars_handle = (skit_utf8c*)mem;
	memset( result.chars_handle, 0, sizeof(skit_utf8c*) );
	result.chars_handle[mem_size-1] = '\0';
	
	/* Set the length to agree with the string data. */
	skit_slice_set_length(&result.as_slice, mem_size - (sizeof(skit_utf8c*)+1));
	
	/* Remember things like user-supplied memory. */
	SKIT_STRING_SET_ALLOC_TYPE(result.as_slice.meta, type);
	
	return result;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_null()
{
	skit_slice result;
	result.chars_handle = NULL;
	result.meta         = skit_string_init_meta();
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_null()
{
	skit_loaf result;
	result.as_slice = skit_slice_null();
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_new()
{
	return skit_loaf_alloc(0);
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_copy_cstr(const char *cstr)
{
	skit_loaf result = skit_loaf_alloc(strlen(cstr));
	skit_loaf_store_cstr(&result,cstr);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_alloc(size_t length)
{
	size_t mem_size = SKIT_LOAF_EMPLACEMENT_OVERHEAD + length;
	void *mem = skit_malloc(mem_size);
	return skit_loaf_emplace_internal(mem, mem_size, skit_string_alloc_type_malloc);
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_emplace( void *mem, size_t mem_size )
{
	return skit_loaf_emplace_internal(mem, mem_size, skit_string_alloc_type_user);
}

static void skit_loaf_emplace_test()
{
	char mem[128];
	size_t mem_size = sizeof(mem);
	skit_loaf loaf = skit_loaf_emplace(mem, mem_size);
	sASSERT_EQ( mem_size, sLLENGTH(loaf) + SKIT_LOAF_EMPLACEMENT_OVERHEAD );
	sASSERT( sLPTR(loaf) != NULL );
	sASSERT( mem <= (char*)sLPTR(loaf) );
	sASSERT( (char*)sLPTR(loaf) < mem+mem_size );
	skit_loaf_free(&loaf);
	printf("  skit_loaf_emplace_test finished.\n");
}

static void skit_loaf_on_stack_test()
{
	SKIT_LOAF_ON_STACK(loaf, 32);
	sASSERT( sLPTR(loaf) != NULL );
	sASSERT_EQ( sLLENGTH(loaf), 32 );
	skit_loaf_free(&loaf);
	printf("  skit_loaf_on_stack_test finished.\n");
}

/* ------------------------------------------------------------------------- */

ssize_t skit_slice_len(skit_slice slice)
{
	return (slice.meta & META_LENGTH_MASK) >> META_LENGTH_SHIFT;
}

ssize_t skit_loaf_len (skit_loaf loaf)   { return skit_slice_len(loaf.as_slice);  }

static void skit_slice_len_test()
{
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT_EQ(skit_loaf_len(loaf), 10);
	printf("  skit_slice_len_test finished.\n");
}

/* ------------------------------------------------------------------------- */

skit_utf8c *skit_loaf_ptr( skit_loaf loaf )
{
	if ( loaf.chars_handle == NULL )
		return NULL;
	
	/* Loaves will never have the skit_string_alloc_type_cstr allocation type. */
	/* That wouldn't make sense. */
	/* So don't bother the extra cycles to check it. */
	
	skit_utf8c *handle = *((skit_utf8c**)loaf.chars_handle);
	if ( handle == NULL )
		return loaf.chars_handle + sizeof(skit_utf8c*);
	else
		return handle;
}

skit_utf8c *skit_slice_ptr( skit_slice slice )
{
	if ( slice.chars_handle == NULL )
		return NULL;

	if ( SKIT_SLICE_IS_CSTR(slice) )
	{
		return slice.chars_handle + skit_slice_get_offset(slice);
	}
	else
	{
		skit_utf8c *handle = *((skit_utf8c**)slice.chars_handle);
		if ( handle == NULL )
			return slice.chars_handle + sizeof(handle) + skit_slice_get_offset(slice);
		else
			return handle + skit_slice_get_offset(slice);
	}
}

static void skit_slice_ptr_test()
{
	skit_loaf  loaf = skit_loaf_copy_cstr("foobarbaz");
	skit_slice slice = skit_slice_of(loaf.as_slice, 3, 6);
	sASSERT_EQ(sSPTR(slice) - sLPTR(loaf), 3);
	sASSERT_EQS(slice, sSLICE("bar"));
	skit_loaf_free(&loaf);
	printf("  skit_slice_ptr_test finished.\n");
}

/* ------------------------------------------------------------------------- */

int skit_loaf_is_null(skit_loaf loaf)
{
	if ( loaf.chars_handle == NULL )
		return 1;
	else
		return 0;
}

int skit_slice_is_null(skit_slice slice)
{
	if ( slice.chars_handle == NULL )
		return 1;
	else
		return 0;
}

static void skit_slice_is_null_test()
{
	skit_loaf  nloaf  = skit_loaf_null();
	skit_slice nslice = skit_slice_null();
	skit_loaf  loaf   = skit_loaf_copy_cstr("foo");
	skit_slice slice  = skit_slice_of(loaf.as_slice, 0, 2);
	sASSERT_EQ(skit_loaf_is_null(nloaf),   1);
	sASSERT_EQ(skit_slice_is_null(nslice), 1);
	sASSERT_EQ(skit_loaf_is_null(loaf),    0);
	sASSERT_EQ(skit_slice_is_null(slice),  0);
	skit_loaf_free(&loaf);
	printf("  skit_slice_is_null_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_loaf_check_init (skit_loaf loaf)   { return skit_slice_check_init(loaf.as_slice);  }

int skit_slice_check_init(skit_slice slice)
{
	if ( META_CHECK_VAL == (slice.meta & META_CHECK_MASK) )
		return 1;
	return 0;
}

static void skit_slice_check_init_test()
{
	skit_slice slice;
	if ( skit_slice_check_init(slice) )
		printf("  skit_slice_check_init: False positive!\n");
	else
		printf("  skit_slice_check_init: Caught an uninitialized slice!\n");
	printf("  skit_slice_check_init_test finished.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of_cstrn(const char *cstr, int length )
{
	skit_slice result = skit_slice_null();
	result.chars_handle = (skit_utf8c*)cstr;
	skit_slice_set_length(&result, length);
	SKIT_STRING_SET_ALLOC_TYPE(result.meta, skit_string_alloc_type_cstr);
	return result;
}

static void skit_slice_of_cstrn_test()
{
	skit_slice slice = skit_slice_of_cstrn("foo",3);
	sASSERT_EQ(skit_slice_len(slice), 3);
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "foo");
	printf("  skit_slice_of_cstrn_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of_cstr(const char *cstr)
{
	return skit_slice_of_cstrn(cstr, strlen(cstr));
}

static void skit_slice_of_cstr_test()
{
	skit_slice slice = skit_slice_of_cstr("foo");
	sASSERT_EQ(skit_slice_len(slice), 3);
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "foo");
	printf("  skit_slice_of_cstr_test passed.\n");
}

static void skit_slice_sSLICE_test()
{
	skit_slice slice = sSLICE("foo");
	sASSERT_EQ(skit_slice_len(slice), 3);
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "foo");
	
	const char *cstr_ptr = "Hello world!";
	slice = sSLICE(cstr_ptr);
	sASSERT_EQ(skit_slice_len(slice), 12);
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "Hello world!");
	
	// Arrays may behave strangely:
	char array1[sizeof(void*)];
	memset(array1, '\0', sizeof(void*));
	slice = sSLICE(array1);
	sASSERT_EQ(skit_slice_len(slice), 0);
	
	char array2[sizeof(void*)-1];
	memset(array2, '\0', sizeof(void*)-1);
	slice = sSLICE(array2);
	sASSERT_EQ(skit_slice_len(slice), sizeof(array2)-1);
	sASSERT_NE(skit_slice_len(slice), 0);

	printf("  skit_slice_sSLICE_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length)
{
	skit_utf8c *loaf_chars = sLPTR(*loaf);
	sASSERT(loaf != NULL);
	sASSERT(loaf_chars != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf), "'loaf' was not initialized.");
	
	skit_utf8c **handle_ptr = (skit_utf8c**)loaf->chars_handle;
	if ( *handle_ptr == NULL )
	{
		size_t old_length = sLLENGTH(*loaf);
		if ( old_length < length )
		{
			/* grow operation */
			*handle_ptr = (skit_utf8c*)skit_malloc(length+1);
			memcpy(*handle_ptr, loaf->chars_handle + sizeof(skit_utf8c*), old_length);
			(*handle_ptr)[old_length] = '\0';
			(*handle_ptr)[length] = '\0';
			
			/* Do not do the below line. */
			/* Allocation type refers to the original ptr.  That allocation */
			/*   type will never change throughout the life of the loaf. */
			/*SKIT_STRING_SET_ALLOC_TYPE(loaf->meta, skit_string_alloc_type_malloc);*/
		}
		else
		{
			/* shrink operation: no allocation required */
			
			/* Note: realloc doesn't seem to guarantee that memory won't move 
			during shrink operations, so we shouldn't try to reclaim the memory
			next to the current handle because we could lose our immovable 
			handle that way. */
			
			loaf->chars_handle[sizeof(skit_utf8c*)+length] = '\0';
		}
	}
	else
	{
		*handle_ptr = (skit_utf8c*)skit_realloc(*handle_ptr, length+1);
		(*handle_ptr)[length] = '\0';
	}
	skit_slice_set_length(&loaf->as_slice, length);
	
	return loaf;
}

static void skit_loaf_resize_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello world!");
	skit_loaf_resize(&loaf, 5);
	sASSERT_EQ_CSTR("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_resize(&loaf, 1024);
	sASSERT_EQ_CSTR("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_resize(&loaf, 512);
	sASSERT_EQ_CSTR("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_resize(&loaf, 1028);
	sASSERT_EQ_CSTR("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_slice str2)
{
	size_t len1;
	size_t len2;
	/* Don't do this:
	skit_utf8c *loaf1_chars = sLPTR(*loaf1);
	skit_utf8c *str2_chars = sSPTR(str2);
	The cached values will become invalid after the resize op.
	*/
	sASSERT(loaf1 != NULL);
	sASSERT(sLPTR(*loaf1) != NULL);
	sASSERT(sSPTR(str2) != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf1), "'loaf1' was not initialized.");
	sASSERT_MSG(skit_slice_check_init(str2), "'str2' was not initialized.");
	
	len2 = skit_slice_len(str2);
	if ( len2 == 0 )
		return loaf1;
	
	len1 = skit_loaf_len(*loaf1);
	
	loaf1 = skit_loaf_resize(loaf1, len1+len2);
	memcpy(sLPTR(*loaf1) + len1, sSPTR(str2), len2);
	/* setlen was already handled by skit_loaf_resize. */
	
	return loaf1;
}

static void skit_loaf_append_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	skit_loaf_append(&loaf, sSLICE(" world!"));
	sASSERT_EQ_CSTR("Hello world!", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_append_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_slice_concat(skit_slice str1, skit_slice str2)
{
	size_t len1;
	size_t len2;
	skit_loaf result;
	sASSERT(sSPTR(str1) != NULL);
	sASSERT(sSPTR(str2) != NULL);
	sASSERT(skit_slice_check_init(str1));
	sASSERT(skit_slice_check_init(str2));
	
	len1 = skit_slice_len(str1);
	len2 = skit_slice_len(str2);
	
	result = skit_loaf_alloc(len1+len2);
	memcpy(sLPTR(result),      sSPTR(str1), len1);
	memcpy(sLPTR(result)+len1, sSPTR(str2), len2);
	/* setlen was already handled by skit_loaf_alloc. */
	
	return result;
}

static void skit_slice_concat_test()
{
	skit_loaf  orig  = skit_loaf_copy_cstr("Hello world!");
	skit_slice slice = skit_slice_of(orig.as_slice, 0, 6);
	skit_loaf  newb  = skit_slice_concat(slice, orig.as_slice);
	sASSERT_EQ_CSTR("Hello Hello world!", skit_loaf_as_cstr(newb));
	skit_loaf_free(&orig);
	skit_loaf_free(&newb);
	printf("  skit_slice_concat_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice *skit_slice_buffered_resize(
	skit_loaf  *buffer,
	skit_slice *buf_slice,
	ssize_t    new_buf_slice_length)
{
	ssize_t buffer_length;
	skit_utf8c *rbound;
	skit_utf8c *lbound;
	skit_utf8c *new_rbound;
	skit_utf8c *buffer_chars = sLPTR(*buffer);
	skit_utf8c *buf_slice_chars = sSPTR(*buf_slice);
	sASSERT(buffer != NULL);
	sASSERT(buf_slice != NULL);
	sASSERT(buffer_chars != NULL);
	sASSERT(buf_slice_chars != NULL);
	
	buffer_length    = skit_loaf_len(*buffer);
	lbound = buffer_chars;
	rbound = lbound + buffer_length;
	sASSERT_MSG(lbound <= buf_slice_chars && buf_slice_chars <= rbound, 
		"The buf_slice given is not a substring of the given buffer.");
	
	new_rbound = buf_slice_chars + new_buf_slice_length;
	if ( new_rbound > rbound )
	{
		ssize_t new_buffer_length = new_rbound - buffer_chars;
		
		/* Resize to (new_buffer_length * 1.5) */
		new_buffer_length = (new_buffer_length * 3) / 2;
		skit_loaf_resize( buffer, new_buffer_length );
	}
	
	skit_slice_set_length(buf_slice, new_buf_slice_length);
	
	return buf_slice;
}

static void skit_slice_buffered_resize_test()
{
	skit_loaf buffer = skit_loaf_alloc(5);
	skit_slice slice = skit_slice_of(buffer.as_slice, 2, 4);
	sASSERT_EQ(skit_slice_len(slice), 2);
	skit_slice_buffered_resize(&buffer, &slice, 5);
	sASSERT(skit_loaf_len(buffer) >= 6);
	sASSERT_EQ(skit_slice_len(slice), 5);
	sASSERT_EQ(sSPTR(slice)[5], '\0');
	skit_loaf_free(&buffer);
	printf("  skit_slice_buffered_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_buffered_append(
	skit_loaf  *buffer,
	skit_slice *buf_slice,
	skit_slice suffix)
{
	ssize_t suffix_length;
	ssize_t buf_slice_length;
	/* We don't need to check sLPTR(*buffer) and sSPTR(*buf_slice) because 
	 *  skit_slice_buffered_resize will do that. */
	sASSERT(buf_slice != NULL);
	sASSERT(sSPTR(suffix) != NULL);
	
	suffix_length = skit_slice_len(suffix);
	buf_slice_length = skit_slice_len(*buf_slice);
	skit_slice_buffered_resize(buffer, buf_slice, buf_slice_length + suffix_length);
	memcpy((void*)(sSPTR(*buf_slice) + buf_slice_length), (void*)sSPTR(suffix), suffix_length);
	
	return *buf_slice;
}

static void skit_slice_buffered_append_test()
{
	skit_loaf  buffer = skit_loaf_alloc(5);
	skit_slice accumulator = skit_slice_of(buffer.as_slice, 0, 0);
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("foo"));
	sASSERT_EQ(skit_loaf_len(buffer), 5);
	sASSERT_EQ(skit_slice_len(accumulator), 3);
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("bar"));
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(buffer), "foobar");
	sASSERT_GE(skit_loaf_len(buffer), 6);
	skit_loaf_free(&buffer);
	printf("  skit_slice_buffered_append_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_dup(skit_slice slice)
{
	size_t length;
	skit_loaf result;
	sASSERT(sSPTR(slice) != NULL);
	sASSERT(skit_slice_check_init(slice));
	
	length = skit_slice_len(slice);
	result = skit_loaf_alloc(length);
	memcpy(sLPTR(result), sSPTR(slice), length);
	/* The call to skit_loaf_alloc will have already handled setlen */
	return result;
}

static void skit_loaf_dup_test()
{
	skit_loaf foo = skit_loaf_copy_cstr("foo");
	skit_slice slice = skit_slice_of(foo.as_slice, 0, 0);
	skit_loaf bar = skit_loaf_dup(slice);
	sASSERT(sLPTR(foo) != sLPTR(bar));
	skit_loaf_store_cstr(&bar, "bar");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
	printf("  skit_loaf_dup_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_loaf_store_cstr(skit_loaf *loaf, const char *cstr)
{
	sASSERT( loaf != NULL );
	
	if ( cstr == NULL )
		return skit_slice_null();
	else
	{
		ssize_t length = strlen(cstr);
		
		if ( skit_loaf_is_null(*loaf) )
			*loaf = skit_loaf_alloc(length);
		
		if ( length > sLLENGTH(*loaf) )
			skit_loaf_resize(loaf, length);
		strcpy((char*)sLPTR(*loaf), cstr);
		return skit_slice_of(loaf->as_slice, 0, length);
	}
}

static void skit_loaf_store_cstr_test()
{
	skit_loaf loaf = skit_loaf_alloc(8);
	const char *smallish = "foo";
	const char *largish  = "Hello world!";
	
	// Start off with a small string.
	skit_slice test1 = skit_loaf_store_cstr(&loaf, smallish);
	sASSERT_EQS( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), 8 );
	
	// Enlarge.
	skit_slice test2 = skit_loaf_store_cstr(&loaf, largish);
	sASSERT_EQS( test2, skit_slice_of_cstr(largish) );
	sASSERT_NES( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(largish) );
	
	// Narrow.
	skit_slice test3 = skit_loaf_store_cstr(&loaf, smallish);
	sASSERT_EQS( test3, skit_slice_of_cstr(smallish) );
	sASSERT_NES( test2, skit_slice_of_cstr(largish) );
	sASSERT_EQS( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(largish) );
	
	skit_loaf_free(&loaf);
	
	// Allocate when null:
	loaf = skit_loaf_null();
	skit_slice test4 = skit_loaf_store_cstr(&loaf, smallish);
	sASSERT_EQS( test4, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(smallish) );
	skit_loaf_free(&loaf);
	
	// Do nothing when assigned NULL.
	loaf = skit_loaf_new();
	skit_slice test5 = skit_loaf_store_cstr(&loaf, NULL);
	sASSERT( skit_slice_is_null(test5) );
	sASSERT( !skit_loaf_is_null(loaf) );
	skit_loaf_free(&loaf);
	
	printf("  skit_loaf_store_cstr_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_loaf_assign_cstr(skit_loaf *loaf, const char *cstr)
{
	if ( cstr == NULL )
	{
		if ( !skit_loaf_is_null(*loaf) )
		{
			skit_loaf_free(loaf);
			*loaf = skit_loaf_null();
		}
		
		return skit_slice_null();
	}
	else
	{
		skit_slice result = skit_loaf_store_cstr(loaf,cstr);
		ssize_t loaf_len = sLLENGTH(*loaf);
		ssize_t cstr_len = sSLENGTH(result);
		if ( cstr_len < loaf_len )
			skit_loaf_resize(loaf, cstr_len);
		return result;
	}
}

static void skit_loaf_assign_cstr_test()
{
	skit_loaf loaf = skit_loaf_alloc(8);
	const char *smallish = "foo";
	const char *largish  = "Hello world!";
	
	// Start off with a small string.
	skit_slice test1 = skit_loaf_assign_cstr(&loaf, smallish);
	sASSERT_EQS( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), 3 );
	
	// Enlarge.
	skit_slice test2 = skit_loaf_assign_cstr(&loaf, largish);
	sASSERT_EQS( test2, skit_slice_of_cstr(largish) );
	sASSERT_NES( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(largish) );
	
	// Narrow.
	skit_slice test3 = skit_loaf_assign_cstr(&loaf, smallish);
	sASSERT_EQS( test3, skit_slice_of_cstr(smallish) );
	sASSERT_NES( test2, skit_slice_of_cstr(largish) );
	sASSERT_EQS( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(smallish) );
	
	skit_loaf_free(&loaf);
	
	// Allocate when null:
	loaf = skit_loaf_null();
	skit_slice test4 = skit_loaf_assign_cstr(&loaf, smallish);
	sASSERT_EQS( test4, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(smallish) );
	skit_loaf_free(&loaf);
	
	// Free the loaf when assigned NULL.
	loaf = skit_loaf_new();
	skit_slice test5 = skit_loaf_assign_cstr(&loaf, NULL);
	sASSERT( skit_slice_is_null(test5) );
	sASSERT( skit_loaf_is_null(loaf) );
	
	printf("  skit_loaf_assign_cstr_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_loaf_store_slice(skit_loaf *loaf, skit_slice slice)
{
	if ( skit_slice_is_null(slice) )
		return skit_slice_null();
	else
	{
		ssize_t length = sSLENGTH(slice);

		if ( skit_loaf_is_null(*loaf) )
			*loaf = skit_loaf_alloc(length);
		
		if ( length > 0 )
		{
			if ( length > sLLENGTH(*loaf) )
				skit_loaf_resize(loaf, length);
			memcpy((char*)sLPTR(*loaf), (char*)sSPTR(slice), length);
		}

		return skit_slice_of(loaf->as_slice, 0, length);
	}
}

static void skit_loaf_store_slice_test()
{
	skit_loaf loaf = skit_loaf_alloc(8);
	skit_slice  smallish = sSLICE("foo");
	skit_slice  largish  = sSLICE("Hello world!");
	
	// Start off with a small string.
	skit_slice test1 = skit_loaf_store_slice(&loaf, smallish);
	sASSERT_EQS( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), 8 );
	
	// Enlarge.
	skit_slice test2 = skit_loaf_store_slice(&loaf, largish);
	sASSERT_EQS( test2, largish );
	sASSERT_NES( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(largish) );
	
	// Narrow.
	skit_slice test3 = skit_loaf_store_slice(&loaf, smallish);
	sASSERT_EQS( test3, smallish );
	sASSERT_NES( test2, largish );
	sASSERT_EQS( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(largish) );
	
	skit_loaf_free(&loaf);

	// Null characters (and any following characters) should be included in
	// the assignment.
	loaf = skit_loaf_new();
	skit_slice nullified = sSLICE("\0a");
	skit_slice testNull = skit_loaf_store_slice(&loaf, nullified);
	sASSERT_EQS( testNull, nullified );
	skit_loaf_free(&loaf);

	// Allocate when null:
	loaf = skit_loaf_null();
	skit_slice test4 = skit_loaf_store_slice(&loaf, smallish);
	sASSERT_EQS( test4, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(smallish) );
	skit_loaf_free(&loaf);
	
	// Do nothing when assigned skit_slice_null().
	loaf = skit_loaf_new();
	skit_slice test5 = skit_loaf_store_slice(&loaf, skit_slice_null());
	sASSERT( skit_slice_is_null(test5) );
	sASSERT( !skit_loaf_is_null(loaf) );
	skit_loaf_free(&loaf);
	
	printf("  skit_loaf_store_slice_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_loaf_assign_slice(skit_loaf *loaf, skit_slice slice)
{
	if ( skit_slice_is_null(slice) )
	{
		if ( !skit_loaf_is_null(*loaf) )
		{
			skit_loaf_free(loaf);
			*loaf = skit_loaf_null();
		}
		
		return skit_slice_null();
	}
	else
	{
		skit_slice result = skit_loaf_store_slice(loaf,slice);
		ssize_t loaf_len  = sLLENGTH(*loaf);
		ssize_t slice_len = sSLENGTH(result);
		if ( slice_len < loaf_len )
			skit_loaf_resize(loaf, slice_len);
		return result;
	}
}

static void skit_loaf_assign_slice_test()
{
	skit_loaf loaf = skit_loaf_alloc(8);
	skit_slice  smallish = sSLICE("foo");
	skit_slice  largish  = sSLICE("Hello world!");
	
	// Start off with a small string.
	skit_slice test1 = skit_loaf_assign_slice(&loaf, smallish);
	sASSERT_EQS( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), 3 );
	
	// Enlarge.
	skit_slice test2 = skit_loaf_assign_slice(&loaf, largish);
	sASSERT_EQS( test2, largish );
	sASSERT_NES( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(largish) );
	
	// Narrow.
	skit_slice test3 = skit_loaf_assign_slice(&loaf, smallish);
	sASSERT_EQS( test3, smallish );
	sASSERT_NES( test2, largish );
	sASSERT_EQS( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(smallish) );
	
	skit_loaf_free(&loaf);

	// Null characters (and any following characters) should be included in
	// the assignment.
	loaf = skit_loaf_new();
	skit_slice nullified = sSLICE("\0a");
	skit_slice testNull = skit_loaf_assign_slice(&loaf, nullified);
	sASSERT_EQS( testNull, nullified );
	skit_loaf_free(&loaf);

	// Allocate when null:
	loaf = skit_loaf_null();
	skit_slice test4 = skit_loaf_assign_slice(&loaf, smallish);
	sASSERT_EQS( test4, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(smallish) );
	skit_loaf_free(&loaf);
	
	// Free the loaf when assigned skit_slice_null().
	loaf = skit_loaf_new();
	skit_slice test5 = skit_loaf_assign_slice(&loaf, skit_slice_null());
	sASSERT( skit_slice_is_null(test5) );
	sASSERT( skit_loaf_is_null(loaf) );

	printf("  skit_loaf_assign_slice_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of(skit_slice slice, ssize_t index1, ssize_t index2)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_slice_len(slice);
	ssize_t old_offset = skit_slice_get_offset(slice);
	DEBUG(printf("skit_slice_of({ptr=%p,offset=%ld,len=%ld}, %ld, %ld)\n",
		sSPTR(slice), old_offset, sSLENGTH(slice), index1, index2));
	
	/* Implement index2 as SKIT_EOT being length. */
	if ( index2 == SKIT_EOT )
		index2 = length;
	
	/* This implements indexing relative to the end of the string. */
	if ( index1 < 0 )
		index1 = length + index1;
	if ( index2 < 0 )
		index2 = length + index2;
	
	/* No negative slices allowed. */
	sASSERT((index2-index1) >= 0);
	
	/* Do the slicing. */
	result.chars_handle = slice.chars_handle;
	skit_slice_set_length(&result, index2-index1);
	skit_slice_set_offset(&result, index1 + old_offset);
	if ( SKIT_SLICE_IS_CSTR(slice) )
		SKIT_STRING_SET_ALLOC_TYPE(result.meta, skit_string_alloc_type_cstr);
	
	DEBUG(printf("skit_slice_of.result = {ptr=%p,offset=%ld,len=%ld}\n",
		sSPTR(result), skit_slice_get_offset(slice), sSLENGTH(result)));
	
	return result;
}

static void skit_slice_of_subtest(skit_slice slice0)
{
	skit_slice slice1 = skit_slice_of(slice0, 3, 5);
	skit_slice slice2 = skit_slice_of(slice0, 3, SKIT_EOT);
	skit_slice slice3 = skit_slice_of(slice1, 0, -1);
	sASSERT_EQS(slice1, sSLICE("ba"));
	sASSERT_EQS(slice2, sSLICE("bar"));
	sASSERT_EQS(slice3, sSLICE("b"));
	skit_slice slice4 = skit_slice_of(slice2, 1, 2);
	sASSERT_EQS(slice4, sSLICE("a"));
}

static void skit_slice_of_test()
{
	printf("  skit_slice_of_subtest: slice of a loaf.\n");
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice_of_subtest(loaf.as_slice);
	skit_loaf_free(&loaf);
	
	printf("  skit_slice_of_subtest: slice of a C-string.\n");
	skit_slice_of_subtest(sSLICE("foobar"));
	
	printf("  skit_slice_of_test passed.\n");
}

/* ------------------------------------------------------------------------- */

char *skit_loaf_as_cstr(skit_loaf loaf)
{
	return (char*)sLPTR(loaf);
}

/* ------------------------------------------------------------------------- */

char *skit_slice_dup_as_cstr(skit_slice slice)
{
	ssize_t length = skit_slice_len(slice);
	char *result = (char*)skit_malloc(length+1);
	memcpy(result, sSPTR(slice), length);
	result[length] = '\0';
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_free(skit_loaf *loaf)
{
	sASSERT(loaf != NULL);
	sASSERT(sLPTR(*loaf) != NULL);
	sASSERT(skit_loaf_check_init(*loaf));
	skit_string_alloc_type alloc_type = SKIT_STRING_GET_ALLOC_TYPE(loaf->as_slice.meta);
	
	skit_utf8c **handle_ptr = (skit_utf8c**)loaf->chars_handle;
	if ( *handle_ptr == NULL )
	{
		/* Shallow string storage configuration.  Only 1 free needed. */
		if ( alloc_type == skit_string_alloc_type_malloc )
			skit_free(loaf->chars_handle);
	}
	else
	{
		/* Deeper storage configuration with an extra layer of indirection. */
		skit_free(*handle_ptr);
		
		/* The original block might not come from malloc, so we have to check
		  its allocation type and make sure. */
		if ( alloc_type == skit_string_alloc_type_malloc )
			skit_free(loaf->chars_handle);
	}
	
	*loaf = skit_loaf_null();
	return *loaf;
}

static void skit_loaf_free_test()
{
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(sLPTR(loaf) != NULL);
	sASSERT_EQ(sLLENGTH(loaf), 10);
	skit_loaf_free(&loaf);
	sASSERT(sLPTR(loaf) == NULL);
	sASSERT_EQ(sLLENGTH(loaf), 0);
	
	SKIT_LOAF_ON_STACK(stack_loaf1, 32);
	sASSERT(sLPTR(stack_loaf1) != NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf1), 32);
	skit_loaf_free(&stack_loaf1);
	sASSERT(sLPTR(stack_loaf1) == NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf1), 0);
	
	SKIT_LOAF_ON_STACK(stack_loaf2, 64);
	sASSERT(sLPTR(stack_loaf2) != NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf2), 64);
	skit_loaf_resize(&stack_loaf2, 128);
	sASSERT(sLPTR(stack_loaf2) != NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf2), 128);
	skit_loaf_free(&stack_loaf2);
	sASSERT(sLPTR(stack_loaf2) == NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf2), 0);
	
	printf("  skit_loaf_free_test passed.\n");
}

/* ------------------------------------------------------------------------- */

char *skit_slice_get_printf_formatter( skit_slice slice, char *buffer, int buf_size, int enquote )
{
	snprintf(buffer, buf_size,
		(enquote ? "'%%.%lds'" : "%%.%lds"),
		skit_slice_len(slice) );
	return buffer;
}

static void skit_slice_get_printf_formatter_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice = skit_slice_of(loaf.as_slice, 0, 3);
	char newstr_buf[128];
	char fmt_str[64];
	char fmt_buf[32];
	skit_slice_get_printf_formatter(slice, fmt_buf, sizeof(fmt_buf), 1 );
	sASSERT_EQ_CSTR( "'%.3s'", fmt_buf );
	snprintf(fmt_str, sizeof(fmt_str), "Slice is %s.", fmt_buf);
	sASSERT_EQ_CSTR(fmt_str, "Slice is '%.3s'.");
	snprintf(newstr_buf, sizeof(newstr_buf), fmt_str, sSPTR(slice));
	sASSERT_EQ_CSTR(newstr_buf, "Slice is 'foo'.");
	printf("  skit_slice_get_printf_formatter_test passed.\n");
}

/* ========================================================================= */
/* ------------------------- string misc functions ------------------------- */

/* TODO: define semantics and do unittests for slice_to_(u)int functions. */
#define SKIT_SLICE_TO_XINT(xint_str, T, strtoxl) \
	char cstr_buffer[20]; /* 64-bit integers + \0 null terminator can't be longer than 20 bytes. */ \
	memcpy(cstr_buffer, sSPTR(xint_str), sSLENGTH(xint_str)); \
	cstr_buffer[sSLENGTH(xint_str)] = '\0'; \
	\
	char *end_ptr = &cstr_buffer[0]; \
	T result = strtoxl(cstr_buffer, &end_ptr, 10); \
	if ( end_ptr == &cstr_buffer[0] || result == (T)LONG_MIN || result == (T)LONG_MAX ) \
	{ \
		/* Errors. */ \
		sASSERT_MSGF(0, "Could not parse integer '%.*s'", sSLENGTH(xint_str), sSPTR(xint_str)); \
	} \
	\
	return result;

int64_t skit_slice_to_int(skit_slice int_str)
{
	SKIT_SLICE_TO_XINT(int_str, int64_t, strtol);
}

uint64_t skit_slice_to_uint(skit_slice uint_str)
{
	SKIT_SLICE_TO_XINT(uint_str, uint64_t, strtoul);
}

/* ------------------------------------------------------------------------- */

/* skit_is_alpha_lower and such are implemented as macros.  See the .h for implementation. */
static void skit_is_alpha_test()
{
	sASSERT_EQ( skit_is_alpha_lower('A') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha_lower('C') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha_lower('Z') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha_lower('a') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha_lower('c') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha_lower('z') ? 1 : 0, 1 );
	
	sASSERT_EQ( skit_is_alpha_upper('A') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha_upper('C') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha_upper('Z') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha_upper('a') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha_upper('c') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha_upper('z') ? 1 : 0, 0 );
	
	sASSERT_EQ( skit_is_alpha('A') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha('C') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha('Z') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha('a') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha('c') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha('z') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_alpha(' ') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha('{') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_alpha('\0') ? 1 : 0, 0 );
	
	printf("  skit_is_alpha_test passed.\n");
}

static void skit_is_digit_test()
{
	sASSERT_EQ( skit_is_digit('0') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_digit('1') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_digit('9') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_digit('a') ? 1 : 0, 0 );
	sASSERT_EQ( skit_is_digit(' ') ? 1 : 0, 0 );
}

static void skit_is_whitespace_test()
{
	sASSERT_EQ( skit_is_whitespace(' ')  ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_whitespace('\v') ? 1 : 0, 1 );
	sASSERT_EQ( skit_is_whitespace('a')  ? 1 : 0, 0 );
}

/* ------------------------------------------------------------------------- */

skit_utf8c skit_char_ascii_to_lower(skit_utf8c c)
{
	if ( 'A' <= c && c <= 'Z' )
		return (c - 'A') + 'a';
	else
		return c;
}

static void skit_char_ascii_to_lower_test()
{
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('a'), 'a' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('A'), 'a' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('b'), 'b' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('B'), 'b' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('z'), 'z' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('Z'), 'z' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('0'), '0' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower('-'), '-' );
	sASSERT_EQ_HEX( skit_char_ascii_to_lower(' '), ' ' );
	
	printf("  skit_char_ascii_to_lower_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_utf8c skit_char_ascii_to_upper(skit_utf8c c)
{
	if ( 'a' <= c && c <= 'z' )
		return (c - 'a') + 'A';
	else
		return c;
}

static void skit_char_ascii_to_upper_test()
{
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('a'), 'A' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('A'), 'A' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('b'), 'B' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('B'), 'B' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('z'), 'Z' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('Z'), 'Z' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('0'), '0' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper('-'), '-' );
	sASSERT_EQ_HEX( skit_char_ascii_to_upper(' '), ' ' );
	
	printf("  skit_char_ascii_to_upper_test passed.\n");
}

/* ------------------------------------------------------------------------- */

void skit_slice_ascii_to_lower(skit_slice *slice)
{
	sASSERT(slice != NULL);
	skit_utf8c *txt = sSPTR(*slice);
	ssize_t length = sSLENGTH(*slice);
	ssize_t i;
	for ( i = 0; i < length; i++ )
		txt[i] = skit_char_ascii_to_lower(txt[i]);
}

void skit_slice_to_lower(skit_slice *slice)
{
	skit_slice_ascii_to_lower(slice);
}

static void skit_slice_to_lower_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("fOObaR");
	skit_slice slice = loaf.as_slice;
	skit_slice_to_lower(&slice);
	sASSERT_EQS(slice,sSLICE("foobar"));
	skit_loaf_free(&loaf);
	
	printf("  skit_slice_to_lower_test passed.\n");
}

/* ------------------------------------------------------------------------- */

void skit_slice_ascii_to_upper(skit_slice *slice)
{
	sASSERT(slice != NULL);
	skit_utf8c *txt = sSPTR(*slice);
	ssize_t length = sSLENGTH(*slice);
	ssize_t i;
	for ( i = 0; i < length; i++ )
		txt[i] = skit_char_ascii_to_upper(txt[i]);
}

void skit_slice_to_upper(skit_slice *slice)
{
	skit_slice_ascii_to_upper(slice);
}

static void skit_slice_to_upper_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("fOObaR");
	skit_slice slice = loaf.as_slice;
	skit_slice_to_upper(&slice);
	sASSERT_EQS(slice,sSLICE("FOOBAR"));
	skit_loaf_free(&loaf);
	
	printf("  skit_slice_to_upper_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_char_ascii_ccmp(skit_utf8c c1, skit_utf8c c2, int case_sensitive)
{
	if ( case_sensitive )
		return c1 - c2;
	else
	{
		/* Case insensitive. */
		c1 = skit_char_ascii_to_lower(c1);
		c2 = skit_char_ascii_to_lower(c2);
		return c1 - c2;
	}
}

int skit_char_ascii_cmp(skit_utf8c c1, skit_utf8c c2)
{
	return skit_char_ascii_ccmp(c1,c2,1);
}

int skit_char_ascii_icmp(skit_utf8c c1, skit_utf8c c2)
{
	return skit_char_ascii_ccmp(c1,c2,0);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_common_cprefix(const skit_slice str1, const skit_slice str2, int case_sensitive)
{
	skit_utf8c *chars1 = sSPTR(str1);
	skit_utf8c *chars2 = sSPTR(str2);
	sASSERT(chars1 != NULL);
	sASSERT(chars2 != NULL);
	ssize_t len1 = skit_slice_len(str1);
	ssize_t len2 = skit_slice_len(str2);

	ssize_t len = SKIT_MIN(len1,len2);
	ssize_t pos = 0;
	while ( pos < len )
	{
		if ( skit_char_ascii_ccmp(chars1[pos], chars2[pos], case_sensitive) != 0 )
			break;
		pos++;
	}
	
	return skit_slice_of(str1, 0, pos);
}

skit_slice skit_slice_common_prefix(const skit_slice str1, const skit_slice str2)
{
	return skit_slice_common_cprefix(str1, str2, 1);
}

skit_slice skit_slice_common_iprefix(const skit_slice str1, const skit_slice str2)
{
	return skit_slice_common_cprefix(str1, str2, 0);
}

static void skit_slice_common_prefix_test()
{
	skit_slice prefix;
	
	// Case-sensitive.
	skit_slice slice1 = sSLICE("foobar");
	skit_slice slice2 = sSLICE("foobaz");
	prefix = skit_slice_common_prefix(slice1, slice2);
	sASSERT_EQS(prefix, sSLICE("fooba"));
	
	// Case-INsensitive.
	skit_slice islice1 = sSLICE("fOobar");
	skit_slice islice2 = sSLICE("FoObaz");
	prefix = skit_slice_common_iprefix(islice1, islice2);
	sASSERT_EQS(prefix, sSLICE("fOoba"));
	
	// Make sure the case-sensitive version isn't case-insensitive.
	prefix = skit_slice_common_prefix(islice1, islice2);
	sASSERT_EQS(prefix, sSLICE(""));
	
	printf("  skit_slice_common_prefix_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_ascii_ccmp(const skit_slice str1, const skit_slice str2, int case_sensitive)
{
	skit_utf8c *chars1 = sSPTR(str1);
	skit_utf8c *chars2 = sSPTR(str2);
	
	if ( chars1 == NULL )
	{
		if ( chars2 == NULL )
			return 0;
		else
			return -1;
	}
	else if ( chars2 == NULL )
		return 1;
	
	ssize_t len1 = skit_slice_len(str1);
	ssize_t len2 = skit_slice_len(str2);
	
	skit_utf8c c1;
	skit_utf8c c2;
	skit_slice slice = skit_slice_common_cprefix(str1,str2,case_sensitive);
	ssize_t pos = skit_slice_len(slice);
	
	ssize_t suffix_len_1 = len1 - pos;
	ssize_t suffix_len_2 = len2 - pos;
	
	/* Don't try to dereference chars[length]... it might not be nul on slices! */
	if ( suffix_len_1 == suffix_len_2 )
	{
		if ( suffix_len_1 == 0 )
			return 0;
	}
	else
	{
		if ( suffix_len_1 == 0 || suffix_len_2 == 0 )
			return suffix_len_1 - suffix_len_2;
	}

	c1 = chars1[pos];
	c2 = chars2[pos];
	
	return skit_char_ascii_ccmp(c1,c2,case_sensitive);
}

int skit_slice_ascii_cmp(const skit_slice str1, const skit_slice str2)
{
	return skit_slice_ascii_ccmp(str1,str2,1);
}

int skit_slice_ascii_icmp(const skit_slice str1, const skit_slice str2)
{
	return skit_slice_ascii_ccmp(str1,str2,0);
}

static void skit_slice_ascii_cmp_test()
{
	// Basic equivalence and ordering.
	skit_slice bigstr = sSLICE("----------");
	skit_slice lilstr = sSLICE("-------");
	skit_slice aaa = sSLICE("aaa");
	skit_slice bbb = sSLICE("bbb");
	skit_slice aaaa = sSLICE("aaaa");
	skit_loaf aaab = skit_loaf_copy_cstr("aaab");
	skit_slice aaa_slice = skit_slice_of(aaab.as_slice,0,3);
	sASSERT(skit_slice_ascii_cmp(lilstr, bigstr) < 0); 
	sASSERT(skit_slice_ascii_cmp(bigstr, lilstr) > 0);
	sASSERT(skit_slice_ascii_cmp(bigstr, bigstr) == 0);
	sASSERT(skit_slice_ascii_cmp(aaa, bbb) < 0);
	sASSERT(skit_slice_ascii_cmp(bbb, aaa) > 0);
	sASSERT(skit_slice_ascii_cmp(aaa, aaa_slice) == 0);
	sASSERT(skit_slice_ascii_cmp(aaa, aaaa) < 0);
	sASSERT(skit_slice_ascii_cmp(aaaa, bbb) < 0);
	skit_loaf_free(&aaab);
	
	// nullity.
	skit_slice nullstr = skit_slice_null();
	sASSERT(skit_slice_ascii_cmp(nullstr, nullstr) == 0);
	sASSERT(skit_slice_ascii_cmp(nullstr, aaa) < 0);
	sASSERT(skit_slice_ascii_cmp(aaa, nullstr) > 0);
	
	// Case-sensitivity.
	skit_slice AAA = sSLICE("AAA");
	skit_slice aAa = sSLICE("aAa");
	sASSERT(skit_slice_ascii_icmp(aaa,AAA) == 0);
	sASSERT(skit_slice_ascii_icmp(aaa,aAa) == 0);
	sASSERT(skit_slice_ascii_icmp(AAA,aAa) == 0);
	sASSERT(skit_slice_ascii_cmp (aaa,AAA) != 0);
	sASSERT(skit_slice_ascii_cmp (aaa,aAa) != 0);
	sASSERT(skit_slice_ascii_cmp (AAA,aAa) != 0);
	
	printf("  skit_slice_ascii_cmp_test passed.\n");
}

/* ------------------------------------------------------------------------- */

/* --- Case-sensitive --- */
int skit_slice_ges(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_cmp(str1,str2) >= 0 ); }
int skit_slice_gts(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_cmp(str1,str2) >  0 ); }
int skit_slice_les(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_cmp(str1,str2) <= 0 ); }
int skit_slice_lts(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_cmp(str1,str2) <  0 ); }
int skit_slice_eqs(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_cmp(str1,str2) == 0 ); }
int skit_slice_nes(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_cmp(str1,str2) != 0 ); }

/* ------------------------ */
/* --- Case-INsensitive --- */
int skit_slice_iges(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_icmp(str1,str2) >= 0 ); }
int skit_slice_igts(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_icmp(str1,str2) >  0 ); }
int skit_slice_iles(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_icmp(str1,str2) <= 0 ); }
int skit_slice_ilts(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_icmp(str1,str2) <  0 ); }
int skit_slice_ieqs(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_icmp(str1,str2) == 0 ); }
int skit_slice_ines(const skit_slice str1, const skit_slice str2) { return ( skit_slice_ascii_icmp(str1,str2) != 0 ); }

/* ------------------------ */
/* --- Case-??sensitive --- */
int skit_slice_cges(const skit_slice str1, const skit_slice str2, int case_sensitive) { return ( skit_slice_ascii_ccmp(str1,str2,case_sensitive) >= 0 ); }
int skit_slice_cgts(const skit_slice str1, const skit_slice str2, int case_sensitive) { return ( skit_slice_ascii_ccmp(str1,str2,case_sensitive) >  0 ); }
int skit_slice_cles(const skit_slice str1, const skit_slice str2, int case_sensitive) { return ( skit_slice_ascii_ccmp(str1,str2,case_sensitive) <= 0 ); }
int skit_slice_clts(const skit_slice str1, const skit_slice str2, int case_sensitive) { return ( skit_slice_ascii_ccmp(str1,str2,case_sensitive) <  0 ); }
int skit_slice_ceqs(const skit_slice str1, const skit_slice str2, int case_sensitive) { return ( skit_slice_ascii_ccmp(str1,str2,case_sensitive) == 0 ); }
int skit_slice_cnes(const skit_slice str1, const skit_slice str2, int case_sensitive) { return ( skit_slice_ascii_ccmp(str1,str2,case_sensitive) != 0 ); }

/* ------------------------ */
static void skit_slice_comparison_ops_test()
{
	skit_slice alphaLo = sSLICE("aaa");
	skit_slice alphaHi = sSLICE("bbb");
	
	sASSERT(!skit_slice_ges(alphaLo,alphaHi)); // alphaLo >= alphaHi
	sASSERT( skit_slice_ges(alphaHi,alphaLo)); // alphaHi >= alphaLo
	sASSERT( skit_slice_ges(alphaHi,alphaHi)); // alphaHi >= alphaHi
	sASSERT(!skit_slice_gts(alphaLo,alphaHi)); // alphaLo >  alphaHi
	sASSERT( skit_slice_gts(alphaHi,alphaLo)); // alphaHi >  alphaLo
	sASSERT(!skit_slice_gts(alphaHi,alphaHi)); // alphaHi >  alphaHi
	
	sASSERT( skit_slice_les(alphaLo,alphaHi)); // alphaLo <= alphaHi
	sASSERT(!skit_slice_les(alphaHi,alphaLo)); // alphaHi <= alphaLo
	sASSERT( skit_slice_les(alphaHi,alphaHi)); // alphaHi <= alphaHi
	sASSERT( skit_slice_lts(alphaLo,alphaHi)); // alphaLo <  alphaHi
	sASSERT(!skit_slice_lts(alphaHi,alphaLo)); // alphaHi <  alphaLo
	sASSERT(!skit_slice_lts(alphaHi,alphaHi)); // alphaHi <  alphaHi
	
	sASSERT(!skit_slice_eqs(alphaLo,alphaHi)); // alphaLo == alphaHi
	sASSERT(!skit_slice_eqs(alphaHi,alphaLo)); // alphaHi == alphaLo
	sASSERT( skit_slice_eqs(alphaHi,alphaHi)); // alphaHi == alphaHi
	sASSERT( skit_slice_nes(alphaLo,alphaHi)); // alphaLo != alphaHi
	sASSERT( skit_slice_nes(alphaHi,alphaLo)); // alphaHi != alphaLo
	sASSERT(!skit_slice_nes(alphaHi,alphaHi)); // alphaHi != alphaHi
	
	skit_slice nullstr = skit_slice_null();
	sASSERT( skit_slice_eqs(nullstr,nullstr));
	sASSERT(!skit_slice_nes(nullstr,nullstr));
	sASSERT(!skit_slice_eqs(alphaLo,nullstr));
	sASSERT( skit_slice_nes(alphaLo,nullstr));
	sASSERT( skit_slice_lts(nullstr,alphaLo));
	
	skit_slice ialphaLo = sSLICE("aaa");
	skit_slice ialphaHi = sSLICE("BBB");
	
	sASSERT(!skit_slice_iges(ialphaLo,ialphaHi)); // alphaLo >= alphaHi
	sASSERT( skit_slice_iges(ialphaHi,ialphaLo)); // alphaHi >= alphaLo
	sASSERT( skit_slice_iges(ialphaHi,ialphaHi)); // alphaHi >= alphaHi
	sASSERT(!skit_slice_igts(ialphaLo,ialphaHi)); // alphaLo >  alphaHi
	sASSERT( skit_slice_igts(ialphaHi,ialphaLo)); // alphaHi >  alphaLo
	sASSERT(!skit_slice_igts(ialphaHi,ialphaHi)); // alphaHi >  alphaHi

	sASSERT( skit_slice_iles(ialphaLo,ialphaHi)); // alphaLo <= alphaHi
	sASSERT(!skit_slice_iles(ialphaHi,ialphaLo)); // alphaHi <= alphaLo
	sASSERT( skit_slice_iles(ialphaHi,ialphaHi)); // alphaHi <= alphaHi
	sASSERT( skit_slice_ilts(ialphaLo,ialphaHi)); // alphaLo <  alphaHi
	sASSERT(!skit_slice_ilts(ialphaHi,ialphaLo)); // alphaHi <  alphaLo
	sASSERT(!skit_slice_ilts(ialphaHi,ialphaHi)); // alphaHi <  alphaHi

	sASSERT(!skit_slice_ieqs(ialphaLo,ialphaHi)); // alphaLo == alphaHi
	sASSERT(!skit_slice_ieqs(ialphaHi,ialphaLo)); // alphaHi == alphaLo
	sASSERT( skit_slice_ieqs(ialphaHi,ialphaHi)); // alphaHi == alphaHi
	sASSERT( skit_slice_ines(ialphaLo,ialphaHi)); // alphaLo != alphaHi
	sASSERT( skit_slice_ines(ialphaHi,ialphaLo)); // alphaHi != alphaLo
	sASSERT(!skit_slice_ines(ialphaHi,ialphaHi)); // alphaHi != alphaHi
	
	printf("  skit_slice_comparison_ops_test passed.\n");
}


/* ------------------------------------------------------------------------- */

skit_slice skit_slice_ltrimx(const skit_slice slice, const skit_slice char_class)
{
	skit_utf8c *chars = sSPTR(slice);
	skit_utf8c *match_set = sSPTR(char_class);
	ssize_t set_len = sSLENGTH(char_class);
	int found;
	ssize_t i;
	
	ssize_t length = skit_slice_len(slice);
	ssize_t lbound = 0;
	while ( lbound < length )
	{
		skit_utf8c c = chars[lbound];
		found = 0;
		for ( i = 0; i < set_len; i++ )
			if ( c == match_set[i] )
				{found = 1; break;}
		if ( !found )
			break;
		
		lbound++;
	}
	
	return skit_slice_of(slice, lbound, length);
}

skit_slice skit_slice_rtrimx(const skit_slice slice, const skit_slice char_class)
{
	skit_utf8c *chars = sSPTR(slice);
	skit_utf8c *match_set = sSPTR(char_class);
	ssize_t set_len = sSLENGTH(char_class);
	int found;
	ssize_t i;
	
	ssize_t length = skit_slice_len(slice);
	ssize_t rbound = length;
	while ( rbound > 0 )
	{
		char c = chars[rbound-1];
		found = 0;
		for ( i = 0; i < set_len; i++ )
			if ( c == match_set[i] )
				{found = 1; break;}
		if ( !found )
			break;
		
		rbound--;
	}
	
	return skit_slice_of(slice, 0, rbound);
}

skit_slice skit_slice_trimx(const skit_slice slice, const skit_slice char_class)
{
	return skit_slice_ltrimx( skit_slice_rtrimx(slice, char_class), char_class );
}

static void skit_slice_trimx_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("xxfooabc");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltrimx(slice0, sSLICE("x"));
	skit_slice slice2 = skit_slice_rtrimx(slice0, sSLICE("c"));
	skit_slice slice3 = skit_slice_trimx (slice0, sSLICE("xabc"));
	sASSERT_EQS( slice1, sSLICE("fooabc") );
	sASSERT_EQS( slice2, sSLICE("xxfooab") );
	sASSERT_EQS( slice3, sSLICE("foo") );
	skit_loaf_free(&loaf);
	printf("  skit_slice_trimx_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_ltrim(const skit_slice slice)
{
	skit_utf8c *chars = sSPTR(slice);
	
	ssize_t length = skit_slice_len(slice);
	ssize_t lbound = 0;
	while ( lbound < length )
	{
		skit_utf8c c = chars[lbound];
		if (!skit_is_whitespace(c))
			break;
		
		lbound++;
	}
	
	return skit_slice_of(slice, lbound, length);
}

skit_slice skit_slice_rtrim(const skit_slice slice)
{
	skit_utf8c *chars = sSPTR(slice);
	
	ssize_t length = skit_slice_len(slice);
	ssize_t rbound = length;
	while ( rbound > 0 )
	{
		char c = chars[rbound-1];
		if (!skit_is_whitespace(c))
			break;
		
		rbound--;
	}
	
	return skit_slice_of(slice, 0, rbound);
}

skit_slice skit_slice_trim(const skit_slice slice)
{
	return skit_slice_ltrim( skit_slice_rtrim(slice) );
}

static void skit_slice_trim_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("  foo \n");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltrim(slice0);
	skit_slice slice2 = skit_slice_rtrim(slice0);
	skit_slice slice3 = skit_slice_trim (slice0);
	sASSERT_EQS( slice1, sSLICE("foo \n") );
	sASSERT_EQS( slice2, sSLICE("  foo") );
	sASSERT_EQS( slice3, sSLICE("foo") );
	skit_loaf_free(&loaf);
	printf("  skit_slice_trim_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_ltruncate(const skit_slice slice, size_t nchars)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_slice_len(slice);
	
	if ( nchars > length )
	{
		/* The requested truncation is greater than the string length. */
		/* In this case we return a zero-length slice at the end of the string. */
		result = skit_slice_of(slice, length, length);
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		result = skit_slice_of(slice, nchars, length);
	}
	
	return result;
}

skit_slice skit_slice_rtruncate(const skit_slice slice, size_t nchars)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_slice_len(slice);
	
	if ( nchars > length )
	{
		/* The requested truncation is greater than the string length. */
		/* In this case we return a zero-length slice at the beginning of the string. */
		result = skit_slice_of(slice, 0, 0);
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		result = skit_slice_of(slice, 0, length - nchars);
	}
	
	return result;
}

static void skit_slice_truncate_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltruncate(slice0,3);
	skit_slice slice2 = skit_slice_rtruncate(slice0,3);
	sASSERT_EQS( slice1, sSLICE("bar") );
	sASSERT_EQS( slice2, sSLICE("foo") );
	skit_loaf_free(&loaf);
	printf("  skit_slice_truncate_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_match(
	const skit_slice haystack,
	const skit_slice needle,
	ssize_t pos)
{
	ssize_t haystack_length = sSLENGTH(haystack);
	ssize_t needle_length = sSLENGTH(needle);
	skit_utf8c *haystack_chars = sSPTR(haystack);
	skit_utf8c *needle_chars = sSPTR(needle);
	ssize_t rbound;
	ssize_t i;
	sASSERT(haystack_chars != NULL);
	sASSERT(needle_chars != NULL);
	sASSERT_GE(pos,0); 
	
	rbound = pos+needle_length;
	if ( rbound > haystack_length )
		return 0;
	
	for ( i = 0; i < needle_length; i++ )
	{
		ssize_t j = i+pos;
		if ( haystack_chars[j] != needle_chars[i] )
			return 0;
	}
	
	return 1;
}

static void skit_slice_match_test()
{
	skit_slice haystack = sSLICE("foobarbaz");
	skit_slice needle = sSLICE("bar");
	sASSERT_EQ(skit_slice_match(haystack,needle,0),0);
	sASSERT_EQ(skit_slice_match(haystack,needle,3),1);
	sASSERT_EQ(skit_slice_match(haystack,needle,6),0);
	sASSERT_EQ(skit_slice_match(haystack,needle,8),0);
	printf("  skit_slice_match_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_match_nl(
	const skit_slice text,
	ssize_t pos)
{
	skit_utf8c *text_chars = sSPTR(text);
	sASSERT(text_chars != NULL);
	sASSERT_GE(pos,0);
	ssize_t length = sSLENGTH(text);
	sASSERT_LT(pos,length);
	if ( text_chars[pos] == '\r' )
	{
		if ( pos+1 < length && text_chars[pos+1] == '\n' )
			return 2;
		else
			return 1;
	}
	else if ( text_chars[pos] == '\n' )
		return 1;

	return 0;
}

static void skit_slice_match_test_nl()
{
	skit_slice haystack = sSLICE("foo\nbar\r\nbaz\rqux");
	sASSERT_EQ(skit_slice_match_nl(haystack,3) ,1);
	sASSERT_EQ(skit_slice_match_nl(haystack,7) ,2);
	sASSERT_EQ(skit_slice_match_nl(haystack,8) ,1);
	sASSERT_EQ(skit_slice_match_nl(haystack,12),1);
	sASSERT_EQ(skit_slice_match_nl(haystack,0) ,0);
	sASSERT_EQ(skit_slice_match_nl(haystack,2) ,0);
	sASSERT_EQ(skit_slice_match_nl(haystack,4) ,0);
	sASSERT_EQ(skit_slice_match_nl(haystack,13),0);
	printf("  skit_slice_match_test_nl passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_find(
	const skit_slice haystack,
	const skit_slice needle,
	ssize_t *output_pos)
{
	ssize_t pos;
	ssize_t len = sSLENGTH(haystack);
	if ( output_pos == NULL )
		output_pos = &pos;

	for ( pos = 0; pos < len; pos++ )
	{
		if ( skit_slice_match(haystack,needle,pos) )
		{
			*output_pos = pos;
			return 1;
		}
	}

	*output_pos = -1;
	return 0;
}

static void skit_slice_find_test()
{
	ssize_t pos = 0;
	sASSERT(!skit_slice_find(sSLICE("foo"),sSLICE("x"),&pos));
	sASSERT_EQ(pos,-1);
	sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("o"),&pos));
	sASSERT_EQ(pos,1);
	sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("f"),&pos));
	sASSERT_EQ(pos,0);
	sASSERT(!skit_slice_find(sSLICE("foo"),sSLICE("x"),NULL));
	sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("f"),NULL));
	sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("o"),NULL));
	printf("  skit_slice_find_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_partition(
	const skit_slice text,
	const skit_slice delimiter,
	skit_slice *head,
	skit_slice *tail)
{
	sASSERT(!skit_slice_is_null(text));
	sASSERT(!skit_slice_is_null(delimiter));

	ssize_t head_end = -1;
	ssize_t tail_start = 0;
	ssize_t text_len = sSLENGTH(text);
	
	int found = skit_slice_find(text, delimiter, &head_end);
	if ( found )
	{
		// head_end is already assigned by skit_slice_find.
		tail_start = head_end + sSLENGTH(delimiter);
	}
	else
	{
		head_end = text_len;
		tail_start = text_len;
	}

	if ( head ) *head = skit_slice_of(text, 0, head_end);
	if ( tail ) *tail = skit_slice_of(text, tail_start, text_len);

	return found;
}

static void skit_slice_partition_test()
{
	skit_slice text = sSLICE("Hello world!");
	skit_slice head = skit_slice_null();
	skit_slice tail = skit_slice_null();
	skit_slice delim = sSLICE(" ");
	
	sASSERT(skit_slice_partition(text, delim, &head, &tail));
	sASSERT_EQS(head, sSLICE("Hello"));
	sASSERT_EQS(tail, sSLICE("world!"));
	
	tail = skit_slice_null();
	sASSERT(skit_slice_partition(text, delim, &head, NULL));
	sASSERT_EQS(head, sSLICE("Hello") );
	sASSERT( skit_slice_is_null(tail) );
	
	head = skit_slice_null();
	sASSERT(skit_slice_partition(text, delim, NULL, &tail));
	sASSERT( skit_slice_is_null(head) );
	sASSERT_EQS(tail, sSLICE("world!"));

	text = sSLICE("Hello");
	sASSERT(!skit_slice_partition(text, delim, &head, &tail));
	sASSERT_EQS(head, sSLICE("Hello"));
	sASSERT_EQS(tail, sSLICE(""));
	
	text = sSLICE("");
	sASSERT(!skit_slice_partition(text, delim, &head, &tail));
	sASSERT_EQS(head, sSLICE(""));
	sASSERT_EQS(tail, sSLICE(""));

	printf("  skit_slice_partition_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_take_head(
	skit_slice *text,
	const skit_slice delimiter,
	skit_slice *head)
{
	sASSERT(text != NULL);
	sASSERT(!skit_slice_is_null(*text));
	sASSERT(!skit_slice_is_null(delimiter));

	int have_next = !!(sSLENGTH(*text) > 0);
	skit_slice_partition(*text, delimiter, head, text);
	return have_next;
}

static void skit_slice_take_head_test()
{
	skit_slice text = sSLICE("a b c");
	skit_slice head = skit_slice_null();
	skit_slice delim = sSLICE(" ");
	
	sASSERT(skit_slice_take_head(&text,delim,&head));
	sASSERT_EQS(head,sSLICE("a"));
	sASSERT_EQS(text,sSLICE("b c"));
	
	sASSERT(skit_slice_take_head(&text,delim,&head));
	sASSERT_EQS(head,sSLICE("b"));
	sASSERT_EQS(text,sSLICE("c"));
	
	sASSERT(skit_slice_take_head(&text,delim,&head));
	sASSERT_EQS(head,sSLICE("c"));
	sASSERT_EQS(text,sSLICE(""));
	
	sASSERT(!skit_slice_take_head(&text,delim,&head));
	sASSERT_EQS(head,sSLICE(""));
	sASSERT_EQS(text,sSLICE(""));
	
	text = sSLICE("a b c");
	int count = 0;
	while ( skit_slice_take_head(&text,delim,NULL) )
	    count++;
	sASSERT_EQ(count, 3);

	printf("  skit_slice_take_head_test passed.\n");
}

/* ------------------------------------------------------------------------- */

static int skit_char_is_printable( skit_utf8c c )
{
	/* Printable chars between space (inclusive) and delete (exclusive) */
	if ( 0x20 <= c && c < 0x7F )
		return 1;
	else
		return 0;
}

static const skit_utf8c skit_escape_table[] =
	"0      abtn fr  " /* 0x00 */
	"                " /* 0x10 */
	"                " /* 0x20 */
	"                " /* 0x30 */
	"                " /* 0x40 */
	"                " /* 0x50 */
	"                " /* 0x60 */
	"                " /* 0x70 */
	"                " /* 0x80 */
	"                " /* 0x90 */
	"                " /* 0xA0 */
	"                " /* 0xB0 */
	"                " /* 0xC0 */
	"                " /* 0xD0 */
	"                " /* 0xE0 */
	"                " /* 0xF0 */ ;

static skit_utf8c skit_int_to_hex( int x )
{
	if ( x < 10 )
		return x + '0';
	else
		return x - 10 + 'A';
}
	
skit_slice skit_slice_escapify(skit_slice str, skit_loaf *buffer)
{
	size_t i;
	size_t req_size;
	skit_utf8c *str_ptr = sSPTR(str);
	ssize_t     str_len = sSLENGTH(str);
	
	req_size = 0;
	for ( i = 0; i < str_len; i++ )
	{
		skit_utf8c c = str_ptr[i];
		if ( skit_char_is_printable(c) )
			req_size++;
		else if ( skit_escape_table[c] != ' ' )
			req_size += 2;
		else
			req_size += 4;
	}
	
	ssize_t  buf_len = sLLENGTH(*buffer);
	
	if ( req_size > buf_len )
	{
		buffer = skit_loaf_resize(buffer, req_size);
		buf_len = req_size;
	}
	
	skit_utf8c *buf_ptr = sLPTR(*buffer);
	
	size_t j = 0;
	for ( i = 0; i < str_len; i++ )
	{
		skit_utf8c c = str_ptr[i];
		
		if ( skit_char_is_printable(c) )
			buf_ptr[j++] = c;
		else if ( skit_escape_table[c] != ' ' )
		{
			buf_ptr[j++] = '\\';
			buf_ptr[j++] = skit_escape_table[c];
		}
		else
		{
			buf_ptr[j++] = '\\';
			buf_ptr[j++] = 'x';
			buf_ptr[j++] = skit_int_to_hex((c >> 4) & 0x0F);
			buf_ptr[j++] = skit_int_to_hex((c >> 0) & 0x0F);
		}
	}
	
	return skit_slice_of(buffer->as_slice, 0, req_size);
}

static void skit_slice_escapify_test()
{
	skit_utf8c cFF[1];
	cFF[0] = 255;
	
	sASSERT_EQ(skit_escape_table['\0'], '0');
	sASSERT_EQ(skit_escape_table['\a'], 'a');
	sASSERT_EQ(skit_escape_table['\b'], 'b');
	sASSERT_EQ(skit_escape_table['\t'], 't');
	sASSERT_EQ(skit_escape_table['\n'], 'n');
	sASSERT_EQ(skit_escape_table['\t'], 't');
	sASSERT_EQ(skit_escape_table['\f'], 'f');
	sASSERT_EQ(skit_escape_table['0'],  ' ');
	sASSERT_EQ(skit_escape_table[cFF[0]], ' ');
	
	SKIT_LOAF_ON_STACK(buffer, 4);
	/*
	sASSERT_EQS(skit_slice_escapify(sSLICE("\0a"),&buffer), sSLICE("\\0a"));
	sASSERT_EQS(skit_slice_escapify(sSLICE("\r\nxy\x02"), &buffer), sSLICE("\\r\\nxy\\x02"));
	sASSERT_EQS(skit_slice_escapify(skit_slice_of_cstrn(cFF,1), &buffer), sSLICE("\\xFF"));
	*/
	sASSERT(skit_slice_eqs(skit_slice_escapify(sSLICE("\0a"),&buffer), sSLICE("\\0a")));
	sASSERT(skit_slice_eqs(skit_slice_escapify(sSLICE("\r\nxy\x02"), &buffer), sSLICE("\\r\\nxy\\x02")));
	sASSERT(skit_slice_eqs(skit_slice_escapify(sSLICE("\xFF"), &buffer), sSLICE("\\xFF")));
	
	skit_loaf_free(&buffer);
	
	printf("  skit_slice_escapify passed.\n");
}

/* ========================================================================= */
/* ----------------------------- unittest list ----------------------------- */

void skit_string_unittest()
{
	printf("skit_slice_unittest()\n");
	skit_loaf_emplace_test();
	skit_loaf_on_stack_test();
	skit_slice_len_test();
	skit_slice_ptr_test();
	skit_slice_is_null_test();
	skit_slice_check_init_test();
	skit_slice_of_cstrn_test();
	skit_slice_of_cstr_test();
	skit_slice_sSLICE_test();
	skit_loaf_resize_test();
	skit_loaf_append_test();
	skit_slice_concat_test();
	skit_slice_buffered_resize_test();
	skit_slice_buffered_append_test();
	skit_loaf_dup_test();
	skit_loaf_store_cstr_test();
	skit_loaf_assign_cstr_test();
	skit_loaf_store_slice_test();
	skit_loaf_assign_slice_test();
	skit_slice_of_test();
	skit_loaf_free_test();
	skit_slice_get_printf_formatter_test();
	skit_is_alpha_test();
	skit_is_digit_test();
	skit_is_whitespace_test();
	skit_char_ascii_to_lower_test();
	skit_char_ascii_to_upper_test();
	skit_slice_to_lower_test();
	skit_slice_to_upper_test();
	skit_slice_common_prefix_test();
	skit_slice_ascii_cmp_test();
	skit_slice_comparison_ops_test();
	skit_slice_trimx_test();
	skit_slice_trim_test();
	skit_slice_truncate_test();
	skit_slice_match_test();
	skit_slice_match_test_nl();
	skit_slice_find_test();
	skit_slice_partition_test();
	skit_slice_take_head_test();
	skit_slice_escapify_test();
	printf("  skit_slice_unittest passed!\n");
	printf("\n");
}
