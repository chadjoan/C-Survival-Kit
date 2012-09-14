#ifndef SKIT_STRING_INCLUDED
#define SKIT_STRING_INCLUDED

#include <inttypes.h> /* uint8_t, uint16_t, uint32_t */
#include <unistd.h> /* For ssize_t */
#include <limits.h> /* For INT_MAX */

#include "survival_kit/math.h"

/* Used in skit_slice_of.  See skit_slice_of for usage. */
#define SKIT_EOT INT_MAX

/* ----------------------------- string types ------------------------------ */

/** Character types. */
typedef uint8_t  skit_utf8c;
typedef uint16_t skit_utf16c;
typedef uint32_t skit_utf32c;

/* Internal use. */
typedef ssize_t  skit_string_meta;

/**
The generic string type.

The string type system has the following semantics:
(1) A loaf is a string.
(2) A slice is a string.
(3) A loaf cannot be a slice and vica-versa.

This layer of type safety assists with memory management and resource ownership
issues associated with string manipulation.
*/
typedef struct skit_string skit_string;
struct skit_string
{
	skit_utf8c       *chars;
	skit_string_meta meta;
};

/**
If it's possible to have a slice of a string, then it only makes sense that
there be a way to have a loaf of string as well!

The skit_loaf type indicates ownership of the memory used to store the string.
*/
typedef union skit_loaf skit_loaf;
union skit_loaf
{
	skit_utf8c   *chars;
	skit_string  as_string;
};

/**
This is a windowed reference to characters in another string.

Slices do not own the memory used to store the string they represent;
it is the responsibility of the original skit_loaf to manage that memory.

Slices are a very efficient way to manipulate strings without having to
copy string data just to partition a string into useful substrings with
metadata (eg. what parsers do).  

It is, of course, possible to directly manipulate the character data
in the original loaf because the slice is simply a reference to that data.
This is not recommended unless the code calling such operations has been
made very aware that their data may be manipulated.  
*/
typedef union skit_slice skit_slice;
union skit_slice
{
	skit_utf8c   *chars;
	skit_string  as_string;
};

/* --------------------- fundamental string functions ---------------------- */

/**
These are functions for creating initialized-but-null values for all of the
different types of strings.
*/
skit_string skit_string_null();
skit_slice skit_slice_null();
skit_loaf skit_loaf_null();

/**
Dynamically allocates a loaf of zero length using skit_malloc.
The allocated memory will contain a nul character and calling 
skit_loaf_len on the resulting loaf will return 0.
*/
skit_loaf skit_string_new();

/**
Dynamically allocates a loaf with the same length as the given nul-terminated
C string and then copies the C string into the loaf.
The allocation will be performed with skit_malloc.
A nul character will be placed loaf.chars[strlen(cstr)].
*/
skit_loaf skit_loaf_copy_cstr(const char *cstr);

/**
Dynamically allocates a loaf of the given 'length' using skit_malloc.
The resulting string will be uninitialzed, except for the nul character that
will be placed at loaf.chars[length].
*/
skit_loaf skit_loaf_alloc(size_t length);

/**
These functions calculate the length of the given string/loaf/slice.
This is an O(1) operation because the length information is stored
in a string's 'meta' field.  It is safe to assume, however, that this operation
is slightly more complicated than a simple variable access, so any
high-performance code should probably cache string lengths whenever it is
safe to do so.
Example:
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(skit_loaf_len(loaf) == 10);
*/
ssize_t skit_string_len(skit_string str);
ssize_t skit_loaf_len (skit_loaf loaf);
ssize_t skit_slice_len(skit_slice slice);

/** 
These functions will return 0 if the given string/loaf/slice is uninitialized.
They will return 1 if there is a high probability that it is initialized.
Given that it is impossible to REALLY tell if memory has been initialized,
this may give a false positive if the given string/loaf/slice just happens to
have valid initialization check patterns in it due to coincidence.
Nonetheless, this check is still performed in a number of places as an attempt
to catch whatever bugs are possible to catch by this method.
	skit_string str;
	if ( skit_string_check_init(str) )
		printf("skit_string_check_init: False positive!\n");
	else
		printf("skit_string_check_init: Caught an uninitialized string!\n");
*/
int skit_string_check_init(skit_string str);
int skit_loaf_check_init (skit_loaf loaf);
int skit_slice_check_init(skit_slice slice);

