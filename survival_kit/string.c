
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


/* The hi byte of the 'meta' member is layed out like so:
 *   1 0 1 L S S S S
 * 
 * The highest 3 bits are always 1 0 1.  This causes the meta member to
 *   always be /very/ negative, and at the same time not entirely made of set
 *   bits.  If a string is provided that doesn't have those 3 bits in that
 *   arrangement, then any string handling function working with it can assume 
 *   that the string wans't propertly initialized and throw an exception.
 *   This is accessed internally with the META_CHECK_XXX defines.
 * 
 * The L bit is whether the string is a (l)oaf or a slice.
 *   This is accessed internally with the META_SLICE_XXX defines.
 * 
 * The four S bits are the stride of the array.  Currently, this field has the
 *   following possible values with the following meanings:
 *   1 - A utf8 encoded string.
 *   2 - A utf16 encoded string.
 *   4 - A utf32 encoded string.
 *   This is accessed internally with the META_STRIDE_XXX defines.
 * 
 * The rest of the bytes in the 'meta' member are the length of the string.
 *   These are accessed internally with the META_LENGTH_XXX defines.
 */

#define META_STRIDE_SHIFT (sizeof(skit_string_meta)*8 - 8)
#define META_STRIDE_MASK  (0x0FULL << META_STRIDE_SHIFT)

#define META_SLICE_SHIFT  (sizeof(skit_string_meta)*8 - 4)
#define META_SLICE_BIT    (0x1ULL << META_SLICE_SHIFT)

#define META_CHECK_SHIFT  (sizeof(skit_string_meta)*8 - 3)
#define META_CHECK_MASK   (0x7ULL << META_CHECK_SHIFT)
#define META_CHECK_VAL    (0x5ULL << META_CHECK_SHIFT)

#define META_LENGTH_MASK  ((1ULL << ((sizeof(skit_string_meta)-1)*8))-1)
#define META_LENGTH_SHIFT (0)

#define skit_string_init_meta() \
	(META_CHECK_VAL | (1ULL << META_STRIDE_SHIFT) | META_SLICE_BIT)

