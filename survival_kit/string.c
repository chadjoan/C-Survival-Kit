
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

static void skit_slice_set_length(skit_slice *slice, size_t len)
{
	slice->meta = 
		(slice->meta & ~META_LENGTH_MASK) | 
		((len << META_LENGTH_SHIFT) & META_LENGTH_MASK);
}

static void skit_slice_set_offset(skit_slice *slice, size_t offset)
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
	
	/* TODO: cleanup. */
	skit_loaf result = skit_loaf_null();
	
	/* Allocate a null handle followed by a zero-length string. */
	/* The zero-length string is represented by a single nul byte. */
	result.chars_handle = (skit_utf8c*)skit_malloc(SKIT_LOAF_EMPLACEMENT_OVERHEAD);
	memset( result.chars_handle, 0, sizeof(skit_utf8c*)+1 );
	
	/* Set the length to agree with the string data. */
	skit_slice_set_length(&result.as_slice,0);
	
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_copy_cstr(const char *cstr)
{
	skit_loaf result = skit_loaf_alloc(strlen(cstr));
	skit_loaf_assign_cstr(&result,cstr);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_alloc(size_t length)
{
	size_t mem_size = SKIT_LOAF_EMPLACEMENT_OVERHEAD + length;
	void *mem = skit_malloc(mem_size);
	return skit_loaf_emplace_internal(mem, mem_size, skit_string_alloc_type_malloc);

	/* TODO: cleanup */
	skit_loaf result = skit_loaf_null();
	
	/* Allocate enough memory for (null handle)+(desired length)+(nul-terminating byte) */
	result.chars_handle = (skit_utf8c*)skit_malloc(sizeof(skit_utf8c*) + length + 1);
	memset(result.chars_handle, 0, sizeof(skit_utf8c*)+1); /* nullify the handle and provide a nul-terminator right after it. */
	result.chars_handle[sizeof(skit_utf8c*)+length] = '\0'; /* put a nul-terminator at the very end, just incase. */
	
	skit_slice_set_length(&result.as_slice, length);
	return result;
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
	sASSERT_EQ( mem_size, sLLENGTH(loaf) + SKIT_LOAF_EMPLACEMENT_OVERHEAD, "%d" );
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
	sASSERT_EQ( sLLENGTH(loaf), 32, "%d" );
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
	sASSERT_EQ(skit_loaf_len(loaf), 10, "%d");
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
		return slice.chars_handle;
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
	sASSERT_EQ(sSPTR(slice) - sLPTR(loaf), 3, "%d");
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
	sASSERT_EQ(skit_loaf_is_null(nloaf),   1, "%d");
	sASSERT_EQ(skit_slice_is_null(nslice), 1, "%d");
	sASSERT_EQ(skit_loaf_is_null(loaf),    0, "%d");
	sASSERT_EQ(skit_slice_is_null(slice),  0, "%d");
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
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
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
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "foo");
	printf("  skit_slice_of_cstr_test passed.\n");
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
	ssize_t buf_slice_length;
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
	buf_slice_length = skit_slice_len(*buf_slice);
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
	sASSERT_EQ(skit_slice_len(slice), 2, "%d");
	skit_slice_buffered_resize(&buffer, &slice, 5);
	sASSERT(skit_loaf_len(buffer) >= 6);
	sASSERT_EQ(skit_slice_len(slice), 5, "%d");
	sASSERT_EQ(sSPTR(slice)[5], '\0', "%d");
	skit_loaf_free(&buffer);
	printf("  skit_slice_buffered_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice *skit_slice_buffered_append(
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
	
	return buf_slice;
}

static void skit_slice_buffered_append_test()
{
	skit_loaf  buffer = skit_loaf_alloc(5);
	skit_slice accumulator = skit_slice_of(buffer.as_slice, 0, 0);
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("foo"));
	sASSERT_EQ(skit_loaf_len(buffer), 5, "%d");
	sASSERT_EQ(skit_slice_len(accumulator), 3, "%d");
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("bar"));
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(buffer), "foobar");
	sASSERT_GE(skit_loaf_len(buffer), 6, "%d");
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
	sASSERT_NE(sLPTR(foo), sLPTR(bar), "%p");
	skit_loaf_assign_cstr(&bar, "bar");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
	printf("  skit_loaf_dup_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_loaf_assign_cstr(skit_loaf *loaf, const char *cstr)
{
	sASSERT( loaf != NULL );
	sASSERT(!skit_loaf_is_null(*loaf));
	ssize_t length = strlen(cstr);
	if ( length > sLLENGTH(*loaf) )
		skit_loaf_resize(loaf, length);
	strcpy((char*)sLPTR(*loaf), cstr);
	return skit_slice_of(loaf->as_slice, 0, length);
}

static void skit_loaf_assign_cstr_test()
{
	skit_loaf loaf = skit_loaf_alloc(8);
	const char *smallish = "foo";
	const char *largish  = "Hello world!";
	skit_slice test1 = skit_loaf_assign_cstr(&loaf, smallish);
	sASSERT_EQS( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), 8, "%d" );
	skit_slice test2 = skit_loaf_assign_cstr(&loaf, largish);
	sASSERT_EQS( test2, skit_slice_of_cstr(largish) );
	sASSERT_NES( test1, skit_slice_of_cstr(smallish) );
	sASSERT_EQ( sLLENGTH(loaf), strlen(largish), "%d" );
	skit_loaf_free(&loaf);
	printf("  skit_loaf_assign_cstr_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_loaf_assign_slice(skit_loaf *loaf, skit_slice slice)
{
	sASSERT(!skit_loaf_is_null(*loaf));
	ssize_t length = sSLENGTH(slice);
	if ( length > sLLENGTH(*loaf) )
		skit_loaf_resize(loaf, length);
	strcpy((char*)sLPTR(*loaf), (char*)sSPTR(slice));
	return skit_slice_of(loaf->as_slice, 0, length);
}

static void skit_loaf_assign_slice_test()
{
	skit_loaf loaf = skit_loaf_alloc(8);
	skit_slice smallish = sSLICE("foo");
	skit_slice largish  = sSLICE("Hello world!");
	skit_slice test1 = skit_loaf_assign_slice(&loaf, smallish);
	sASSERT_EQS( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), 8, "%d" );
	skit_slice test2 = skit_loaf_assign_slice(&loaf, largish);
	sASSERT_EQS( test2, largish );
	sASSERT_NES( test1, smallish );
	sASSERT_EQ( sLLENGTH(loaf), sSLENGTH(largish), "%d" );
	skit_loaf_free(&loaf);
	printf("  skit_loaf_assign_slice_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of(skit_slice slice, ssize_t index1, ssize_t index2)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_slice_len(slice);
	ssize_t old_offset = skit_slice_get_offset(slice);
	
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
	
	return result;
}

static void skit_slice_of_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_of(slice0, 3, 5);
	skit_slice slice2 = skit_slice_of(slice0, 3, SKIT_EOT);
	skit_slice slice3 = skit_slice_of(slice1, 0, -1);
	sASSERT_EQS(slice1, sSLICE("ba"));
	sASSERT_EQS(slice2, sSLICE("bar"));
	sASSERT_EQS(slice3, sSLICE("b"));
	skit_loaf_free(&loaf);
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
	sASSERT_EQ(sLLENGTH(loaf), 10, "%d");
	skit_loaf_free(&loaf);
	sASSERT(sLPTR(loaf) == NULL);
	sASSERT_EQ(sLLENGTH(loaf), 0, "%d");
	
	SKIT_LOAF_ON_STACK(stack_loaf1, 32);
	sASSERT(sLPTR(stack_loaf1) != NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf1), 32, "%d");
	skit_loaf_free(&stack_loaf1);
	sASSERT(sLPTR(stack_loaf1) == NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf1), 0, "%d");
	
	SKIT_LOAF_ON_STACK(stack_loaf2, 64);
	sASSERT(sLPTR(stack_loaf2) != NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf2), 64, "%d");
	skit_loaf_resize(&stack_loaf2, 128);
	sASSERT(sLPTR(stack_loaf2) != NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf2), 128, "%d");
	skit_loaf_free(&stack_loaf2);
	sASSERT(sLPTR(stack_loaf2) == NULL);
	sASSERT_EQ(sLLENGTH(stack_loaf2), 0, "%d");
	
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