/**
Creates a slice of the given nul-terminated C string.
Example:
	skit_slice slice = skit_slice_of_cstr("foo");
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
	sASSERT_EQS((char*)slice.chars, "foo");
*/
skit_slice skit_slice_of_cstr(const char *cstr);

/**
Returns 1 if the given 'str' is actually a loaf.
Returns 0 otherwise.
Example:
	skit_string loaf = skit_loaf_null().as_string;
	skit_string slice = skit_slice_null().as_string;
	sASSERT(skit_string_is_loaf(loaf));
	sASSERT(!skit_string_is_loaf(slice));
*/
int skit_string_is_loaf(skit_string str);

/**
Returns 1 if the given 'str' is actually a slice.
Returns 0 otherwise.
Example:
	skit_string loaf = skit_loaf_null().as_string;
	skit_string slice = skit_slice_null().as_string;
	sASSERT(!skit_string_is_slice(loaf));
	sASSERT(skit_string_is_slice(slice));
*/
int skit_string_is_slice(skit_string str);

/** 
Does a checked cast of the given string into a loaf.
An assertion will trigger if the given 'str' isn't actually a skit_loaf.
This is NOT a conversion.  It just reinterprets the bytes of the given string.
If the original string is a slice, then the safe way of preparing it for
use in operations requiring a loaf is to call skit_string_dup(slice.as_string)
on the slice to obtain a loaf, and then free the loaf when done using
skit_loaf_free(loaf).
*/
skit_loaf skit_string_as_loaf(skit_string str);

/** 
Does a checked cast of the given string into a slice.
An assertion will trigger if the given 'str' isn't actually a skit_slice.
This is NOT a conversion.  It just reinterprets the bytes of the given string.
*/
skit_slice skit_string_as_slice(skit_string str);

/**
Resizes the given 'loaf' to the given 'length'.
This will call skit_realloc to adjust the underlying memory size.
'loaf' will be altered in-place, and the result will also be returned.
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("Hello world!");
	skit_loaf_resize(&loaf, 5);
	sASSERT_EQS("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
*/
skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length);


/**
Appends 'str2' onto the end of 'loaf1'.
The additional memory needed is created using realloc. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	skit_loaf_append(&loaf, skit_slice_of_cstr(" world!").as_string);
	sASSERT_EQS("Hello world!", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
*/
skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_string str2);

/** 
This is similar to skit_string_append: the return value is the result of
concatenating str1 and str2.  
The difference is that the result in this case is always freshly allocated
memory, with the contents being copied from str1 and str2.
The advantage is that this allows the operation to be valid for both
slices and loafs.
It has the disadvantage of requiring more dynamic allocation.
The caller must eventually free the returned skit_loaf entity.
Example:
	skit_loaf  orig  = skit_loaf_copy_cstr("Hello world!");
	skit_slice slice = skit_slice_of(orig.as_string, 0, 6);
	skit_loaf  newb  = skit_string_join(slice.as_string, orig.as_string);
	sASSERT_EQS("Hello Hello world!", skit_loaf_as_cstr(newb));
	skit_loaf_free(&orig);
	skit_loaf_free(&newb);
*/
skit_loaf skit_string_join(skit_string str1, skit_string str2);

/**
Duplicates the given string 'str'.
This is the recommended way to obtain a loaf when given a slice.
This function allocates memory, so be sure to clean up as needed.
Example:
	skit_loaf foo = skit_loaf_copy_cstr("foo");
	skit_slice slice = skit_slice_of(foo);
	skit_loaf bar = skit_string_dup(slice.as_string);
	sASSERT(foo.chars != bar.chars);
	skit_loaf_assign_cstr(&bar, "bar");
	sASSERT_EQS(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQS(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
*/
skit_loaf skit_string_dup(skit_string str);

