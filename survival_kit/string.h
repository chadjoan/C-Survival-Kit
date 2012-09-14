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
This string module uses a distinction between complete a complete string
(called a "loaf") and a reference to part of the string (a "slice").  This
allows the caller to avoid string allocations by passing around slices
whenever possible and using only the original string to manage memory.

Originally, this design had a type system like so:
(1) skit_slice, which is a skit_string
(2) skit_loaf, which is a skit_string
(3) skit_loaf and skit_slice cannot be (safely) cast between each other.

To move contents between loaves and slices, it is necessary to use functions
that perform memory management in order to do allocations (hopefully as
infrequently as possible).  

The pattern emerged that skit_slices and skit_strings were essentially
capable of exactly the same things.  There was no functional difference
between the two types; instead there was only an artificial difference created
by the type definitions.

As a result, the skit_string type was removed.  Instead, this type system
is used:
(1) A skit_loaf is a skit_slice (of itself).
(2) A skit_slice is the most general type: it can do fewer operations, but
        never requires any allocations except when moving data into loaves
        or C strings.

The movement of data between loaves and slices is still determined by the
choice of string manipulation functions.

At this point it would be possible to rename all loaves and call them "strings"
instead, but this results in a language ambiguity where slices are a kind of
string (in the English sense) but loaves are a kind of slice (in the semantics
defined in C code).  This is undesirable, so loaves are called loaves.  
Although there is no skit_string type, documention and any discussion may 
refer to either as a string in the English and programming jargon sense of
the term.

The end result is an unambiguous terminology and the ability to avoid string
allocations unless it is strictly necessary.

Other notes:
- In general slices are used in arguments whenever possible.  loaves can be
    easily converted into slices using their '.as_slice' member, so slice
    arguments can effectively receive either string type.
- Functions in this module will not accept NULL values, unless explicitly
    stated otherwise.  Passing NULL values into skit_ string handling functions
    will quickly lead to assertion crashes.
- It is usually a bad idea to pass a skit_loaf into a function as a value
    type.  Either pass it as a pointer or pass it's '.as_slice' value.  Passing
    a skit_loaf as a pointer is a way of communicating to the called function
    that ownership of the loaf is being (temporarily) transferred.
*/

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
typedef struct skit_slice skit_slice;
struct skit_slice
{
	skit_utf8c       *chars;
	skit_string_meta meta;
};

/**
If it's possible to have a slice of a string, then it only makes sense that
there be a way to have a loaf of string as well!

The skit_loaf type indicates ownership of the memory used to store a string.
*/
typedef union skit_loaf skit_loaf;
union skit_loaf
{
	skit_utf8c  *chars;
	skit_slice  as_slice;
};

/* --------------------- fundamental string functions ---------------------- */

/**
These are functions for creating initialized-but-null values for all of the
different types of strings.
*/
skit_slice skit_slice_null();
skit_loaf skit_loaf_null();

/**
Dynamically allocates a loaf of zero length using skit_malloc.
The allocated memory will contain a nul character and calling 
skit_loaf_len on the resulting loaf will return 0.
*/
skit_loaf skit_slice_new();

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
These functions calculate the length of the given loaf/slice.
This is an O(1) operation because the length information is stored
in a string's 'meta' field.  It is safe to assume, however, that this operation
is slightly more complicated than a simple variable access, so any
high-performance code should probably cache string lengths whenever it is
safe to do so.
Example:
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(skit_loaf_len(loaf) == 10);
*/
ssize_t skit_loaf_len (skit_loaf loaf);
ssize_t skit_slice_len(skit_slice slice);

/** 
These functions will return 0 if the given loaf/slice is uninitialized.
They will return 1 if there is a high probability that it is initialized.
Given that it is impossible to REALLY tell if memory has been initialized,
this may give a false positive if the given loaf/slice just happens to
have valid initialization check patterns in it due to coincidence.
Nonetheless, this check is still performed in a number of places as an attempt
to catch whatever bugs are possible to catch by this method.
	skit_slice slice;
	if ( skit_slice_check_init(slice) )
		printf("skit_slice_check_init: False positive!\n");
	else
		printf("skit_slice_check_init: Caught an uninitialized slice!\n");
*/
int skit_slice_check_init(skit_slice slice);
int skit_loaf_check_init (skit_loaf loaf);

/**
Creates a slice of the given nul-terminated C string.
Example:
	skit_slice slice = skit_slice_of_cstr("foo");
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
	sASSERT_EQS((char*)slice.chars, "foo");
*/
skit_slice skit_slice_of_cstr(const char *cstr);