static void skit_is_alpha_test()
{
	sASSERT_EQ( skit_is_alpha_lower('A') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha_lower('C') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha_lower('Z') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha_lower('a') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha_lower('c') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha_lower('z') ? 1 : 0, 1, "%d" );
	
	sASSERT_EQ( skit_is_alpha_upper('A') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha_upper('C') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha_upper('Z') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha_upper('a') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha_upper('c') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha_upper('z') ? 1 : 0, 0, "%d" );
	
	sASSERT_EQ( skit_is_alpha('A') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha('C') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha('Z') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha('a') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha('c') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha('z') ? 1 : 0, 1, "%d" );
	sASSERT_EQ( skit_is_alpha(' ') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha('{') ? 1 : 0, 0, "%d" );
	sASSERT_EQ( skit_is_alpha('\0') ? 1 : 0, 0, "%d" );
	
	printf("  skit_is_alpha_test passed.\n");
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
	sASSERT_EQ( skit_char_ascii_to_lower('a'), 'a', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('A'), 'a', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('b'), 'b', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('B'), 'b', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('z'), 'z', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('Z'), 'z', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('0'), '0', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower('-'), '-', "%c" );
	sASSERT_EQ( skit_char_ascii_to_lower(' '), ' ', "%c" );
	
	printf("  skit_char_ascii_to_lower_test passed.\n");
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
	
	if ( len1 != len2 )
		return len1 - len2;
	else
	{
		skit_utf8c c1;
		skit_utf8c c2;
		skit_slice slice = skit_slice_common_cprefix(str1,str2,case_sensitive);
		ssize_t pos = skit_slice_len(slice);
		
		/* Don't try to dereference chars[pos]... it might not be nul on slices! */
		if ( pos == len1 )
			return 0;

		c1 = chars1[pos];
		c2 = chars2[pos];
		
		return skit_char_ascii_ccmp(c1,c2,case_sensitive);
	}
	sASSERT(0);
	return -55;
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
	skit_slice bigstr = sSLICE("Big string!");
	skit_slice lilstr = sSLICE("lil str.");
	skit_slice aaa = sSLICE("aaa");
	skit_slice bbb = sSLICE("bbb");
	skit_loaf aaab = skit_loaf_copy_cstr("aaab");
	skit_slice aaa_slice = skit_slice_of(aaab.as_slice,0,3);
	sASSERT(skit_slice_ascii_cmp(lilstr, bigstr) < 0); 
	sASSERT(skit_slice_ascii_cmp(bigstr, lilstr) > 0);
	sASSERT(skit_slice_ascii_cmp(bigstr, bigstr) == 0);
	sASSERT(skit_slice_ascii_cmp(aaa, bbb) < 0);
	sASSERT(skit_slice_ascii_cmp(bbb, aaa) > 0);
	sASSERT(skit_slice_ascii_cmp(aaa, aaa_slice) == 0);
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
int skit_slice_ges(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_cmp(str1,str2) >= 0 )
		return 1;
	return 0;
}