/**
Replaces the contents of the given loaf with a copy of the text in the given
nul-terminated C string.  It will use skit_loaf_resize to handle any necessary
memory resizing.
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	sASSERT_EQS( skit_loaf_as_cstr(loaf), "Hello" );
	skit_loaf_assign_cstr(&loaf, "Hello world!");
	sASSERT_EQS( skit_loaf_as_cstr(loaf), "Hello world!" );
	skit_loaf_free(&loaf);
*/
skit_loaf *skit_loaf_assign_cstr(skit_loaf *loaf, const char *cstr);

/**
Takes a slice of the given string.
This is the recommended way to obtain a slice when given a loaf.
This is a reasonably fast operation that does not allocate memory.  The loaf
is responsible for managing the memory held by either resulting string.
The indexing is zero-based.
It is possible to create a zero-length slice.
It is not possible to create a negative-length slice: this will cause an
assertion to trigger and crash the caller.
As a convenience, passing negative indices will count from the right
side of the array, and passing SKIT_EOT as the second index will slice to
the very end of the array:
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_string str = loaf.as_string;
	skit_slice slice1 = skit_slice_of(str, 3, -1);
	skit_slice slice2 = skit_slice_of(str, 3, SKIT_EOT);
	char *cstr1 = skit_slice_dup_as_cstr(slice1);
	char *cstr2 = skit_slice_dup_as_cstr(slice2);
	sASSERT_EQS(cstr1, "ba");
	sASSERT_EQS(cstr2, "bar");
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_of(skit_string str, ssize_t index1, ssize_t index);

/** 
Returns the loaf's string as a nul-terminated C character pointer.
This operation does not require an allocation because skit_loaf entities
are already nul-terminated.
*/
char *skit_loaf_as_cstr(skit_loaf loaf);

/** 
Returns a copy of the slice's string as a nul-terminated C character pointer.
This operation calls skit_malloc to allocate the required memory.
The caller is responsible for free'ing the returned C string.
*/
char *skit_slice_dup_as_cstr(skit_slice slice);

/**
Use this to free memory held by 'loaf'.
'loaf' will be reinitialized to have a NULL ptr and zero length.
Example:
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(loaf.chars != NULL);
	skit_loaf_free(&loaf);
	sASSERT(loaf.chars == NULL);
	sASSERT(skit_loaf_len(loaf) == 0);
*/
skit_loaf *skit_loaf_free(skit_loaf *loaf);

/* ------------------------- string misc functions ------------------------- */

/**
Gets the common prefix of the two strings.
This will return a slice of 'str1' whose contents are the common prefix.
If there is no common prefix between the two strings, then a zero-length slice
pointing at the beginning of str1 will be returned.
Example:
	skit_loaf loaf1 = skit_loaf_copy_cstr("foobar");
	skit_loaf loaf2 = skit_loaf_copy_cstr("foobaz");
	skit_string str1 = loaf1.as_string;
	skit_string str2 = loaf2.as_string;
	skit_slice prefix = skit_string_common_prefix(str1, str2);
	char *cstr = skit_slice_dup_as_cstr(prefix);
	sASSERT_EQS(cstr, "fooba");
	skit_free(cstr);
	skit_loaf_free(&loaf1);
	skit_loaf_free(&loaf2);
*/
skit_slice skit_string_common_prefix(const skit_string str1, const skit_string str2);

/**
Performs an asciibetical comparison of the two strings.

+-------+--------------------------------------+
| Value |  Relationship between str1 and str2  |
+-------+--------------------------------------+
|  < 0  | str1 is less than str2               |
|   0   | str1 is equal to str2                |
|  > 0  | str1 is greater than str2            |
+-------+--------------------------------------+

Example:
	skit_loaf bigstr = skit_loaf_copy_cstr("Big string!");
	skit_loaf lilstr = skit_loaf_copy_cstr("lil str.");
	skit_loaf aaa = skit_loaf_copy_cstr("aaa");
	skit_loaf bbb = skit_loaf_copy_cstr("bbb");
	skit_loaf aaab = skit_loaf_copy_cstr("aaab");
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
*/
int skit_string_ascii_cmp(const skit_string str1, const skit_string str2);

