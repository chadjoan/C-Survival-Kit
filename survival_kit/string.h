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
	sASSERT_EQ(skit_loaf_len(loaf), 10, "%d");
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
	sASSERT_EQ_CSTR((char*)slice.chars, "foo");
*/
skit_slice skit_slice_of_cstr(const char *cstr);

/**
This macro is shorthand for skit_slice_of_cstr.
It provides a concise way making C-style strings compatible with skit_slices.
Example:
	skit_slice slice = sSLICE("foo");
	sASSERT_EQ(skit_slice_len(slice), 3, "%d");
	sASSERT_EQ_CSTR((char*)slice.chars, "foo");
*/
#define sSLICE(cstr) (skit_slice_of_cstr((cstr)))

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
	sASSERT_EQ_CSTR("Hello", skit_loaf_as_cstr(loaf));
	skit_loaf_free(&loaf);
*/
skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length);


/**
Appends 'str2' onto the end of 'loaf1'.
The additional memory needed is created using realloc. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	skit_loaf_append(&loaf, sSLICE(" world!"));
	sASSERT_EQ_CSTR("Hello world!", skit_loaf_as_cstr(loaf));
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
	skit_loaf  newb  = skit_slice_concat(slice, orig.as_slice);
	sASSERT_EQ_CSTR("Hello Hello world!", skit_loaf_as_cstr(newb));
	skit_loaf_free(&orig);
	skit_loaf_free(&newb);
*/
skit_loaf skit_slice_concat(skit_slice str1, skit_slice str2);

/**
This resizes 'buf_slice' to 'new_buf_slice_length' by growing or shrinking it
within the given 'buffer'.  If 'buffer' is not large enough to handle 
the new size, then it will use skit_loaf_resize to increase its size by an 
implementation-defined amount that will make it at least large enough.

It is assumed that 'buf_slice' is already a slice of 'buffer'.  If this is 
not true, then call this function will cause an assertion to be triggered.

If a buffer resize was necessary to fit the new slice, and assuming the
resize allocated more than enough memory, then a 0-byte (C string 
nul-terminating character) will be placed after the end of the new slice 
in addition to the usual nul character after the end of the loaf. 

Example:
	skit_loaf buffer = skit_loaf_alloc(5);
	skit_slice slice = skit_slice_of(buffer.as_slice, 2, 4);
	sASSERT_EQ(skit_slice_len(slice), 2, "%d");
	skit_slice_buffered_resize(&buffer, &slice, 5);
	sASSERT(skit_loaf_len(buffer) >= 6);
	sASSERT_EQ(skit_slice_len(slice), 5, "%d");
	sASSERT_EQ(slice.chars[5], '\0', "%d");
	skit_loaf_free(&buffer);
*/
skit_slice *skit_slice_buffered_resize(
	skit_loaf  *buffer,
	skit_slice *buf_slice,
	ssize_t    new_buf_slice_length);

/** 
Appends a slice 'suffix' to the given slice 'buf_slice'.  'buf_slice' is assumed
to be a slice of 'buffer', thus 'buf_slice' will grow by becoming a larger
slice of 'buffer'.  If 'buffer' is not large enough to contain the growth, then
it will be resized with the semantics described in 
skit_slice_buffered_resize(...).

This operation will overwrite the contents in 'buffer' immediately following
'buf_slice' with a copy of the contents of 'suffix'.

If a buffer resize was necessary to fit the new slice, and assuming the
resize allocated more than enough memory, then a 0-byte (C string 
nul-terminating character) will be placed after the end of the new slice 
in addition to the usual nul character after the end of the loaf. 

Example:
	skit_loaf  buffer = skit_loaf_alloc(5);
	skit_slice accumulator = skit_slice_of(buffer.as_slice, 0, 0);
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("foo"));
	sASSERT_EQ(skit_loaf_len(buffer), 5, "%d");
	sASSERT_EQ(skit_slice_len(accumulator), 3, "%d");
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("bar"));
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(buffer), "foobar");
	sASSERT_GE(skit_loaf_len(buffer), 6, "%d");
	skit_loaf_free(&buffer);
*/
skit_slice *skit_slice_buffered_append(
	skit_loaf  *buffer,
	skit_slice *buf_slice,
	skit_slice suffix);

