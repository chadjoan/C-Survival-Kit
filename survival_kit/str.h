#ifndef STR_UTIL_INCLUDED
#define STR_UTIL_INCLUDED

#include <stdlib.h>

typedef ssize_t skit_string_meta;

typedef skit_string skit_string;
union skit_string
{
	skit_utf8c  *chars;
	skit_loaf   as_loaf;
	skit_slice  as_slice;
};

typedef skit_loaf skit_loaf;
struct skit_loaf
{
	skit_utf8c       *chars;
	skit_string_meta meta;
};

typedef skit_slice skit_slice;
struct skit_slice
{
	skit_utf8c       *chars;
	skit_string_meta meta;
};

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
#define META_SLICE_MASK   (0x1 << META_SLICE_SHIFT)

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

skit_string skit_string_null()
{
	skit_string result;
	result.as_loaf.chars = NULL;
	result.as_loaf.meta  = skit_string_init_meta();
	return result;
}

skit_string skit_string_new()
{
	skit_string result = skit_string_null();
	result.chars = (skit_utf8c*)skit_malloc(1);
	result.chars[0] = '\0';
	skit_string_setlen(&result,0);
	return result;
}

skit_string skit_string_copy_lit(const char *literal)
{
	size_t length = strlen(literal);
	skit_string result = skit_string_null();
	result.chars = (skit_utf8c*)skit_malloc(length+1);
	strcpy(result.chars, literal);
	skit_string_setlen(&result,length);
	return result;
}

skit_string skit_string_alloc(size_t length)
{
	skit_string result = skit_string_null();
	result.chars = (skit_utf8c*)skit_malloc(length+1);
	result.chars[length] = '\0';
	skit_string_setlen(&result,length);
	return result;
}

ssize_t skit_string_len(skit_string str)
{
	return (str.as_loaf.meta & META_LENGTH_MASK) >>> META_LENGTH_SHIFT;
}

int skit_string_check_init(skit_string str)
{
	if ( META_CHECK_VAL == (str.as_loaf.meta & META_CHECK_MASK) )
		return 1;
	return 0;
}

int skit_string_is_loaf(skit_string str)
{
	if ( (str.as_loaf.meta & META_SLICE_MASK) == 0 )
		return 1;
	return 0;
}

int skit_string_is_slice(skit_string str)
{
	return !skit_string_is_loaf(str);
}

/** 
Casts the given slice into a generic string.
This is NOT a conversion.  It just reinterprets the bytes of the given slice.
*/
skit_string *skit_slice_as_string(skit_slice *slice)
{
	/* For now, something really simple like this will work. */
	return (skit_string*)slice;
}

/** 
Casts the given loaf into a generic string.
This is NOT a conversion.  It just reinterprets the bytes of the given loaf.
*/
skit_string *skit_loaf_as_string(skit_loaf *loaf)
{
	/* For now, something really simple like this will work. */
	return (skit_string*)loaf;
}

skit_string *skit_string_resize(skit_string *str, size_t length)
{
	sASSERT(str != NULL);
	sASSERT(str->chars != NULL);
	sASSERT(skit_string_check_init(*str));
	sASSERT(skit_string_is_loaf(*str));
	
	str->chars = skit_realloc(str->chars, length+1);
	str->chars[length] = '\0';
	skit_string_setlen(&str,length);
	
	return str;
}