int skit_slice_gts(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_cmp(str1,str2) > 0 )
		return 1;
	return 0;
}

int skit_slice_les(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_cmp(str1,str2) <= 0 )
		return 1;
	return 0;
}

int skit_slice_lts(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_cmp(str1,str2) < 0 )
		return 1;
	return 0;
}

int skit_slice_eqs(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_cmp(str1,str2) == 0 )
		return 1;
	return 0;
}

int skit_slice_nes(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_cmp(str1,str2) != 0 )
		return 1;
	return 0;
}

/* ------------------------ */
/* --- Case-INsensitive --- */
int skit_slice_iges(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_icmp(str1,str2) >= 0 )
		return 1;
	return 0;
}

int skit_slice_igts(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_icmp(str1,str2) > 0 )
		return 1;
	return 0;
}

int skit_slice_iles(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_icmp(str1,str2) <= 0 )
		return 1;
	return 0;
}

int skit_slice_ilts(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_icmp(str1,str2) < 0 )
		return 1;
	return 0;
}

int skit_slice_ieqs(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_icmp(str1,str2) == 0 )
		return 1;
	return 0;
}

int skit_slice_ines(const skit_slice str1, const skit_slice str2)
{
	if ( skit_slice_ascii_icmp(str1,str2) != 0 )
		return 1;
	return 0;
}

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
	sASSERT_GE(pos,0,"%d"); 
	
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
	sASSERT_EQ(skit_slice_match(haystack,needle,0),0,"%d");
	sASSERT_EQ(skit_slice_match(haystack,needle,3),1,"%d");
	sASSERT_EQ(skit_slice_match(haystack,needle,6),0,"%d");
	sASSERT_EQ(skit_slice_match(haystack,needle,8),0,"%d");
	printf("  skit_slice_match_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_match_nl(
	const skit_slice text,
	ssize_t pos)
{
	skit_utf8c *text_chars = sSPTR(text);
	sASSERT(text_chars != NULL);
	sASSERT_GE(pos,0,"%d");
	ssize_t length = sSLENGTH(text);
	sASSERT_LT(pos,length,"%d");
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
	sASSERT_EQ(skit_slice_match_nl(haystack,3),1,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,7),2,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,8),1,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,12),1,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,0),0,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,2),0,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,4),0,"%d");
	sASSERT_EQ(skit_slice_match_nl(haystack,13),0,"%d");
	printf("  skit_slice_match_test_nl passed.\n");
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
	skit_loaf_resize_test();
	skit_loaf_append_test();
	skit_slice_concat_test();
	skit_slice_buffered_resize_test();
	skit_slice_buffered_append_test();
	skit_loaf_dup_test();
	skit_loaf_assign_cstr_test();
	skit_loaf_assign_slice_test();
	skit_slice_of_test();
	skit_loaf_free_test();
	skit_slice_get_printf_formatter_test();
	skit_is_alpha_test();
	skit_char_ascii_to_lower_test();
	skit_slice_common_prefix_test();
	skit_slice_ascii_cmp_test();
	skit_slice_comparison_ops_test();
	skit_slice_trim_test();
	skit_slice_truncate_test();
	skit_slice_match_test();
	skit_slice_match_test_nl();
	printf("  skit_slice_unittest passed!\n");
	printf("\n");
}