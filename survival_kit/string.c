
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

#define META_STRIDE_SHIFT (sizeof(skit_string_meta) - 8)
#define META_STRIDE_MASK  (0x0F << META_STRIDE_SHIFT)

#define META_SLICE_SHIFT  (sizeof(skit_string_meta) - 4)
#define META_SLICE_BIT    (0x1 << META_SLICE_SHIFT)

#define META_CHECK_SHIFT  (sizeof(skit_string_meta) - 3)
#define META_CHECK_MASK   (0x7 << META_CHECK_SHIFT)
#define META_CHECK_VAL    (0x5 << META_CHECK_SHIFT)

#define META_LENGTH_MASK  ((1 << sizeof(skit_string_meta)) - 1)
#define META_LENGTH_SHIFT (0)

#define skit_string_init_meta() \
	(META_CHECK_VAL | (1 << META_STRIDE_SHIFT))

static void skit_string_setlen(skit_string *str, size_t len)
{
	str->meta = 
		(str->meta & ~META_LENGTH_MASK) | 
		((len << META_LENGTH_SHIFT) & META_LENGTH_MASK);
}

/* ------------------------------------------------------------------------- */

skit_string skit_string_null()
{
	skit_string result;
	result.chars = NULL;
	result.meta  = skit_string_init_meta();
	return result;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_null()
{
	skit_string result = skit_string_null();
	result.meta |= META_SLICE_BIT; /* set the slice bit. */
	return skit_string_as_slice(result);
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_null()
{
	skit_string result = skit_string_null();
	/* result.meta &= ~META_SLICE_BIT; // clear the slice bit -- already handled by skit_string_null() */
	return skit_string_as_loaf(result);
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_string_new()
{
	skit_loaf result = skit_loaf_null();
	result.chars = (skit_utf8c*)skit_malloc(1);
	result.chars[0] = '\0';
	skit_string_setlen(&result.as_string,0);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_copy_lit(const char *literal)
{
	size_t length = strlen(literal);
	skit_loaf result = skit_loaf_null();
	result.chars = (skit_utf8c*)skit_malloc(length+1);
	strcpy((char*)result.chars, literal);
	skit_string_setlen(&result.as_string, length);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_loaf_alloc(size_t length)
{
	skit_loaf result = skit_loaf_null();
	result.chars = (skit_utf8c*)skit_malloc(length+1);
	result.chars[length] = '\0';
	skit_string_setlen(&result.as_string, length);
	return result;
}

/* ------------------------------------------------------------------------- */

ssize_t skit_string_len(skit_string str)
{
	return (str.meta & META_LENGTH_MASK) >> META_LENGTH_SHIFT;
}

ssize_t skit_loaf_len (skit_loaf loaf)   { return skit_string_len(loaf.as_string);  }
ssize_t skit_slice_len(skit_slice slice) { return skit_string_len(slice.as_string); }

/* ------------------------------------------------------------------------- */

int skit_string_check_init(skit_string str)
{
	if ( META_CHECK_VAL == (str.meta & META_CHECK_MASK) )
		return 1;
	return 0;
}

int skit_loaf_check_init (skit_loaf loaf)   { return skit_string_check_init(loaf.as_string);  }
int skit_slice_check_init(skit_slice slice) { return skit_string_check_init(slice.as_string); }

/* ------------------------------------------------------------------------- */

int skit_string_is_loaf(skit_string str)
{
	if ( (str.meta & META_SLICE_BIT) == 0 )
		return 1;
	return 0;
}

/* ------------------------------------------------------------------------- */

int skit_string_is_slice(skit_string str)
{
	return !skit_string_is_loaf(str);
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_string_as_loaf(skit_string str)
{
	sASSERT(skit_string_check_init(str));
	sASSERT(skit_string_is_loaf(str));
	return (skit_loaf)str;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_string_as_slice(skit_string str)
{
	sASSERT(skit_string_check_init(str));
	sASSERT(skit_string_is_slice(str));
	return (skit_slice)str;
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length)
{
	sASSERT(loaf != NULL);
	sASSERT(loaf->chars != NULL);
	sASSERT(skit_loaf_check_init(*loaf));
	
	loaf->chars = skit_realloc(loaf->chars, length+1);
	loaf->chars[length] = '\0';
	skit_string_setlen(&loaf->as_string, length);
	
	return loaf;
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_string str2)
{
	size_t len1;
	size_t len2;
	sASSERT(loaf1 != NULL);
	sASSERT(loaf1->chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT(skit_loaf_check_init(*loaf1));
	sASSERT(skit_string_check_init(str2));
	
	len2 = skit_string_len(str2);
	if ( len2 == 0 )
		return loaf1;
	
	len1 = skit_loaf_len(*loaf1);
	
	loaf1 = skit_loaf_resize(loaf1, len1+len2);
	memcpy(loaf1->chars + len1, str2.chars, len2);
	/* setlen was already handled by skit_string_resize. */
	
	return loaf1;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_string_join(skit_string str1, skit_string str2)
{
	size_t len1;
	size_t len2;
	skit_loaf result;
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT(skit_string_check_init(str1));
	sASSERT(skit_string_check_init(str2));
	
	len1 = skit_string_len(str1);
	len2 = skit_string_len(str2);
	
	result = skit_loaf_alloc(len1+len2);
	memcpy(result.chars,      str1.chars, len1);
	memcpy(result.chars+len1, str2.chars, len2);
	/* setlen was already handled by skit_loaf_alloc. */
	
	return result;
}

/* ------------------------------------------------------------------------- */

skit_loaf skit_string_dup(skit_string str)
{
	size_t length;
	skit_loaf result;
	sASSERT(str.chars != NULL);
	sASSERT(skit_string_check_init(str));
	
	length = skit_string_len(str);
	result = skit_loaf_alloc(length);
	memcpy(result.chars, str.chars, length);
	/* The call to skit_loaf_alloc will have already handled setlen */
	return result;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of(skit_string str, ssize_t index1, ssize_t index2)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_string_len(str);
	
	/* This implements indexing relative to the end of the string. */
	if ( index1 < 0 )
		index1 = length + index1;
	if ( index2 < 0 )
		index2 = length + index2;
	
	/* No negative slices allowed. */
	sASSERT((index2-index1) > 0);
	
	/* Do the slicing. */
	result.chars = str.chars + index1;
	skit_string_setlen(&result.as_string, index2-index1);
	
	return result;
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

/* ------------------------- string misc functions ------------------------- */

#define IS_WHITESPACE(x) (x == ' '  || x == '\t' || x == '\r' || x == '\n' )

skit_slice str_ltrim(const skit_string str)
{
	skit_slice slice = skit_slice_null();
	
	ssize_t length = skit_string_len(str);
	ssize_t lbound = 0;
	while ( lbound < length )
	{
		skit_utf8c c = str.chars[lbound];
		if (!IS_WHITESPACE(c))
			break;
		
		lbound++;
	}
	
	slice.chars  = str.chars + lbound;
	skit_string_setlen(&slice.as_string, length - lbound);
	
	return slice;
}

/* ------------------------------------------------------------------------- */

skit_slice str_rtrim(const skit_string str)
{
	skit_slice slice = skit_slice_null();
	
	ssize_t length = skit_string_len(str);
	ssize_t rbound = length;
	while ( rbound > 0 )
	{
		char c = str.chars[rbound-1];
		if (!IS_WHITESPACE(c))
			break;
		
		rbound--;
	}
	
	slice.chars  = str.chars;
	skit_string_setlen(&slice.as_string, rbound);
	
	return slice;
}

/* ------------------------------------------------------------------------- */

skit_slice str_trim(const skit_string str)
{
	return str_ltrim( str_rtrim(str).as_string );
}

/* ------------------------------------------------------------------------- */

skit_slice str_ltruncate(const skit_string str, size_t nchars)
{
	skit_slice slice = skit_slice_null();
	ssize_t length = skit_string_len(str);
	
	if ( nchars > length )
	{
		/* The requested truncation is greater than the string length. */
		/* In this case we return a zero-length slice at the end of the string. */
		slice.chars = str.chars;
		skit_string_setlen(&slice.as_string, 0);
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		slice.chars = str.chars + nchars;
		skit_string_setlen(&slice.as_string, length - nchars);
	}
	
	return slice;
}

/* ------------------------------------------------------------------------- */

skit_slice str_rtruncate(const skit_string str, size_t nchars)
{
	skit_slice slice = skit_slice_null();
	ssize_t length = skit_string_len(str);
	
	if ( nchars > length )
	{
		/* The requested truncation is greater than the string length. */
		/* In this case we return a zero-length slice at the beginning of the string. */
		slice.chars = str.chars;
		skit_string_setlen(&slice.as_string, 0);
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		slice.chars = str.chars;
		skit_string_setlen(&slice.as_string, length - nchars);
	}
	
	return slice;
}