/**
Duplicates the given 'slice'.
This is the recommended way to obtain a loaf when given a slice.
This function allocates memory, so be sure to clean up as needed.
Example:
	skit_loaf foo = skit_loaf_copy_cstr("foo");
	skit_slice slice = skit_slice_of(foo.as_slice, 0, 0);
	skit_loaf bar = skit_slice_dup(slice);
	sASSERT_NE(foo.chars, bar.chars, "%p");
	skit_loaf_assign_cstr(&bar, "bar");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(bar), "bar");
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
	skit_loaf loaf = skit_loaf_copy_cstr("Hello");
	sASSERT_EQ_CSTR( skit_loaf_as_cstr(loaf), "Hello" );
	skit_loaf_assign_cstr(&loaf, "Hello world!");
	sASSERT_EQ_CSTR( skit_loaf_as_cstr(loaf), "Hello world!" );
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

/**
Returns a standard C printf format specifier that can be used with the .chars
member of a slice to print that slice.  This is done because slices are not
necessarily nul-terminated, and must have length information provided in the
format specifier.

This can be tedious to use, so it is mostly intended to help with implementing
other functions responsible for placing slices into formatted strings 
(ex: sASSERT_EQS).

Example:
	skit_loaf loaf = skit_loaf_copy_cstr("foobar");
	skit_slice slice = skit_slice_of(loaf.as_slice, 0, 3);
	char newstr_buf[128];
	char fmt_str[64];
	char fmt_buf[32];
	skit_slice_get_printf_formatter(slice, fmt_buf, sizeof(fmt_buf) );
	sASSERT_EQ_CSTR( "%.3s", fmt_buf );
	snprintf(fmt_str, sizeof(fmt_str), "Slice is '%s'.", fmt_buf);
	sASSERT_EQ_CSTR(fmt_str, "Slice is '%.3s'.");
	snprintf(newstr_buf, sizeof(newstr_buf), fmt_str, slice.chars);
	sASSERT_EQ_CSTR(newstr_buf, "Slice is 'foo'.");
	skit_loaf_free(&loaf);
*/
char *skit_slice_get_printf_formatter( skit_slice slice, char *buffer, int buf_size );

/* Internal use.  Please do not call directly. */
#define sASSERT_SLICE( assert_name, comparison, lhs, rhs ) \
	do { \
		/* lfmt and rfmt provide space for generating length-maxed printf formatters, */ \
		/* such as: %.124s for a 124-character long slice. */ \
		char lfmt[32]; \
		char rfmt[32]; \
		sASSERT_COMPLICATED( \
			assert_name, \
			comparison, \
			skit_slice_get_printf_formatter( lhs, lfmt, sizeof(lfmt) ), \
			skit_slice_get_printf_formatter( rhs, rfmt, sizeof(rfmt) ), \
			lhs.chars, \
			rhs.chars); \
	} while(0)

/**
Assertions involving comparisons of slices.
These are good to use because they will print the slices involved in the
comparison if anything goes wrong, thus aiding in fast debugging.
*/
#define sASSERT_EQS(lhs,rhs) sASSERT_SLICE("sASSERT_EQS", skit_slice_eqs((lhs),(rhs)), (lhs), (rhs))
#define sASSERT_NES(lhs,rhs) sASSERT_SLICE("sASSERT_NES", skit_slice_nes((lhs),(rhs)), (lhs), (rhs))
#define sASSERT_GES(lhs,rhs) sASSERT_SLICE("sASSERT_GES", skit_slice_ges((lhs),(rhs)), (lhs), (rhs))
#define sASSERT_LES(lhs,rhs) sASSERT_SLICE("sASSERT_LES", skit_slice_les((lhs),(rhs)), (lhs), (rhs))
#define sASSERT_GTS(lhs,rhs) sASSERT_SLICE("sASSERT_GTS", skit_slice_gts((lhs),(rhs)), (lhs), (rhs))
#define sASSERT_LTS(lhs,rhs) sASSERT_SLICE("sASSERT_LTS", skit_slice_lts((lhs),(rhs)), (lhs), (rhs))


/* ------------------------- string misc functions ------------------------- */

/**
Gets the common prefix of the two strings.
This will return a slice of 'str1' whose contents are the common prefix.
If there is no common prefix between the two strings, then a zero-length slice
pointing at the beginning of str1 will be returned.
Example:
	skit_slice slice1 = sSLICE("foobar");
	skit_slice slice2 = sSLICE("foobaz");
	skit_slice prefix = skit_slice_common_prefix(slice1, slice2);
	sASSERT_EQS(prefix, sSLICE("fooba"));
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
*/
int skit_slice_ascii_cmp(const skit_slice str1, const skit_slice str2);

/**
Convenient asciibetical comparison functions.
Example:
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
	sASSERT_EQS( slice1, sSLICE("foo \n") );
	sASSERT_EQS( slice2, sSLICE("  foo") );
	sASSERT_EQS( slice3, sSLICE("foo") );
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
	sASSERT_EQS( slice1, sSLICE("bar") );
	sASSERT_EQS( slice2, sSLICE("foo") );
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_ltruncate(const skit_slice slice, size_t nchars);
skit_slice skit_slice_rtruncate(const skit_slice slice, size_t nchars);

/* Unittests all string functions. */
void skit_string_unittest();

#endif