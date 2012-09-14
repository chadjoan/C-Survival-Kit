
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
	(META_CHECK_VAL | (1ULL << META_STRIDE_SHIFT))

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
	sASSERT(skit_loaf_check_init(result));
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

static void skit_string_len_test()
{
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(skit_loaf_len(loaf) == 10);
	printf("  skit_string_len_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_string_check_init(skit_string str)
{
	if ( META_CHECK_VAL == (str.meta & META_CHECK_MASK) )
		return 1;
	return 0;
}

int skit_loaf_check_init (skit_loaf loaf)   { return skit_string_check_init(loaf.as_string);  }
int skit_slice_check_init(skit_slice slice) { return skit_string_check_init(slice.as_string); }

static void skit_string_check_init_test()
{
	skit_string str;
	if ( skit_string_check_init(str) )
		printf("  skit_string_check_init: False positive!\n");
	else
		printf("  skit_string_check_init: Caught an uninitialized string!\n");
	printf("  skit_string_check_init_test finished.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of_lit(const char *literal)
{
	skit_slice result = skit_slice_null();
	ssize_t length = strlen(literal);
	result.chars = (skit_utf8c*)literal;
	skit_string_setlen(&result.as_string, length);
	return result;
}

static void skit_slice_of_lit_test()
{
	skit_slice slice = skit_slice_of_lit("foo");
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
	sASSERT_EQS((char*)slice.chars, "foo");
	printf("  skit_slice_of_lit_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_string_is_loaf(skit_string str)
{
	if ( (str.meta & META_SLICE_BIT) == 0 )
		return 1;
	return 0;
}

static void skit_string_is_loaf_test()
{
	skit_string loaf = skit_loaf_null().as_string;
	skit_string slice = skit_slice_null().as_string;
	sASSERT(skit_string_is_loaf(loaf));
	sASSERT(!skit_string_is_loaf(slice));
	printf("  skit_string_is_loaf_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_string_is_slice(skit_string str)
{
	return !skit_string_is_loaf(str);
}

static void skit_string_is_slice_test()
{
	skit_string loaf = skit_loaf_null().as_string;
	skit_string slice = skit_slice_null().as_string;
	sASSERT(!skit_string_is_slice(loaf));
	sASSERT(skit_string_is_slice(slice));
	printf("  skit_string_is_slice_test passed.\n");
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
	sASSERT_MSG(skit_loaf_check_init(*loaf), "'loaf' was not initialized.");
	
	loaf->chars = skit_realloc(loaf->chars, length+1);
	loaf->chars[length] = '\0';
	skit_string_setlen(&loaf->as_string, length);
	
	return loaf;
}

static void skit_loaf_resize_test()
{
	skit_loaf loaf = skit_loaf_copy_lit("Hello world!");
	skit_loaf_resize(&loaf, 5);
	sASSERT_EQS("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_resize_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_string str2)
{
	size_t len1;
	size_t len2;
	sASSERT(loaf1 != NULL);
	sASSERT(loaf1->chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT_MSG(skit_loaf_check_init(*loaf1), "'loaf1' was not initialized.");
	sASSERT_MSG(skit_string_check_init(str2), "'str2' was not initialized.");
	
	len2 = skit_string_len(str2);
	if ( len2 == 0 )
		return loaf1;
	
	len1 = skit_loaf_len(*loaf1);
	
	loaf1 = skit_loaf_resize(loaf1, len1+len2);
	memcpy(loaf1->chars + len1, str2.chars, len2);
	/* setlen was already handled by skit_string_resize. */
	
	return loaf1;
}

static void skit_loaf_append_test()
{
	skit_loaf loaf = skit_loaf_copy_lit("Hello");
	skit_loaf_append(&loaf, skit_slice_of_lit(" world!").as_string);
	sASSERT_EQS("Hello world!", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
	printf("  skit_loaf_append_test passed.\n");
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

static void skit_string_join_test()
{
	skit_loaf  orig  = skit_loaf_copy_lit("Hello world!");
	skit_slice slice = skit_slice_of(orig.as_string, 0, 6);
	skit_loaf  newb  = skit_string_join(slice.as_string, orig.as_string);
	sASSERT_EQS("Hello Hello world!", skit_loaf_as_cstr(newb));
	skit_loaf_free(&orig);
	skit_loaf_free(&newb);
	printf("  skit_string_join_test passed.\n");
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

static void skit_string_dup_test()
{
	skit_loaf foo = skit_loaf_copy_lit("foo");
	skit_slice slice = skit_slice_of(foo.as_string, 0, 0);
	skit_loaf bar = skit_string_dup(slice.as_string);
	sASSERT(foo.chars != bar.chars);
	skit_loaf_assign_lit(&bar, "bar");
	sASSERT_EQS(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQS(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
	printf("  skit_string_dup_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_loaf *skit_loaf_assign_lit(skit_loaf *loaf, const char *literal)
{
	ssize_t length = strlen(literal);
	skit_loaf_resize(loaf, length);
	strcpy((char*)loaf->chars, literal);
	return loaf;
}

static void skit_loaf_assign_lit_test()
{
	skit_loaf loaf = skit_loaf_copy_lit("Hello");
	sASSERT_EQS( skit_loaf_as_cstr(loaf), "Hello" );
	skit_loaf_assign_lit(&loaf, "Hello world!");
	sASSERT_EQS( skit_loaf_as_cstr(loaf), "Hello world!" );
	skit_loaf_free(&loaf);
	printf("  skit_loaf_assign_lit_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_of(skit_string str, ssize_t index1, ssize_t index2)
{
	skit_slice result = skit_slice_null();
	ssize_t length = skit_string_len(str);
	
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
	result.chars = str.chars + index1;
	skit_string_setlen(&result.as_string, index2-index1);
	
	return result;
}

static void skit_slice_of_test()
{
	skit_loaf loaf = skit_loaf_copy_lit("foobar");
	skit_string str = loaf.as_string;
	skit_slice slice1 = skit_slice_of(str, 3, -1);
	skit_slice slice2 = skit_slice_of(str, 3, SKIT_EOT);
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

skit_slice skit_string_common_prefix(const skit_string str1, const skit_string str2)
{
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
	ssize_t len1 = skit_string_len(str1);
	ssize_t len2 = skit_string_len(str2);

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

static void skit_string_common_prefix_test()
{
	skit_loaf loaf1 = skit_loaf_copy_lit("foobar");
	skit_loaf loaf2 = skit_loaf_copy_lit("foobaz");
	skit_string str1 = loaf1.as_string;
	skit_string str2 = loaf2.as_string;
	skit_slice prefix = skit_string_common_prefix(str1, str2);
	char *cstr = skit_slice_dup_as_cstr(prefix);
	sASSERT_EQS(cstr, "fooba");
	skit_free(cstr);
	skit_loaf_free(&loaf1);
	skit_loaf_free(&loaf2);
	printf("  skit_string_common_prefix_test passed.\n");
}

/* ------------------------------------------------------------------------- */

int skit_string_ascii_cmp(const skit_string str1, const skit_string str2)
{
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
	ssize_t len1 = skit_string_len(str1);
	ssize_t len2 = skit_string_len(str2);
	
	if ( len1 != len2 )
		return len1 - len2;
	else
	{
		skit_utf8c c1;
		skit_utf8c c2;
		skit_slice slice = skit_string_common_prefix(str1,str2);
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

static void skit_string_ascii_cmp_test()
{
	skit_loaf bigstr = skit_loaf_copy_lit("Big string!");
	skit_loaf lilstr = skit_loaf_copy_lit("lil str.");
	skit_loaf aaa = skit_loaf_copy_lit("aaa");
	skit_loaf bbb = skit_loaf_copy_lit("bbb");
	skit_loaf aaab = skit_loaf_copy_lit("aaab");
	skit_slice aaa_slice = skit_slice_of(aaab.as_string,0,3);
	sASSERT(skit_string_ascii_cmp(lilstr.as_string, bigstr.as_string) < 0); 
	sASSERT(skit_string_ascii_cmp(bigstr.as_string, lilstr.as_string) > 0);
	sASSERT(skit_string_ascii_cmp(bigstr.as_string, bigstr.as_string) == 0);
	sASSERT(skit_string_ascii_cmp(aaa.as_string, bbb.as_string) < 0);
	sASSERT(skit_string_ascii_cmp(bbb.as_string, aaa.as_string) > 0);
	sASSERT(skit_string_ascii_cmp(aaa.as_string, aaa_slice.as_string) == 0);
	skit_loaf_free(&bigstr);
	skit_loaf_free(&lilstr);
	skit_loaf_free(&aaa);
	skit_loaf_free(&bbb);
	printf("  skit_string_ascii_cmp_test passed.\n");
}

/* ------------------------------------------------------------------------- */

/**
Convenient asciibetical comparison functions.
Example:
*/

int skit_string_ges(const skit_string str1, const skit_string str2)
{
	if ( skit_string_ascii_cmp(str1,str2) >= 0 )
		return 1;
	return 0;
}

int skit_string_gts(const skit_string str1, const skit_string str2)
{
	if ( skit_string_ascii_cmp(str1,str2) > 0 )
		return 1;
	return 0;
}

int skit_string_les(const skit_string str1, const skit_string str2)
{
	if ( skit_string_ascii_cmp(str1,str2) <= 0 )
		return 1;
	return 0;
}

int skit_string_lts(const skit_string str1, const skit_string str2)
{
	if ( skit_string_ascii_cmp(str1,str2) < 0 )
		return 1;
	return 0;
}

int skit_string_eqs(const skit_string str1, const skit_string str2)
{
	if ( skit_string_ascii_cmp(str1,str2) == 0 )
		return 1;
	return 0;
}

int skit_string_nes(const skit_string str1, const skit_string str2)
{
	if ( skit_string_ascii_cmp(str1,str2) != 0 )
		return 1;
	return 0;
}

static void skit_string_comparison_ops_test()
{
	skit_loaf aaa = skit_loaf_copy_lit("aaa");
	skit_loaf bbb = skit_loaf_copy_lit("bbbb");
	skit_string alphaLo = aaa.as_string;
	skit_string alphaHi = bbb.as_string;
	
	sASSERT(!skit_string_ges(alphaLo,alphaHi)); /* alphaLo >= alphaHi */
	sASSERT( skit_string_ges(alphaHi,alphaLo)); /* alphaHi >= alphaLo */
	sASSERT( skit_string_ges(alphaHi,alphaHi)); /* alphaHi >= alphaHi */
	sASSERT(!skit_string_gts(alphaLo,alphaHi)); /* alphaLo >  alphaHi */
	sASSERT( skit_string_gts(alphaHi,alphaLo)); /* alphaHi >  alphaLo */
	sASSERT(!skit_string_gts(alphaHi,alphaHi)); /* alphaHi >  alphaHi */

	sASSERT( skit_string_les(alphaLo,alphaHi)); /* alphaLo <= alphaHi */
	sASSERT(!skit_string_les(alphaHi,alphaLo)); /* alphaHi <= alphaLo */
	sASSERT( skit_string_les(alphaHi,alphaHi)); /* alphaHi <= alphaHi */
	sASSERT( skit_string_lts(alphaLo,alphaHi)); /* alphaLo <  alphaHi */
	sASSERT(!skit_string_lts(alphaHi,alphaLo)); /* alphaHi <  alphaLo */
	sASSERT(!skit_string_lts(alphaHi,alphaHi)); /* alphaHi <  alphaHi */

	sASSERT(!skit_string_eqs(alphaLo,alphaHi)); /* alphaLo == alphaHi */
	sASSERT(!skit_string_eqs(alphaHi,alphaLo)); /* alphaHi == alphaLo */
	sASSERT( skit_string_eqs(alphaHi,alphaHi)); /* alphaHi == alphaHi */
	sASSERT( skit_string_nes(alphaLo,alphaHi)); /* alphaLo != alphaHi */
	sASSERT( skit_string_nes(alphaHi,alphaLo)); /* alphaHi != alphaLo */
	sASSERT(!skit_string_nes(alphaHi,alphaHi)); /* alphaHi != alphaHi */
	
	skit_loaf_free(&aaa);
	skit_loaf_free(&bbb);
	printf("  skit_string_comparison_ops_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_ltrim(const skit_string str)
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

skit_slice skit_slice_rtrim(const skit_string str)
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

skit_slice skit_slice_trim(const skit_string str)
{
	return skit_slice_ltrim( skit_slice_rtrim(str).as_string );
}

static void skit_slice_trim_test()
{
	skit_loaf loaf = skit_loaf_copy_lit("  foo \n");
	skit_string str = loaf.as_string;
	skit_string slice1 = skit_slice_ltrim(str).as_string;
	skit_string slice2 = skit_slice_rtrim(str).as_string;
	skit_string slice3 = skit_slice_trim (str).as_string;
	sASSERT( skit_string_eqs(slice1, skit_slice_of_lit("foo \n").as_string) );
	sASSERT( skit_string_eqs(slice2, skit_slice_of_lit("  foo").as_string) );
	sASSERT( skit_string_eqs(slice3, skit_slice_of_lit("foo").as_string) );
	skit_loaf_free(&loaf);
	printf("  skit_slice_trim_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_slice_ltruncate(const skit_string str, size_t nchars)
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

skit_slice skit_slice_rtruncate(const skit_string str, size_t nchars)
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

static void skit_slice_truncate_test()
{
	skit_loaf loaf = skit_loaf_copy_lit("foobar");
	skit_string str = loaf.as_string;
	skit_string slice1 = skit_slice_ltruncate(str,3).as_string;
	skit_string slice2 = skit_slice_rtruncate(str,3).as_string;
	sASSERT( skit_string_eqs(slice1, skit_slice_of_lit("bar").as_string) );
	sASSERT( skit_string_eqs(slice2, skit_slice_of_lit("foo").as_string) );
	skit_loaf_free(&loaf);
	printf("  skit_slice_truncate_test passed.\n");
}

/* ========================================================================= */
/* ----------------------------- unittest list ----------------------------- */

void skit_string_unittest()
{
	printf("skit_string_unittest()\n");
	skit_string_len_test();
	skit_string_check_init_test();
	skit_slice_of_lit_test();
	skit_string_is_loaf_test();
	skit_string_is_slice_test();
	skit_loaf_resize_test();
	skit_loaf_append_test();
	skit_string_join_test();
	skit_string_dup_test();
	skit_loaf_assign_lit_test();
	skit_slice_of_test();
	skit_loaf_free_test();
	skit_string_common_prefix_test();
	skit_string_ascii_cmp_test();
	skit_string_comparison_ops_test();
	skit_slice_trim_test();
	skit_slice_truncate_test();
	printf("  skit_string_unittest passed!\n");
	printf("\n");
}