/**
Convenient asciibetical comparison functions.
Example:
	skit_loaf aaa = skit_loaf_copy_cstr("aaa");
	skit_loaf bbb = skit_loaf_copy_cstr("bbb");
	skit_string alphaLo = aaa.as_string;
	skit_string alphaHi = bbb.as_string;
	
	sASSERT(!skit_string_ges(alphaLo,alphaHi)); // alphaLo >= alphaHi
	sASSERT( skit_string_ges(alphaHi,alphaLo)); // alphaHi >= alphaLo
	sASSERT( skit_string_ges(alphaHi,alphaHi)); // alphaHi >= alphaHi
	sASSERT(!skit_string_gts(alphaLo,alphaHi)); // alphaLo >  alphaHi
	sASSERT( skit_string_gts(alphaHi,alphaLo)); // alphaHi >  alphaLo
	sASSERT(!skit_string_gts(alphaHi,alphaHi)); // alphaHi >  alphaHi
	
	sASSERT( skit_string_les(alphaLo,alphaHi)); // alphaLo <= alphaHi
	sASSERT(!skit_string_les(alphaHi,alphaLo)); // alphaHi <= alphaLo
	sASSERT( skit_string_les(alphaHi,alphaHi)); // alphaHi <= alphaHi
	sASSERT( skit_string_lts(alphaLo,alphaHi)); // alphaLo <  alphaHi
	sASSERT(!skit_string_lts(alphaHi,alphaLo)); // alphaHi <  alphaLo
	sASSERT(!skit_string_lts(alphaHi,alphaHi)); // alphaHi <  alphaHi
	
	sASSERT(!skit_string_eqs(alphaLo,alphaHi)); // alphaLo == alphaHi
	sASSERT(!skit_string_eqs(alphaHi,alphaLo)); // alphaHi == alphaLo
	sASSERT( skit_string_eqs(alphaHi,alphaHi)); // alphaHi == alphaHi
	sASSERT( skit_string_nes(alphaLo,alphaHi)); // alphaLo != alphaHi
	sASSERT( skit_string_nes(alphaHi,alphaLo)); // alphaHi != alphaLo
	sASSERT(!skit_string_nes(alphaHi,alphaHi)); // alphaHi != alphaHi
	
	skit_loaf_free(&aaa);
	skit_loaf_free(&bbb);
*/
int skit_string_ges(const skit_string str1, const skit_string str2);
int skit_string_gts(const skit_string str1, const skit_string str2);
int skit_string_les(const skit_string str1, const skit_string str2);
int skit_string_lts(const skit_string str1, const skit_string str2);
int skit_string_eqs(const skit_string str1, const skit_string str2);
int skit_string_nes(const skit_string str1, const skit_string str2);

/** 
Trim whitespace from 'str'.
Always returns a slice. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("  foo \n");
	skit_string str = loaf.as_string;
	skit_string slice1 = skit_slice_ltrim(str).as_string;
	skit_string slice2 = skit_slice_rtrim(str).as_string;
	skit_string slice3 = skit_slice_trim (str).as_string;
	sASSERT( skit_string_eqs(slice1, skit_slice_of_cstr("foo \n").as_string) );
	sASSERT( skit_string_eqs(slice2, skit_slice_of_cstr("  foo").as_string) );
	sASSERT( skit_string_eqs(slice3, skit_slice_of_cstr("foo").as_string) );
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_ltrim(const skit_string str);
skit_slice skit_slice_rtrim(const skit_string str);
skit_slice skit_slice_trim(const skit_string str);

/**
Truncate 'nchars' from the left or right side of 'str'.
Always returns a slice. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_string str = loaf.as_string;
	skit_string slice1 = skit_slice_ltruncate(str,3).as_string;
	skit_string slice2 = skit_slice_rtruncate(str,3).as_string;
	sASSERT( skit_string_eqs(slice1, skit_slice_of_cstr("bar").as_string) );
	sASSERT( skit_string_eqs(slice2, skit_slice_of_cstr("foo").as_string) );
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_ltruncate(const skit_string str, size_t nchars);
skit_slice skit_slice_rtruncate(const skit_string str, size_t nchars);

/* Unittests all string functions. */
void skit_string_unittest();

#endif