static void skit_slice_setlen(skit_slice *slice, size_t len)
{
	slice->meta = 
		(slice->meta & ~META_LENGTH_MASK) | 
		((len << META_LENGTH_SHIFT) & META_LENGTH_MASK);
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
	result.as_slice.meta &= ~META_SLICE_BIT; /* clear the slice bit */
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_new()
{
	skit_loaf result = skit_loaf_null();
	result.chars_handle = (skit_utf8c*)skit_malloc(1);
	result.chars_handle[0] = '\0';
	skit_slice_setlen(&result.as_slice,0);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_copy_cstr(const char *cstr)
{
	size_t length = strlen(cstr);
	skit_loaf result = skit_loaf_null();
	result.chars_handle = (skit_utf8c*)skit_malloc(length+1);
	strcpy((char*)result.chars_handle, cstr);
	skit_slice_setlen(&result.as_slice, length);
	sASSERT(skit_loaf_check_init(result));
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_alloc(size_t length)
{
	skit_loaf result = skit_loaf_null();
	result.chars_handle = (skit_utf8c*)skit_malloc(length+1);
	result.chars_handle[length] = '\0';
	skit_slice_setlen(&result.as_slice, length);
	return result;
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
	printf("  skit_slice_len_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_utf8c *skit_loaf_ptr( skit_loaf loaf )
{
	return loaf.chars_handle;
}

skit_utf8c *skit_slice_ptr( skit_slice slice )
{
	return slice.chars_handle;
}

/* ------------------------------------------------------------------------- */

int skit_slice_check_init(skit_slice slice)
{
	if ( META_CHECK_VAL == (slice.meta & META_CHECK_MASK) )
		return 1;
	return 0;
}

int skit_loaf_check_init (skit_loaf loaf)   { return skit_slice_check_init(loaf.as_slice);  }

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
	skit_slice_setlen(&result, length);
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

int skit_slice_is_loaf(skit_slice slice)
{
	if ( (slice.meta & META_SLICE_BIT) == 0 )
		return 1;
	return 0;
}

static void skit_slice_is_loaf_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foo");
	skit_slice casted_slice = loaf.as_slice;
	skit_slice sliced_slice = skit_slice_of(loaf.as_slice, 0, SKIT_EOT);
	sASSERT( skit_slice_is_loaf(casted_slice));
	sASSERT(!skit_slice_is_loaf(sliced_slice));
	skit_loaf_free(&loaf);
	printf("  skit_slice_is_loaf_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_slice_as_loaf(skit_slice slice)
{
	skit_loaf result = skit_loaf_null();
	sASSERT(skit_slice_check_init(slice));
	sASSERT(skit_slice_is_loaf(slice));
	result.chars_handle = slice.chars_handle;
	result.as_slice.meta = slice.meta;
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length)
{
	skit_utf8c *loaf_chars = sLPTR(*loaf);
	sASSERT(loaf != NULL);
	sASSERT(loaf_chars != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf), "'loaf' was not initialized.");
	
	loaf_chars = skit_realloc(loaf_chars, length+1);
	loaf_chars[length] = '\0';
	skit_slice_setlen(&loaf->as_slice, length);
	
	return loaf;
}

static void skit_loaf_resize_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello world!");
	skit_loaf_resize(&loaf, 5);
	sASSERT_EQ_CSTR("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_slice str2)
{
	size_t len1;
	size_t len2;
	skit_utf8c *loaf1_chars = sLPTR(*loaf1);
	skit_utf8c *str2_chars = sSPTR(str2);
	sASSERT(loaf1 != NULL);
	sASSERT(loaf1_chars != NULL);
	sASSERT(str2_chars != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf1), "'loaf1' was not initialized.");
	sASSERT_MSG(skit_slice_check_init(str2), "'str2' was not initialized.");
	
	len2 = skit_slice_len(str2);
	if ( len2 == 0 )
		return loaf1;
	
	len1 = skit_loaf_len(*loaf1);
	
	loaf1 = skit_loaf_resize(loaf1, len1+len2);
	memcpy(loaf1_chars + len1, str2_chars, len2);
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
		ptrdiff_t slice_pos = buf_slice_chars - buffer_chars;
		
		/* Resize to (new_buffer_length * 1.5) */
		new_buffer_length = (new_buffer_length * 3) / 2;
		skit_loaf_resize( buffer, new_buffer_length );
		*buf_slice = skit_slice_of( buffer->as_slice, slice_pos, slice_pos + new_buf_slice_length );
	}
	else
		skit_slice_setlen(buf_slice, new_buf_slice_length);
	
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

skit_loaf skit_slice_dup(skit_slice slice)
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

static void skit_slice_dup_test()
{
	skit_loaf foo = skit_loaf_copy_cstr("foo");
	skit_slice slice = skit_slice_of(foo.as_slice, 0, 0);
	skit_loaf bar = skit_slice_dup(slice);
	sASSERT_NE(sLPTR(foo), sLPTR(bar), "%p");
	skit_loaf_assign_cstr(&bar, "bar");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
	printf("  skit_slice_dup_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_assign_cstr(skit_loaf *loaf, const char *cstr)
{
	ssize_t length = strlen(cstr);
	skit_loaf_resize(loaf, length);
	strcpy((char*)sLPTR(*loaf), cstr);
	return loaf;
}

static void skit_loaf_assign_cstr_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	sASSERT_EQ_CSTR( skit_loaf_as_cstr(loaf), "Hello" );
	skit_loaf_assign_cstr(&loaf, "Hello world!");
	sASSERT_EQ_CSTR( skit_loaf_as_cstr(loaf), "Hello world!" );
	skit_loaf_free(&loaf);
	printf("  skit_loaf_assign_cstr_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of(skit_slice slice, ssize_t index1, ssize_t index2)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_slice_len(slice);
	
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
	result.chars_handle = sSPTR(slice) + index1;
	skit_slice_setlen(&result, index2-index1);
	
	return result;
}

static void skit_slice_of_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_of(slice0, 3, -1);
	skit_slice slice2 = skit_slice_of(slice0, 3, SKIT_EOT);
	sASSERT_EQS(slice1, sSLICE("ba"));
	sASSERT_EQS(slice2, sSLICE("bar"));
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

skit_loaf *skit_loaf_free(skit_loaf *loaf)
{
	sASSERT(loaf != NULL);
	sASSERT(sLPTR(*loaf) != NULL);
	sASSERT(skit_loaf_check_init(*loaf));
	
	skit_free(sLPTR(*loaf));
	*loaf = skit_loaf_null();
	return loaf;
}

static void skit_loaf_free_test()
{
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(sLPTR(loaf) != NULL);
	skit_loaf_free(&loaf);
	sASSERT(sLPTR(loaf) == NULL);
	sASSERT(skit_loaf_len(loaf) == 0);
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

#define IS_WHITESPACE(x) (x == ' '  || x == '\t' || x == '\r' || x == '\n' )

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_common_prefix(const skit_slice str1, const skit_slice str2)
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
		if ( chars1[pos] != chars2[pos] )
			break;
		pos++;
	}
	
	return skit_slice_of(str1, 0, pos);
}

static void skit_slice_common_prefix_test()
{
	skit_slice slice1 = sSLICE("foobar");
	skit_slice slice2 = sSLICE("foobaz");
	skit_slice prefix = skit_slice_common_prefix(slice1, slice2);
	sASSERT_EQS(prefix, sSLICE("fooba"));
	printf("  skit_slice_common_prefix_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_ascii_cmp(const skit_slice str1, const skit_slice str2)
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
		skit_slice slice = skit_slice_common_prefix(str1,str2);
		ssize_t pos = skit_slice_len(slice);
		
		/* Don't try to dereference chars[pos]... it might not be nul on slices! */
		if ( pos == len1 )
			return 0;

		c1 = chars1[pos];
		c2 = chars2[pos];
		
		return c1 - c2;
	}
	sASSERT(0);
	return -55;
}

static void skit_slice_ascii_cmp_test()
{
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
	
	skit_slice nullstr = skit_slice_null();
	sASSERT(skit_slice_ascii_cmp(nullstr, nullstr) == 0);
	sASSERT(skit_slice_ascii_cmp(nullstr, aaa) < 0);
	sASSERT(skit_slice_ascii_cmp(aaa, nullstr) > 0);
	printf("  skit_slice_ascii_cmp_test passed.\n");
}

/* ------------------------------------------------------------------------- */

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
		if (!IS_WHITESPACE(c))
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
		if (!IS_WHITESPACE(c))
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
	skit_slice_len_test();
	skit_slice_check_init_test();
	skit_slice_of_cstrn_test();
	skit_slice_of_cstr_test();
	skit_slice_is_loaf_test();
	skit_loaf_resize_test();
	skit_loaf_append_test();
	skit_slice_concat_test();
	skit_slice_buffered_resize_test();
	skit_slice_buffered_append_test();
	skit_slice_dup_test();
	skit_loaf_assign_cstr_test();
	skit_slice_of_test();
	skit_loaf_free_test();
	skit_slice_get_printf_formatter_test();
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