/**
Appends 'str2' onto the end of 'str1'.
The additional memory needed is created using realloc. 
'str1' must be a loaf.  If it is a slice, then the call will trigger an 
assertion.
*/
skit_string *skit_string_append(skit_string *str1, skit_string str2)
{
	size_t len1;
	size_t len2;
	sASSERT(str1 != NULL);
	sASSERT(str1->chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT(skit_string_check_init(*str1));
	sASSERT(skit_string_check_init(str2));
	sASSERT(skit_string_is_loaf(*str1));
	
	len2 = skit_string_len(str2);
	if ( len2 == 0 )
		return str1;
	
	len1 = skit_string_len(*str1);
	
	str1 = skit_string_resize(str1, len1+len2);
	memcpy(str1.chars + len1, str2.chars, len2);
	/* setlen was already handled by skit_string_resize. */
	
	return str1;
}

/** 
This is similar to skit_string_append: the return value is the result of
concatenating str1 and str2.  
The difference is that the result in this case is always freshly allocated
memory, with the contents being copied from str1 and str2.
This allows the operation to be valid for slices as well as loafs.
*/
skit_string skit_string_join(skit_string str1, skit_string str2)
{
	size_t len1;
	size_t len2;
	sASSERT(str1.chars != NULL);
	sASSERT(str2.chars != NULL);
	sASSERT(skit_string_check_init(str1));
	sASSERT(skit_string_check_init(str2));
	
	len1 = skit_string_len(str1);
	len2 = skit_string_len(str2);
	
	skit_string result = skit_string_alloc(len1+len2);
	memcpy(result.chars,      str1.chars, len1);
	memcpy(reuslt.chars+len1, str2.chars, len2);
	/* setlen was already handled by skit_string_alloc. */
	
	return result;
}

skit_string skit_string_dup(skit_string str)
{
	size_t length;
	skit_string result;
	sASSERT(str.chars != NULL);
	sASSERT(skit_string_check_init(str));
	
	length = skit_string_len(str);
	result = skit_string_alloc(length);
	memcpy(result.chars, str.chars, length);
	/* The call to skit_string_alloc will have already handled setlen */
	sASSERT(skit_string_check_init(*str1));
	return result;
}

skit_string *str_delete(skit_string *str)
{
	sASSERT(str != NULL);
	sASSERT(str->chars != NULL);
	sASSERT(skit_string_check_init(*str));
	sASSERT(skit_string_is_loaf(*str));
	
	skit_free(str->chars);
	*str = skit_string_null();
	return str;
}

/* ------------------------- string misc functions ------------------------- */

/** 
Trim whitespace from 'str'.
Always returns a slice. 
*/
skit_string skit_string_ltrim(skit_string str);
skit_string skit_string_rtrim(skit_string str);
skit_string skit_string_trim(skit_string str);

/* Truncate 'nchars' from the left or right side of 'str'. */
skit_string skit_string_ltruncate(skit_string str, size_t nchars);
skit_string skit_string_rtruncate(skit_string str, size_t nchars);


/* 
Zeroes/Nulls out the string. 
This MUST be called on uninitialized strings before they are fed to 
  str_malloc or any function that expects a NULL string (str->ptr). 
*/
string *str_init(string *str);

/* 
Allocate 'length' bytes for the string 'str'.
str->ptr should be NULL before this is called. 
*/
string *str_malloc(string *str, size_t length);

/*
Similar to str_malloc, except that non-null str->ptr's will be reallocated.
*/
string *str_realloc(string *str, size_t length);

/* Allocates a string and fills it with the given nul-terminated */
/* c-string literal. */
string str_literal(const char *lit);

/* Appends 'str2' onto the end of 'str1'. */
/* The additional memory needed is created using realloc. */
string *str_append(string *str1, string str2);

/* Duplicates the given string 'str'. */
/* This is an allocating function, so be sure to clean up as needed. */
string str_dup(string str);

/* Use this to free memory held by 'str'. */
/* str will be reinitialized to have a NULL ptr and zero length. */
string *str_delete(string *str);


/* ------------------------- string misc functions ------------------------- */

/* Trim whitespace from 'str'. */
string str_ltrim(string str);
string str_rtrim(string str);
string str_trim(string str);

/* Truncate 'nchars' from the left or right side of 'str'. */
string str_ltruncate(string str, size_t nchars);
string str_rtruncate(string str, size_t nchars);

#endif