/**
Returns 1 if the given 'slice' is actually a loaf.
Returns 0 otherwise.
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("foo");
	skit_slice casted_slice = loaf.as_slice;
	skit_slice sliced_slice = skit_slice_of(loaf.as_slice, 0, SKIT_EOT);
	sASSERT( skit_slice_is_loaf(casted_slice));
	sASSERT(!skit_slice_is_loaf(sliced_slice));
	skit_loaf_free(&loaf);
*/
int skit_slice_is_loaf(skit_slice slice);

/** 
Does a checked cast of the given slice into a loaf.
An assertion will trigger if the given 'slice' isn't actually a skit_loaf.
This is NOT a conversion.  It just reinterprets the bytes of the given slice.
If you're given a slice and you need a loaf, then the safe way of preparing 
it for use in operations requiring a loaf is to call 
skit_slice_dup(slice.as_slice) on the slice to obtain a loaf, and then free 
the loaf when done using skit_loaf_free(loaf).
*/
skit_loaf skit_slice_as_loaf(skit_slice slice);

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
	skit_loaf_append(&loaf, skit_slice_of_cstr(" world!"));
	sASSERT_EQS("Hello world!", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
*/
skit_loaf *skit_loaf_append(skit_loaf *loaf1, skit_slice str2);

/** 
This is similar to skit_slice_append: the return value is the result of
concatenating str1 and str2.  
The difference is that the result in this case is always freshly allocated
memory, with the contents being copied from str1 and str2.
The advantage is that this allows the operation to be valid for both
slices and loafs.
It has the disadvantage of requiring more dynamic allocation.
The caller must eventually free the returned skit_loaf entity.
Example:
	skit_loaf  orig  = skit_loaf_copy_cstr("Hello world!");
	skit_slice slice = skit_slice_of(orig.as_slice, 0, 6);
	skit_loaf  newb  = skit_slice_join(slice, orig.as_slice);
	sASSERT_EQS("Hello Hello world!", skit_loaf_as_cstr(newb));
	skit_loaf_free(&orig);
	skit_loaf_free(&newb);
*/
skit_loaf skit_slice_join(skit_slice str1, skit_slice str2);

/**

If the caller has ownership of the original loaf that 'buffer' is allocated
from, then calling skit_slice_alloc_using(...) may be considerably more 
convenient and safe, since that function will upsize the buffer as needed to 
prevent buf_slice running over the edge of the buffer.
*/
skit_slice skit_slice_grow_using(skit_slice buffer, skit_slice buf_slice, ssize_t nbytes)
{
	skit_slice result;
	ssize_t plen;
	ssize_t clen;
	skit_utf8c *lbound;
	skit_utf8c *rbound;
	sASSERT(buffer.chars != NULL);
	sASSERT(buf_slice.chars != NULL);
	
	plen = skit_slice_len(buffer);
	clen = skit_slice_len(buf_slice);
	lbound = buffer.chars;
	rbound = buffer.chars + plen;
	sASSERT_MSG(lbound <= buf_slice.chars && buf_slice.chars <= rbound, "The buf_slice given is not a substring of the given buffer.");
	sASSERT_MSG(buf_slice.chars + clen + nbytes <= rbound, "Attempt to grow outside of the buffer string's allocated size.");
	
	/* Lots of checking just to ensure we can do this: */
	result = buf_slice;
	skit_slice_setlen(result, clen + nbytes);
	
	return result;
}

/**
This function increases the size of 'buf_slice' by making it a larger slice of
the given 'buffer'.  If 'buffer' is not large enough to handle the new size,
then it will use skit_loaf_resize to increase its size by an implementation-
defined amount that will make it at least large enough.

It is assumed that 'buf_slice' is a slice of 'buffer'.  If this is not true,
then call this function will cause an assertion to be triggered.

This function is similar to skit_slice_grow_using, with the difference being
that skit_slice_grow_using will not expand 'buffer' as needed.  
The only disadvantage of using skit_slice_alloc_using is that it requires
the caller to own the original reference to 'buffer'.
*/
skit_slice skit_slice_alloc_using(skit_loaf *buffer, skit_slice buf_slice, ssize_t nbytes)
{
	skit_slice result;
	ssize_t plen;
	ssize_t clen;
	skit_utf8c *rbound;
	sASSERT(buffer != NULL);
	
	plen = skit_slice_len(buffer);
	clen = skit_slice_len(buf_slice);
	rbound = buffer.chars + plen;
	
	if ( buf_slice.chars + clen + nbytes > rbound )
	{
		ssize_t new_len = plen;
		if ( new_len <= 1 )
			new_len = 2;
		
		rbound = buffer.chars + new_len;
		while ( buf_slice.chars + clen + nbytes > rbound )
		{
			new_len *= 2;
			rbound = buffer.chars + new_len;
		}
		
		buffer = skit_loaf_resize( buffer, new_len );
	}
	
	result = skit_slice_grow_using(*buffer.as_slice, buf_slice, nbytes);
	
}

skit_slice skit_slice_append_alloc_using(skit_loaf *buffer, skit_slice buf_slice, skit_slice suffix)
{
	skit_slice result;
	ssize_t slen;
	ssize_t clen;
	/* We don't need to check buffer and buf_slice because skit_slice_grow_using will do that. */
	sASSERT(suffix.chars != NULL);
	
	slen = skit_slice_len(suffix);
	clen = skit_slice_len(buf_slice);
	result = skit_slice_alloc_using(buffer, buf_slice, slen);
	memcpy((void*)(result.chars + clen), (void*)suffix.chars, slen);
	
	return result;
}

/**
Duplicates the given 'slice'.
This is the recommended way to obtain a loaf when given a slice.
This function allocates memory, so be sure to clean up as needed.
Example:
	skit_loaf foo = skit_loaf_copy_cstr("foo");
	skit_slice slice = skit_slice_of(foo.as_slice, 0, 0);
	skit_loaf bar = skit_slice_dup(slice);
	sASSERT(foo.chars != bar.chars);
	skit_loaf_assign_cstr(&bar, "bar");
	sASSERT_EQS(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQS(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
*/
skit_loaf skit_slice_dup(skit_slice slice);

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
Takes a slice of the given string/slice.
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
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_of(slice0, 3, -1);
	skit_slice slice2 = skit_slice_of(slice0, 3, SKIT_EOT);
	char *cstr1 = skit_slice_dup_as_cstr(slice1);
	char *cstr2 = skit_slice_dup_as_cstr(slice2);
	sASSERT_EQS(cstr1, "ba");
	sASSERT_EQS(cstr2, "bar");
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_of(skit_slice slice, ssize_t index1, ssize_t index);

/** 
Returns the loaf's contents as a nul-terminated C character pointer.
This operation does not require an allocation because skit_loaf entities
are already nul-terminated.
*/
char *skit_loaf_as_cstr(skit_loaf loaf);

/** 
Returns a copy of the slice's contents as a nul-terminated C character pointer.
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
	skit_slice str1 = loaf1.as_slice;
	skit_slice str2 = loaf2.as_slice;
	skit_slice prefix = skit_slice_common_prefix(str1, str2);
	char *cstr = skit_slice_dup_as_cstr(prefix);
	sASSERT_EQS(cstr, "fooba");
	skit_free(cstr);
	skit_loaf_free(&loaf1);
	skit_loaf_free(&loaf2);
*/
skit_slice skit_slice_common_prefix(const skit_slice str1, const skit_slice str2);

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
*/
int skit_slice_ascii_cmp(const skit_slice str1, const skit_slice str2);

/**
Convenient asciibetical comparison functions.
Example:
	skit_loaf aaa = skit_loaf_copy_cstr("aaa");
	skit_loaf bbb = skit_loaf_copy_cstr("bbb");
	skit_slice alphaLo = aaa.as_slice;
	skit_slice alphaHi = bbb.as_slice;
	
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
	
	skit_loaf_free(&aaa);
	skit_loaf_free(&bbb);
*/
int skit_slice_ges(const skit_slice str1, const skit_slice str2);
int skit_slice_gts(const skit_slice str1, const skit_slice str2);
int skit_slice_les(const skit_slice str1, const skit_slice str2);
int skit_slice_lts(const skit_slice str1, const skit_slice str2);
int skit_slice_eqs(const skit_slice str1, const skit_slice str2);
int skit_slice_nes(const skit_slice str1, const skit_slice str2);

/** 
Trim whitespace from 'slice'.
Always returns a slice. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("  foo \n");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltrim(slice0);
	skit_slice slice2 = skit_slice_rtrim(slice0);
	skit_slice slice3 = skit_slice_trim (slice0);
	sASSERT( skit_slice_eqs(slice1, skit_slice_of_cstr("foo \n")) );
	sASSERT( skit_slice_eqs(slice2, skit_slice_of_cstr("  foo")) );
	sASSERT( skit_slice_eqs(slice3, skit_slice_of_cstr("foo")) );
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_ltrim(const skit_slice slice);
skit_slice skit_slice_rtrim(const skit_slice slice);
skit_slice skit_slice_trim(const skit_slice slice);

/**
Truncate 'nchars' from the left or right side of 'slice'.
Always returns a slice. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltruncate(slice0,3);
	skit_slice slice2 = skit_slice_rtruncate(slice0,3);
	sASSERT( skit_slice_eqs(slice1, skit_slice_of_cstr("bar")) );
	sASSERT( skit_slice_eqs(slice2, skit_slice_of_cstr("foo")) );
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_ltruncate(const skit_slice slice, size_t nchars);
skit_slice skit_slice_rtruncate(const skit_slice slice, size_t nchars);

/* Unittests all string functions. */
void skit_string_unittest();

#endif