
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h> /* For ssize_t */

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
	result.chars = NULL;
	result.meta  = skit_string_init_meta();
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

skit_loaf skit_string_new()
{
	skit_loaf result = skit_loaf_null();
	result.chars = (skit_utf8c*)skit_malloc(1);
	result.chars[0] = '\0';
	skit_slice_setlen(&result.as_slice,0);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_copy_cstr(const char *cstr)
{
	size_t length = strlen(cstr);
	skit_loaf result = skit_loaf_null();
	result.chars = (skit_utf8c*)skit_malloc(length+1);
	strcpy((char*)result.chars, cstr);
	skit_slice_setlen(&result.as_slice, length);
	sASSERT(skit_loaf_check_init(result));
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_alloc(size_t length)
{
	skit_loaf result = skit_loaf_null();
	result.chars = (skit_utf8c*)skit_malloc(length+1);
	result.chars[length] = '\0';
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
	sASSERT(skit_loaf_len(loaf) == 10);
	printf("  skit_slice_len_test passed.\n");
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

skit_slice skit_slice_of_cstr(const char *cstr)
{
	skit_slice result = skit_slice_null();
	ssize_t length = strlen(cstr);
	result.chars = (skit_utf8c*)cstr;
	skit_slice_setlen(&result, length);
	return result;
}

static void skit_slice_of_cstr_test()
{
	skit_slice slice = skit_slice_of_cstr("foo");
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
	sASSERT_EQS((char*)slice.chars, "foo");
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
	result.chars = slice.chars;
	result.as_slice.meta = slice.meta;
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length)
{
	sASSERT(loaf != NULL);
	sASSERT(loaf->chars != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf), "'loaf' was not initialized.");
	
	loaf->chars = skit_realloc(loaf->chars, length+1);
	loaf->chars[length] = '\0';
	skit_slice_setlen(&loaf->as_slice, length);
	
	return loaf;
}

static void skit_loaf_resize_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello world!");
	skit_loaf_resize(&loaf, 5);
	sASSERT_EQS("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_slice str2)
{
	size_t len1;
	size_t len2;
	sASSERT(loaf1 != NULL);
	sASSERT(loaf1->chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf1), "'loaf1' was not initialized.");
	sASSERT_MSG(skit_slice_check_init(str2), "'str2' was not initialized.");
	
	len2 = skit_slice_len(str2);
	if ( len2 == 0 )
		return loaf1;
	
	len1 = skit_loaf_len(*loaf1);
	
	loaf1 = skit_loaf_resize(loaf1, len1+len2);
	memcpy(loaf1->chars + len1, str2.chars, len2);
	/* setlen was already handled by skit_loaf_resize. */
	
	return loaf1;
}

static void skit_loaf_append_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	skit_loaf_append(&loaf, skit_slice_of_cstr(" world!"));
	sASSERT_EQS("Hello world!", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_append_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_slice_join(skit_slice str1, skit_slice str2)
{
	size_t len1;
	size_t len2;
	skit_loaf result;
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT(skit_slice_check_init(str1));
	sASSERT(skit_slice_check_init(str2));
	
	len1 = skit_slice_len(str1);
	len2 = skit_slice_len(str2);
	
	result = skit_loaf_alloc(len1+len2);
	memcpy(result.chars,      str1.chars, len1);
	memcpy(result.chars+len1, str2.chars, len2);
	/* setlen was already handled by skit_loaf_alloc. */
	
	return result;
}

static void skit_slice_join_test()
{
	skit_loaf  orig  = skit_loaf_copy_cstr("Hello world!");
	skit_slice slice = skit_slice_of(orig.as_slice, 0, 6);
	skit_loaf  newb  = skit_slice_join(slice, orig.as_slice);
	sASSERT_EQS("Hello Hello world!", skit_loaf_as_cstr(newb));
	skit_loaf_free(&orig);
	skit_loaf_free(&newb);
	printf("  skit_slice_join_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_buffered_resize(
	skit_loaf  *buffer,
	skit_slice buf_slice,
	ssize_t    new_buf_slice_length)
{
	skit_slice result;
	ssize_t buffer_length;
	ssize_t buf_slice_length;
	skit_utf8c *rbound;
	skit_utf8c *lbound;
	skit_utf8c *new_rbound;
	sASSERT(buffer != NULL);
	sASSERT(buffer->chars != NULL);
	sASSERT(buf_slice.chars != NULL);
	
	buffer_length    = skit_loaf_len(*buffer);
	buf_slice_length = skit_slice_len(buf_slice);
	lbound = buffer->chars;
	rbound = lbound + buffer_length;
	sASSERT_MSG(lbound <= buf_slice.chars && buf_slice.chars <= rbound, 
		"The buf_slice given is not a substring of the given buffer.");
	
	new_rbound = buf_slice.chars + new_buf_slice_length;
	if ( new_rbound > rbound )
	{
		ssize_t new_buffer_length = new_rbound - buffer->chars;
		
		/* Resize to (new_buffer_length * 1.5) */
		new_buffer_length = (new_buffer_length * 3) / 2;
		skit_loaf_resize( buffer, new_buffer_length );
	}
	
	result = skit_slice_null();
	result.chars = buf_slice.chars;
	skit_slice_setlen(&result, new_buf_slice_length);
	return result;
}

static void skit_slice_buffered_resize_test()
{
	skit_loaf buffer = skit_loaf_alloc(5);
	skit_slice slice1 = skit_slice_of(buffer.as_slice, 2, 4);
	skit_slice slice2 = skit_slice_buffered_resize(&buffer, slice1, 5);
	sASSERT(skit_loaf_len(buffer) >= 6);
	sASSERT_EQ(skit_slice_len(slice1), 2, "%d");
	sASSERT_EQ(skit_slice_len(slice2), 5, "%d");
	sASSERT_EQ(slice2.chars[5], '\0', "%d");
	skit_loaf_free(&buffer);
	printf("  skit_slice_buffered_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_buffered_append(
	skit_loaf  *buffer,
	skit_slice buf_slice,
	skit_slice suffix)
{
	skit_slice result;
	ssize_t suffix_length;
	ssize_t buf_slice_length;
	/* We don't need to check buffer and buf_slice because skit_slice_buffered_resize will do that. */
	sASSERT(suffix.chars != NULL);
	
	suffix_length = skit_slice_len(suffix);
	buf_slice_length = skit_slice_len(buf_slice);
	result = skit_slice_buffered_resize(buffer, buf_slice, buf_slice_length + suffix_length);
	memcpy((void*)(result.chars + buf_slice_length), (void*)suffix.chars, suffix_length);
	
	return result;
}

static void skit_slice_buffered_append_test()
{
	skit_loaf  buffer = skit_loaf_alloc(5);
	skit_slice accumulator = skit_slice_of(buffer.as_slice, 0, 0);
	accumulator = skit_slice_buffered_append(&buffer, accumulator, skit_slice_of_cstr("foo"));
	sASSERT_EQ(skit_loaf_len(buffer), 5, "%d");
	sASSERT_EQ(skit_slice_len(accumulator), 3, "%d");
	accumulator = skit_slice_buffered_append(&buffer, accumulator, skit_slice_of_cstr("bar"));
	sASSERT_EQS(skit_loaf_as_cstr(buffer), "foobar");
	sASSERT(skit_loaf_len(buffer) >= 6);
	skit_loaf_free(&buffer);
	printf("  skit_slice_buffered_append_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_slice_dup(skit_slice slice)
{
	size_t length;
	skit_loaf result;
	sASSERT(slice.chars != NULL);
	sASSERT(skit_slice_check_init(slice));
	
	length = skit_slice_len(slice);
	result = skit_loaf_alloc(length);
	memcpy(result.chars, slice.chars, length);
	/* The call to skit_loaf_alloc will have already handled setlen */
	return result;
}

static void skit_slice_dup_test()
{
	skit_loaf foo = skit_loaf_copy_cstr("foo");
	skit_slice slice = skit_slice_of(foo.as_slice, 0, 0);
	skit_loaf bar = skit_slice_dup(slice);
	sASSERT(foo.chars != bar.chars);
	skit_loaf_assign_cstr(&bar, "bar");
	sASSERT_EQS(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQS(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
	printf("  skit_slice_dup_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_assign_cstr(skit_loaf *loaf, const char *cstr)
{
	ssize_t length = strlen(cstr);
	skit_loaf_resize(loaf, length);
	strcpy((char*)loaf->chars, cstr);
	return loaf;
}

static void skit_loaf_assign_cstr_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	sASSERT_EQS( skit_loaf_as_cstr(loaf), "Hello" );
	skit_loaf_assign_cstr(&loaf, "Hello world!");
	sASSERT_EQS( skit_loaf_as_cstr(loaf), "Hello world!" );
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
	result.chars = slice.chars + index1;
	skit_slice_setlen(&result, index2-index1);
	
	return result;
}

static void skit_slice_of_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_of(slice0, 3, -1);
	skit_slice slice2 = skit_slice_of(slice0, 3, SKIT_EOT);
	char *cstr1 = skit_slice_dup_as_cstr(slice1);
	char *cstr2 = skit_slice_dup_as_cstr(slice2);
	sASSERT_EQS(cstr1, "ba");
	sASSERT_EQS(cstr2, "bar");
	skit_loaf_free(&loaf);
	printf("  skit_slice_of_test passed.\n");
}

/* ------------------------------------------------------------------------- */

char *skit_loaf_as_cstr(skit_loaf loaf)
{
	return (char*)loaf.chars;
}

/* ------------------------------------------------------------------------- */

char *skit_slice_dup_as_cstr(skit_slice slice)
{
	ssize_t length = skit_slice_len(slice);
	char *result = (char*)skit_malloc(length+1);
	memcpy(result, slice.chars, length);
	result[length] = '\0';
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_free(skit_loaf *loaf)
{
	sASSERT(loaf != NULL);
	sASSERT(loaf->chars != NULL);
	sASSERT(skit_loaf_check_init(*loaf));
	
	skit_free(loaf->chars);
	*loaf = skit_loaf_null();
	return loaf;
}

static void skit_loaf_free_test()
{
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(loaf.chars != NULL);
	skit_loaf_free(&loaf);
	sASSERT(loaf.chars == NULL);
	sASSERT(skit_loaf_len(loaf) == 0);
	printf("  skit_loaf_free_test passed.\n");
}

/* ========================================================================= */
/* ------------------------- string misc functions ------------------------- */

#define IS_WHITESPACE(x) (x == ' '  || x == '\t' || x == '\r' || x == '\n' )

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_common_prefix(const skit_slice str1, const skit_slice str2)
{
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
	ssize_t len1 = skit_slice_len(str1);
	ssize_t len2 = skit_slice_len(str2);

	ssize_t len = SKIT_MIN(len1,len2);
	ssize_t pos = 0;
	while ( pos < len )
	{
		if ( str1.chars[pos] != str2.chars[pos] )
			break;
		pos++;
	}
	
	return skit_slice_of(str1, 0, pos);
}

static void skit_slice_common_prefix_test()
{
	skit_loaf loaf1 = skit_loaf_copy_cstr("foobar");
	skit_loaf loaf2 = skit_loaf_copy_cstr("foobaz");
	skit_slice str1 = loaf1.as_slice;
	skit_slice str2 = loaf2.as_slice;
	skit_slice prefix = skit_slice_common_prefix(str1, str2);
	char *cstr = skit_slice_dup_as_cstr(prefix);
	sASSERT_EQS(cstr, "fooba");
	skit_free(cstr);
	skit_loaf_free(&loaf1);
	skit_loaf_free(&loaf2);
	printf("  skit_slice_common_prefix_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_slice_ascii_cmp(const skit_slice str1, const skit_slice str2)
{
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
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

		c1 = str1.chars[pos];
		c2 = str2.chars[pos];
		
		return c1 - c2;
	}
	sASSERT(0);
	return -55;
}

static void skit_slice_ascii_cmp_test()
{
	skit_loaf bigstr = skit_loaf_copy_cstr("Big string!");
	skit_loaf lilstr = skit_loaf_copy_cstr("lil str.");
	skit_loaf aaa = skit_loaf_copy_cstr("aaa");
	skit_loaf bbb = skit_loaf_copy_cstr("bbb");
	skit_loaf aaab = skit_loaf_copy_cstr("aaab");
	skit_slice aaa_slice = skit_slice_of(aaab.as_slice,0,3);
	sASSERT(skit_slice_ascii_cmp(lilstr.as_slice, bigstr.as_slice) < 0); 
	sASSERT(skit_slice_ascii_cmp(bigstr.as_slice, lilstr.as_slice) > 0);
	sASSERT(skit_slice_ascii_cmp(bigstr.as_slice, bigstr.as_slice) == 0);
	sASSERT(skit_slice_ascii_cmp(aaa.as_slice, bbb.as_slice) < 0);
	sASSERT(skit_slice_ascii_cmp(bbb.as_slice, aaa.as_slice) > 0);
	sASSERT(skit_slice_ascii_cmp(aaa.as_slice, aaa_slice) == 0);
	skit_loaf_free(&bigstr);
	skit_loaf_free(&lilstr);
	skit_loaf_free(&aaa);
	skit_loaf_free(&bbb);
	printf("  skit_slice_ascii_cmp_test passed.\n");
}

/* ------------------------------------------------------------------------- */

/**
Convenient asciibetical comparison functions.
Example:
*/

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
	skit_loaf aaa = skit_loaf_copy_cstr("aaa");
	skit_loaf bbb = skit_loaf_copy_cstr("bbbb");
	skit_slice alphaLo = aaa.as_slice;
	skit_slice alphaHi = bbb.as_slice;
	
	sASSERT(!skit_slice_ges(alphaLo,alphaHi)); /* alphaLo >= alphaHi */
	sASSERT( skit_slice_ges(alphaHi,alphaLo)); /* alphaHi >= alphaLo */
	sASSERT( skit_slice_ges(alphaHi,alphaHi)); /* alphaHi >= alphaHi */
	sASSERT(!skit_slice_gts(alphaLo,alphaHi)); /* alphaLo >  alphaHi */
	sASSERT( skit_slice_gts(alphaHi,alphaLo)); /* alphaHi >  alphaLo */
	sASSERT(!skit_slice_gts(alphaHi,alphaHi)); /* alphaHi >  alphaHi */

	sASSERT( skit_slice_les(alphaLo,alphaHi)); /* alphaLo <= alphaHi */
	sASSERT(!skit_slice_les(alphaHi,alphaLo)); /* alphaHi <= alphaLo */
	sASSERT( skit_slice_les(alphaHi,alphaHi)); /* alphaHi <= alphaHi */
	sASSERT( skit_slice_lts(alphaLo,alphaHi)); /* alphaLo <  alphaHi */
	sASSERT(!skit_slice_lts(alphaHi,alphaLo)); /* alphaHi <  alphaLo */
	sASSERT(!skit_slice_lts(alphaHi,alphaHi)); /* alphaHi <  alphaHi */

	sASSERT(!skit_slice_eqs(alphaLo,alphaHi)); /* alphaLo == alphaHi */
	sASSERT(!skit_slice_eqs(alphaHi,alphaLo)); /* alphaHi == alphaLo */
	sASSERT( skit_slice_eqs(alphaHi,alphaHi)); /* alphaHi == alphaHi */
	sASSERT( skit_slice_nes(alphaLo,alphaHi)); /* alphaLo != alphaHi */
	sASSERT( skit_slice_nes(alphaHi,alphaLo)); /* alphaHi != alphaLo */
	sASSERT(!skit_slice_nes(alphaHi,alphaHi)); /* alphaHi != alphaHi */
	
	skit_loaf_free(&aaa);
	skit_loaf_free(&bbb);
	printf("  skit_slice_comparison_ops_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_ltrim(const skit_slice slice)
{
	skit_slice result = skit_slice_null();
	
	ssize_t length = skit_slice_len(slice);
	ssize_t lbound = 0;
	while ( lbound < length )
	{
		skit_utf8c c = slice.chars[lbound];
		if (!IS_WHITESPACE(c))
			break;
		
		lbound++;
	}
	
	result.chars  = slice.chars + lbound;
	skit_slice_setlen(&result, length - lbound);
	
	return result;
}

skit_slice skit_slice_rtrim(const skit_slice slice)
{
	skit_slice result = skit_slice_null();
	
	ssize_t length = skit_slice_len(slice);
	ssize_t rbound = length;
	while ( rbound > 0 )
	{
		char c = slice.chars[rbound-1];
		if (!IS_WHITESPACE(c))
			break;
		
		rbound--;
	}
	
	result.chars  = slice.chars;
	skit_slice_setlen(&result, rbound);
	
	return result;
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
	sASSERT( skit_slice_eqs(slice1, skit_slice_of_cstr("foo \n")) );
	sASSERT( skit_slice_eqs(slice2, skit_slice_of_cstr("  foo")) );
	sASSERT( skit_slice_eqs(slice3, skit_slice_of_cstr("foo")) );
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
		result.chars = slice.chars;
		skit_slice_setlen(&result, 0);
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		result.chars = slice.chars + nchars;
		skit_slice_setlen(&result, length - nchars);
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
		result.chars = slice.chars;
		skit_slice_setlen(&result, 0);
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		result.chars = slice.chars;
		skit_slice_setlen(&result, length - nchars);
	}
	
	return result;
}

static void skit_slice_truncate_test()
{
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltruncate(slice0,3);
	skit_slice slice2 = skit_slice_rtruncate(slice0,3);
	sASSERT( skit_slice_eqs(slice1, skit_slice_of_cstr("bar")) );
	sASSERT( skit_slice_eqs(slice2, skit_slice_of_cstr("foo")) );
	skit_loaf_free(&loaf);
	printf("  skit_slice_truncate_test passed.\n");
}

/* ========================================================================= */
/* ----------------------------- unittest list ----------------------------- */

void skit_string_unittest()
{
	printf("skit_slice_unittest()\n");
	skit_slice_len_test();
	skit_slice_check_init_test();
	skit_slice_of_cstr_test();
	skit_slice_is_loaf_test();
	skit_loaf_resize_test();
	skit_loaf_append_test();
	skit_slice_join_test();
	skit_slice_buffered_resize_test();
	skit_slice_buffered_append_test();
	skit_slice_dup_test();
	skit_loaf_assign_cstr_test();
	skit_slice_of_test();
	skit_loaf_free_test();
	skit_slice_common_prefix_test();
	skit_slice_ascii_cmp_test();
	skit_slice_comparison_ops_test();
	skit_slice_trim_test();
	skit_slice_truncate_test();
	printf("  skit_slice_unittest passed!\n");
	printf("\n");
}