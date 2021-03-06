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
typedef int64_t  skit_string_meta;

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
- Both loaves and slices have a max length limit of about 286MB worth of
    characters (2^28 bytes).  
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

Do not manipulate the fields in this struct directly.
To access the characters directly, use sSPTR(slice) or skit_slice_ptr(slice).
To access the slice's length, use sSLENGTH(slice) or skit_slice_len(slice).
*/
typedef struct skit_slice skit_slice;
struct skit_slice
{
	skit_utf8c       *chars_handle;
	skit_string_meta meta;
};

/**
If it's possible to have a slice of a string, then it only makes sense that
there be a way to have a loaf of string as well!

The skit_loaf type indicates ownership of the memory used to store a string.

Do not manipulate the fields in this union directly.
To access the characters directly, use sLPTR(loaf) or skit_loaf_ptr(loaf).
To access the loaf's length, use sLLENGTH(loaf) or skit_loaf_len(loaf).
*/
typedef union skit_loaf skit_loaf;
union skit_loaf
{
	skit_utf8c  *chars_handle;
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
skit_loaf skit_loaf_new();

/**
Dynamically allocates a loaf with the same length as the given nul-terminated
C string and then copies the C string into the loaf.
The allocation will be performed with skit_malloc.
A nul character will be placed at sLPTR(loaf)[strlen(cstr)].
*/
skit_loaf skit_loaf_copy_cstr(const char *cstr);

/**
Dynamically allocates a loaf of the given 'length' using skit_malloc.
The resulting string will be uninitialzed, except for the nul character that
will be placed at sLPTR(loaf)[length].
*/
skit_loaf skit_loaf_alloc(size_t length);

/**
The length of a loaf returned from skit_loaf_emplace will be smaller than
the original memory allocated by this much.  The overhead is used to do
things like allocate a handle and trailing null byte.
*/
#define SKIT_LOAF_EMPLACEMENT_OVERHEAD (sizeof(skit_utf8c*) + 1)

/**
Creates a loaf from the given chunk of caller-allocated memory.  This can
be useful if you have some place to obtain memory more quickly than 
skit_malloc can, and you wish to use that memory to avoid the memory allocation
overhead that happens with calls to skit_loaf_alloc.

skit_free should still be called on loaves allocated this way.  skit_free will
not free the memory introduced here (the 'mem' pointer), but it will free any
auxiliary resources that the loaf might have allocated on its own to deal with
enlarging resizes.  

Example:

	char mem[128];
	size_t mem_size = sizeof(mem);
	skit_loaf loaf = skit_loaf_emplace(mem, mem_size);
	sASSERT_EQ( mem_size, sLLENGTH(loaf) + SKIT_LOAF_EMPLACEMENT_OVERHEAD );
	sASSERT( sLPTR(loaf) != NULL );
	sASSERT( mem <= (char*)sLPTR(loaf) );
	sASSERT( (char*)sLPTR(loaf) < mem+mem_size );
	skit_loaf_free(&loaf);
	skit_free(mem);
	
*/
skit_loaf skit_loaf_emplace( void *mem, size_t mem_size );

/**
This is shorthand for allocating a loaf using stack memory.
Internally it will declare a stack array large enough to hold the desired loaf,
and then call skit_loaf_emplace to turn the memory into a usable loaf.

Example:
	SKIT_LOAF_ON_STACK(loaf, 32);
	sASSERT( sLPTR(loaf) != NULL );
	sASSERT_EQ( sLLENGTH(loaf), 32 );
	skit_loaf_free(&loaf);
*/
#define SKIT_LOAF_ON_STACK( loaf_name, size ) \
	const char *SKIT__LOAF_##loaf_name [size + SKIT_LOAF_EMPLACEMENT_OVERHEAD]; \
	skit_loaf loaf_name = skit_loaf_emplace( SKIT__LOAF_##loaf_name, size + SKIT_LOAF_EMPLACEMENT_OVERHEAD);

/**
These functions calculate the length of the given loaf/slice.
This is an O(1) operation because the length information is stored
in a string's 'meta' field.  It is safe to assume, however, that this operation
is slightly more complicated than a simple variable access, so any
high-performance code should probably cache string lengths whenever it is
safe to do so.
Example:
	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT_EQ(skit_loaf_len(loaf), 10);
*/
ssize_t skit_loaf_len (skit_loaf loaf);
ssize_t skit_slice_len(skit_slice slice); /** ditto */

/**
This macro is shorthand for calling skit_loaf_len.
*/
#define sLLENGTH(loaf) (skit_loaf_len((loaf)))

/**
This macro is shorthand for calling skit_slice_len.
*/
#define sSLENGTH(slice) (skit_slice_len((slice)))

/**
Returns the pointer to the character string's data, thus allowing direct
manipulation of the string's individual characters.

Since any resize operation on a loaf can move the underlying memory for that
loaf, it is important to avoid doing that when working with a pointer returned
from this function.  The loaf could get updated, but the pointer won't, and
will then point to unallocated memory.  Alternatively, always refresh any
pointers to the loaf's data whenever a potentially resizing operation is
called.

Example:
	skit_loaf  loaf = skit_loaf_copy_cstr("foobarbaz");
	skit_slice slice = skit_slice_of(loaf.as_slice, 3, 6);
	sASSERT_EQ(sSPTR(slice) - sLPTR(loaf), 3);
	sASSERT_EQS(slice, sSLICE("bar"));
	skit_loaf_free(&loaf);
*/
skit_utf8c *skit_loaf_ptr( skit_loaf loaf );
skit_utf8c *skit_slice_ptr( skit_slice slice ); /** ditto */

/**
This macro is shorthand for calling skit_loaf_ptr.
*/
#define sLPTR(loaf) (skit_loaf_ptr((loaf)))

/**
This macro is shorthand for calling skit_slice_ptr.
*/
#define sSPTR(slice) (skit_slice_ptr((slice)))

/**
These functions return 1 if the given string is considered null.
They return 0 otherwise.
A string is "null" if it has been initialized but has no memory allocated
to it -- not even an empty string.
Such null strings are returned by the skit_loaf_null() and skit_slice_null()
functions.
Example:
	skit_loaf  nloaf  = skit_loaf_null();
	skit_slice nslice = skit_slice_null();
	skit_loaf  loaf   = skit_loaf_copy_cstr("foo");
	skit_slice slice  = skit_slice_of(loaf.as_slice, 0, 2);
	sASSERT_EQ(skit_loaf_is_null(nloaf),  1);
	sASSERT_EQ(skit_slice_is_null(nloaf), 1);
	sASSERT_EQ(skit_loaf_is_null(loaf),   0);
	sASSERT_EQ(skit_slice_is_null(loaf),  0);
	skit_loaf_free(&loaf);
*/
int skit_loaf_is_null(skit_loaf loaf);
int skit_slice_is_null(skit_slice slice);

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
int skit_loaf_check_init (skit_loaf loaf);
int skit_slice_check_init(skit_slice slice);

/**
Like skit_slice_of_cstr, but only takes a slice with 'length' characters
in it.
This can avoid an O(n) string scan if the caller already knows the desired
length of the given C string.
Example:
	skit_slice slice = skit_slice_of_cstrn("foo",3);
	sASSERT_EQ(skit_slice_len(slice), 3);
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "foo");
*/
skit_slice skit_slice_of_cstrn(const char *cstr, int length );

/**
Creates a slice of the given nul-terminated C string.
Example:
	skit_slice slice = skit_slice_of_cstr("foo");
	sASSERT_EQ(skit_slice_len(slice), 3);
	sASSERT_EQ_CSTR((char*)sSPTR(slice), "foo");
*/
skit_slice skit_slice_of_cstr(const char *cstr);

/**
This macro is shorthand for skit_slice_of_cstr and skit_slice_of_cstrn.
It provides a concise way making C-style strings compatible with
skit_slices.
It may be used with either C-style string literals or null-terminated
C-strings given as pointers.
Take caution when using it with C stack-allocated arrays: behavior may be
undefined in this case.
Example:
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
*/
#define sSLICE(cstr) (sizeof((cstr)) == sizeof(void*) ? skit_slice_of_cstr((cstr)) : skit_slice_of_cstrn((cstr), sizeof((cstr))-1))
/* subtract 1 because sizeof(x) includes the nul byte. */

/**
Resizes the given 'loaf' to the given 'length'.
This will call skit_realloc to adjust the underlying memory size.
'loaf' will be altered in-place, and the result will also be returned.
When growing text, any nul-terminating character immediately following the text
will remain in place, and a nul-terminating character will also be placed at
the very end of the newly allocated memory.
When shrinking text, a nul-terminating character will be placed at the end of
the shrunken text.  Loaves must always have a nul-terminator just past the end
of their length.

Example:
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
	sASSERT_EQ(skit_slice_len(slice), 2);
	skit_slice_buffered_resize(&buffer, &slice, 5);
	sASSERT(skit_loaf_len(buffer) >= 6);
	sASSERT_EQ(skit_slice_len(slice), 5);
	sASSERT_EQ(sSPTR(slice)[5], '\0');
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
	sASSERT_EQ(skit_loaf_len(buffer), 5);
	sASSERT_EQ(skit_slice_len(accumulator), 3);
	skit_slice_buffered_append(&buffer, &accumulator, sSLICE("bar"));
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(buffer), "foobar");
	sASSERT_GE(skit_loaf_len(buffer), 6);
	skit_loaf_free(&buffer);
*/
skit_slice skit_slice_buffered_append(
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
	skit_loaf bar = skit_loaf_dup(slice);
	sASSERT(sLPTR(foo) != sLPTR(bar));
	skit_loaf_store_cstr(&bar, "bar");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(foo), "foo");
	sASSERT_EQ_CSTR(skit_loaf_as_cstr(bar), "bar");
	skit_loaf_free(&foo);
	skit_loaf_free(&bar);
*/
skit_loaf skit_loaf_dup(skit_slice slice);

/**
Replaces the contents of the given loaf with a copy of the text in the given
nul-terminated C string.  It will use skit_loaf_resize to handle an necessary 
enlarging of the loaf.  

The loaf will never be shrunk.  Instead, the returned value is a slice of the 
loaf corresponding to the text actually written.

This is similar to skit_loaf_assign_cstr, except that it will never perform
deallocating operations, and skit_loaf_assign_cstr might.  
skit_loaf_store_cstr is intended for situations where the given 'loaf' is
being used as a buffer for string data of unknown length.  It is expected
that the caller will store length information outside of the loaf (such as
by persisting the returned slice) so that it is possible to extract the
correct string information from the loaf being used as a buffer.

If the referenced loaf is null (ex: skit_loaf_is_null(*loaf) == 0), then the
loaf will be allocated using skit_loaf_alloc().

Assigning a NULL string to a non-null loaf will do nothing, and
skit_slice_null() will be returned.

Assigning a NULL string to a null loaf will do nothing, and skit_slice_null()
will be returned.

Example:
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
*/
skit_slice skit_loaf_store_cstr(skit_loaf *loaf, const char *cstr);

/**
Turns the given loaf into a carbon-copy of the given nul-terminated C string
while re-using the loaf's already allocated memory whenever possible.
The loaf will be allocated, deallocated, shrunk, or enlarged as needed to make
it fit the exact length, contents, and NULL status of the given C string.

This is similar to skit_loaf_store_cstr, except that it will do deallocating
operations, and skit_loaf_store_cstr won't.  skit_loaf_assign_cstr is intended
to be used in situations where 'loaf' is used as the sole representation for
the stored string data, including length information.

If the referenced loaf is null (ex: skit_loaf_is_null(*loaf) == 0), then the
loaf will be allocated using skit_loaf_alloc().

Assigning a NULL string to a non-null loaf will cause the loaf to be freed,
and skit_slice_null() will be returned.

Assigning a NULL string to a null loaf will do nothing, and skit_slice_null()
will be returned.

Example:
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
*/
skit_slice skit_loaf_assign_cstr(skit_loaf *loaf, const char *cstr);

/**
Replaces the contents of the given loaf with a copy of the text in the given
slice.  It will use skit_loaf_resize to handle an necessary enlarging of the
loaf.  

The loaf will never be shrunk.  Instead, the returned value is a slice of the 
loaf corresponding to the text actually written.

This is similar to skit_loaf_assign_slice, except that it will never perform
deallocating operations, and skit_loaf_assign_slice might.  
skit_loaf_store_slice is intended for situations where the given 'loaf' is
being used as a buffer for string data of unknown length.  It is expected
that the caller will store length information outside of the loaf (such as
by persisting the returned slice) so that it is possible to extract the
correct string information from the loaf being used as a buffer.

If the referenced loaf is null (ex: skit_loaf_is_null(*loaf) == 0), then the
loaf will be allocated using skit_loaf_alloc().

Assigning a null slice to a non-null loaf will do nothing, and
skit_slice_null() will be returned.

Assigning a null slice to a null loaf will do nothing, and skit_slice_null()
will be returned.

Example:
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
*/
skit_slice skit_loaf_store_slice(skit_loaf *loaf, skit_slice slice);

/**
Turns the given loaf into a carbon-copy of the given slice while re-using the
loaf's already allocated memory whenever possible.  The loaf will be allocated,
deallocated, shrunk, or enlarged as needed to make it fit the exact length,
contents, and NULL status of the given slice.

This is similar to skit_loaf_store_slice, except that it will do deallocating
operations, and skit_loaf_store_slice won't.  skit_loaf_assign_slice is 
intended to be used in situations where 'loaf' is used as the sole 
representation for the stored string data, including length information.

If the referenced loaf is null (ex: skit_loaf_is_null(*loaf) == 0), then the
loaf will be allocated using skit_loaf_alloc().

Assigning a null slice to a non-null loaf will cause the loaf to be freed,
and skit_slice_null() will be returned.

Assigning a null slice to a null loaf will do nothing, and skit_slice_null()
will be returned.

Example:
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
*/
skit_slice skit_loaf_assign_slice(skit_loaf *loaf, skit_slice slice);

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
	skit_slice slice0 = sSLICE("foobar");
	skit_slice slice1 = skit_slice_of(slice0, 3, 5);
	skit_slice slice2 = skit_slice_of(slice0, 3, SKIT_EOT);
	skit_slice slice3 = skit_slice_of(slice1, 0, -1);
	sASSERT_EQS(slice1, sSLICE("ba"));
	sASSERT_EQS(slice2, sSLICE("bar"));
	sASSERT_EQS(slice3, sSLICE("b"));
	skit_slice slice4 = skit_slice_of(slice2, 1, 2);
	sASSERT_EQS(slice4, sSLICE("a"));
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
If the loaf's memory was created using one of the skit_loaf_* functions
'loaf' will be reinitialized to have a NULL value for sLPTR(loaf) and
zero for sLLENGTH(loaf), which is what skit_loaf_null() would return.

"User allocated" memory is memory given to a loaf using skit_loaf_emplace or
allocated using SKIT_LOAF_ON_STACK.
If the original memory for the loaf was user allocated, then skit_free will 
NOT be called on the original memory.  It will be the caller's responsibility
to clean up that memory separately, if it is even required.  In this case the
loaf will still be assigned a value like that returned from skit_loaf_null().

It is still important for user allocated loaves to have skit_free called on
them at the end of their lifetime.  This is because loaves may have
skit_loaf_resize called on them in the meantime, either by the caller or by
another loaf manipulation function.  In this case the call to skit_free will
selectively free any auxiliary memory that was allocated to enlarge the loaf.

Protip:
If the loaf allocated will only live within the span of a function call's
lifetime (and won't be stored for later use), then use 
sSCOPE_EXIT(skit_loaf_free(&loaf));
directly after the point where a loaf is allocated.  This will guarantee that
any memory resources allocated to the loaf are released, regardless of what 
happens later in the function, including thrown exceptions.

Example:

	skit_loaf loaf = skit_loaf_alloc(10);
	sASSERT(sLPTR(loaf) != NULL);
	skit_loaf_free(&loaf);
	sASSERT(sLPTR(loaf) == NULL);
	sASSERT(sLLENGTH(loaf) == 0);
	
	SKIT_LOAF_ON_STACK(stack_loaf, 64);
	sASSERT(sLPTR(stack_loaf) != NULL);
	sASSERT(sLLENGTH(stack_loaf) == 64);
	skit_loaf_resize(&stack_loaf, 128);
	sASSERT(sLPTR(stack_loaf) != NULL);
	sASSERT(sLLENGTH(stack_loaf) == 128);
	skit_loaf_free(&stack_loaf);
	sASSERT(sLPTR(loaf) == NULL);
	sASSERT(sLLENGTH(loaf) == 0);
	
*/
skit_loaf skit_loaf_free(skit_loaf *loaf);

/**
Returns a standard C printf format specifier that can be used with the character
data in a slice given by sSPTR(slice).  This is done because slices are not
necessarily nul-terminated, and must have length information provided in the
format specifier.

This can be tedious to use, so it is mostly intended to help with implementing
other functions responsible for placing slices into formatted strings 
(ex: sASSERT_EQS).

Only use this in places where a single formatter for a single argument is
required.  Otherwise, use the format specifier %.*s like so:
skit_slice slice = sSLICE("foo");
printf("%.*s", sSLENGTH(slice), sSPTR(slice));

Example:
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
	skit_loaf_free(&loaf);
*/
char *skit_slice_get_printf_formatter( skit_slice slice, char *buffer, int buf_size, int enquote );

/* Internal use.  Please do not call directly. */
#define SKIT__CHECK_SLICE_CMP_ESC( cmp_op, check_name, lhs, rhs, lhs_evaluated, rhs_evaluated, RESPONSE ) \
	do { \
		SKIT_LOAF_ON_STACK(skit_assert__lhs_buf, 32); \
		SKIT_LOAF_ON_STACK(skit_assert__rhs_buf, 32); \
		skit_slice skit_assert__lhs_escape = skit_slice_escapify((lhs_evaluated), &skit_assert__lhs_buf); \
		skit_slice skit_assert__rhs_escape = skit_slice_escapify((rhs_evaluated), &skit_assert__rhs_buf); \
		\
		/* TODO: Possible memory leak if either buffer grows past stack allocation */ \
		/* TODO:   and the caller-supplied response doesn't exit the program. */ \
		RESPONSE( cmp_op, check_name, lhs, rhs, \
			sSPTR(skit_assert__lhs_escape),    sSPTR(skit_assert__rhs_escape), \
			sSLENGTH(skit_assert__lhs_escape), sSLENGTH(skit_assert__rhs_escape)); \
	} while(0)

/* Internal use.  Please do not call directly. */
#define SKIT__CHECK_SLICE_CMP_EVAL( cmp_op, case_sensitive, check_name, upcased_suffix, lhs, rhs, RESPONSE ) \
	do { \
		skit_slice skit__check_lhs_evaluated = (lhs); \
		skit_slice skit__check_rhs_evaluated = (rhs); \
		\
		if ( !(skit_slice_ascii_ccmp(skit__check_lhs_evaluated, skit__check_rhs_evaluated, case_sensitive) cmp_op 0) ) \
			SKIT__CHECK_SLICE_CMP_ESC( cmp_op, check_name "_" #upcased_suffix, lhs, rhs, \
				skit__check_lhs_evaluated, skit__check_rhs_evaluated, RESPONSE ); \
	} while(0)

/* Internal use.  Please do not call directly. */
#define SKIT__ENFORCE_SLICE_RE(cmp_op, case_sensitive, upcased_suffix, lhs, rhs, RESPONSE) \
	SKIT__CHECK_SLICE_CMP_EVAL(cmp_op, case_sensitive, "ENFORCE", upcased_suffix, lhs, rhs, RESPONSE)

#define SKIT__ASSERT_SLICE_RE(cmp_op, case_sensitive, upcased_suffix, lhs, rhs, RESPONSE) \
	SKIT__CHECK_SLICE_CMP_EVAL(cmp_op, case_sensitive, "ASSERT", upcased_suffix, lhs, rhs, RESPONSE)

/**
Assertions/enforcement with caller-defined responses to test failure.

This exists to allow other libraries or utility code to build their own
assertions or enforcements ontop of the survival kit string.h asserts/enforces.

Appropriate responses are a macro with a signature like so:
#define RESPONSE(cmp_op, check_name, lhs, rhs, lhs_strptr, rhs_strptr, lhs_len, rhs_len)
cmp_op:     Usually expands to one of ==, !=, >=, <=, >, or <.  This will be a raw C token (unquoted).
check_name: Usually expands to "ASSERT_EQS", "ENFORCE_EQS", or similar, depending on origins.
lhs:        The original left-hand-side expression.  It is provided for stringizing; do not evaluate it.
rhs:        The original right-hand-side expression.  It is provided for stringizing; do not evaluate it.
lhs_strptr: A pointer derived from a slice whose value is the result of evaluating the left-hand-side of the original assert/enforce.
rhs_strptr: A pointer derived from a slice whose value is the result of evaluating the right-hand-side of the original assert/enforce.
lhs_len:    The length of the string (in bytes) pointed to by lhs_strptr.
rhs_len:    The length of the string (in bytes) pointed to by rhs_strptr.

The RESPONSE macro should expand as a statement, so wrap it in a 'do {...} while(0)' as necessary.
*/
#define SKIT_ENFORCE_EQS_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(==, 1, EQS,lhs,rhs,RESPONSE)
#define SKIT_ENFORCE_NES_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(!=, 1, NES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_GES_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(>=, 1, GES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_LES_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(<=, 1, LES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_GTS_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(>,  1, GTS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_LTS_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(<,  1, LTS,lhs,rhs,RESPONSE) /// Ditto.

#define SKIT_ENFORCE_IEQS_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(==, 0, EQS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_INES_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(!=, 0, NES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_IGES_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(>=, 0, GES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_ILES_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(<=, 0, LES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_IGTS_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(>,  0, GTS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ENFORCE_ILTS_RE(lhs,rhs,RESPONSE) SKIT__ENFORCE_SLICE_RE(<,  0, LTS,lhs,rhs,RESPONSE) /// Ditto.

#define SKIT_ASSERT_EQS_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(==, 1, EQS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_NES_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(!=, 1, NES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_GES_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(>=, 1, GES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_LES_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(<=, 1, LES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_GTS_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(>,  1, GTS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_LTS_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(<,  1, LTS,lhs,rhs,RESPONSE) /// Ditto.

#define SKIT_ASSERT_IEQS_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(==, 0, EQS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_INES_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(!=, 0, NES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_IGES_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(>=, 0, GES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_ILES_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(<=, 0, LES,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_IGTS_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(>,  0, GTS,lhs,rhs,RESPONSE) /// Ditto.
#define SKIT_ASSERT_ILTS_RE(lhs,rhs,RESPONSE) SKIT__ASSERT_SLICE_RE(<,  0, LTS,lhs,rhs,RESPONSE) /// Ditto.

/**
Assertions/enforcement involving comparisons of slices.
These are good to use because they will print the slices involved in the
comparison if anything goes wrong, thus aiding in fast debugging.
*/
#define sENFORCE_EQS(lhs,rhs) SKIT_ENFORCE_EQS_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE)
#define sENFORCE_NES(lhs,rhs) SKIT_ENFORCE_NES_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_GES(lhs,rhs) SKIT_ENFORCE_GES_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_LES(lhs,rhs) SKIT_ENFORCE_LES_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_GTS(lhs,rhs) SKIT_ENFORCE_GTS_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_LTS(lhs,rhs) SKIT_ENFORCE_LTS_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.

#define sENFORCE_IEQS(lhs,rhs) SKIT_ENFORCE_IEQS_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_INES(lhs,rhs) SKIT_ENFORCE_INES_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_IGES(lhs,rhs) SKIT_ENFORCE_IGES_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_ILES(lhs,rhs) SKIT_ENFORCE_ILES_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_IGTS(lhs,rhs) SKIT_ENFORCE_IGTS_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.
#define sENFORCE_ILTS(lhs,rhs) SKIT_ENFORCE_ILTS_RE(lhs,rhs,SKIT__ENFORCE_CMP_RESPONSE) /// Ditto.

#define sASSERT_EQS(lhs,rhs) SKIT_ENFORCE_EQS_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_NES(lhs,rhs) SKIT_ENFORCE_NES_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_GES(lhs,rhs) SKIT_ENFORCE_GES_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_LES(lhs,rhs) SKIT_ENFORCE_LES_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_GTS(lhs,rhs) SKIT_ENFORCE_GTS_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_LTS(lhs,rhs) SKIT_ENFORCE_LTS_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.

#define sASSERT_IEQS(lhs,rhs) SKIT_ENFORCE_IEQS_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_INES(lhs,rhs) SKIT_ENFORCE_INES_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_IGES(lhs,rhs) SKIT_ENFORCE_IGES_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_ILES(lhs,rhs) SKIT_ENFORCE_ILES_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_IGTS(lhs,rhs) SKIT_ENFORCE_IGTS_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.
#define sASSERT_ILTS(lhs,rhs) SKIT_ENFORCE_ILTS_RE(lhs,rhs,SKIT__ASSERT_CMP_RESPONSE) /// Ditto.

/* ------------------------- string misc functions ------------------------- */

/**
Converts a slice into a signed 64-bit integer.
Currently the semantics are defined largely by the underlying strtol
implementation, though that will probably change in the future.
*/
int64_t skit_slice_to_int(skit_slice int_str);

/**
Converts a slice into an unsigned 64-bit integer.
Currently the semantics are defined largely by the underlying strtoul
implementation, though that will probably change in the future.
*/
uint64_t skit_slice_to_uint(skit_slice uint_str);

/**
Returns true (nonzero) if the given character is alphabetic.
BUG: this currently only covers ascii characters.  UTF-8 is currently unimplemented.
*/
#define skit_is_alpha_lower(c) ('a' <= (c) && (c) <= 'z')
#define skit_is_alpha_upper(c) ('A' <= (c) && (c) <= 'Z')
#define skit_is_alpha(c) (skit_is_alpha_upper(c) || skit_is_alpha_lower(c))

/**
Returns true (nonzero) if the given character is a digit (0-9).
BUG: this currently only covers ascii characters.  UTF-8 is currently unimplemented.
*/
#define skit_is_digit(c) ('0' <= (c) && (c) <= '9')

/**
Returns true (nonzero) if the given character is a alphabetic or a digit (0-9).
BUG: this currently only covers ascii characters.  UTF-8 is currently unimplemented.
*/
#define skit_is_alphanumeric(c) (skit_is_alpha(c) || skit_is_digit(c))

/**
Returns true (nonzero) if the given character is whitespace (space, tab, '\r', or '\n').
BUG: this currently only covers ascii characters.  UTF-8 is currently unimplemented.
*/
#define skit_is_whitespace(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == '\v' )

/**
Lowercases the given character.
This assumes that the character is in the ASCII range.
*/
skit_utf8c skit_char_ascii_to_lower(skit_utf8c c);

/**
Uppercases the given character.
This assumes that the character is in the ASCII range.
*/
skit_utf8c skit_char_ascii_to_upper(skit_utf8c c);

/**
Lowercases the given slice in-place.
This assumes that the slice consists of ASCII characters.
*/
void skit_slice_ascii_to_lower(skit_slice *slice);

/**
Uppercases the given slice in-place.
This assumes that the slice consists of ASCII characters.
*/
void skit_slice_ascii_to_upper(skit_slice *slice);

/**
Lowercases the given slice in-place.
BUG: this currently only covers ascii characters.  UTF-8 is currently unimplemented.
*/
void skit_slice_to_lower(skit_slice *slice);

/**
Uppercases the given slice in-place.
BUG: this currently only covers ascii characters.  UTF-8 is currently unimplemented.
*/
void skit_slice_to_upper(skit_slice *slice);

/**
Compares two characters.
*/
int skit_char_ascii_cmp(skit_utf8c c1, skit_utf8c c2);
int skit_char_ascii_icmp(skit_utf8c c1, skit_utf8c c2);
int skit_char_ascii_ccmp(skit_utf8c c1, skit_utf8c c2, int case_sensitive);

/**
Gets the common prefix of the two strings.
This will return a slice of 'str1' whose contents are the common prefix.
If there is no common prefix between the two strings, then a zero-length slice
pointing at the beginning of str1 will be returned.

Bug: These currently use ascii comparisons.  UTF-8 is currently unimplemented.

Example:
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
*/
skit_slice skit_slice_common_prefix(const skit_slice str1, const skit_slice str2);
skit_slice skit_slice_common_iprefix(const skit_slice str1, const skit_slice str2);
skit_slice skit_slice_common_cprefix(const skit_slice str1, const skit_slice str2, int case_sensitive);

/**
Performs an either alphabetical or asciibetical comparison of the two ascii
strings.

+-------+--------------------------------------+
| Value |  Relationship between str1 and str2  |
+-------+--------------------------------------+
|  < 0  | str1 is less than str2               |
|   0   | str1 is equal to str2                |
|  > 0  | str1 is greater than str2            |
+-------+--------------------------------------+

As a special case, null strings (slices/loaves with sSPTR(*) == NULL) will always
be equal to each other and not equal to non-null strings.
null strings will always be less than non-null strings.

Example:
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
*/
int skit_slice_ascii_cmp(const skit_slice str1, const skit_slice str2);
int skit_slice_ascii_icmp(const skit_slice str1, const skit_slice str2);
int skit_slice_ascii_ccmp(const skit_slice str1, const skit_slice str2, int case_sensitive);

/**
Convenient string comparison functions.

Bug: These currently use ascii comparisons.  UTF-8 is currently unimplemented.

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
	
	skit_slice nullstr = skit_slice_null();
	sASSERT( skit_slice_eqs(nullstr,nullstr));
	sASSERT(!skit_slice_nes(nullstr,nullstr));
	sASSERT(!skit_slice_eqs(alphaLo,nullstr));
	sASSERT( skit_slice_nes(alphaLo,nullstr));
	sASSERT( skit_slice_lts(nullstr,alphaLo));
*/
int skit_slice_ges(const skit_slice str1, const skit_slice str2);
int skit_slice_gts(const skit_slice str1, const skit_slice str2);
int skit_slice_les(const skit_slice str1, const skit_slice str2);
int skit_slice_lts(const skit_slice str1, const skit_slice str2);
int skit_slice_eqs(const skit_slice str1, const skit_slice str2);
int skit_slice_nes(const skit_slice str1, const skit_slice str2);

/**
Case-insensitive versions of the other asciibetical comparison functions.
*/
int skit_slice_iges(const skit_slice str1, const skit_slice str2);
int skit_slice_igts(const skit_slice str1, const skit_slice str2);
int skit_slice_iles(const skit_slice str1, const skit_slice str2);
int skit_slice_ilts(const skit_slice str1, const skit_slice str2);
int skit_slice_ieqs(const skit_slice str1, const skit_slice str2);
int skit_slice_ines(const skit_slice str1, const skit_slice str2);

/**
Versions of slice comparison with dynamically determined case sensitivity.
*/
int skit_slice_cges(const skit_slice str1, const skit_slice str2, int case_sensitive);
int skit_slice_cgts(const skit_slice str1, const skit_slice str2, int case_sensitive);
int skit_slice_cles(const skit_slice str1, const skit_slice str2, int case_sensitive);
int skit_slice_clts(const skit_slice str1, const skit_slice str2, int case_sensitive);
int skit_slice_ceqs(const skit_slice str1, const skit_slice str2, int case_sensitive);
int skit_slice_cnes(const skit_slice str1, const skit_slice str2, int case_sensitive);


/** 
Trims any character in 'char_class' from 'slice'.
Always returns a slice. 
Example:
	skit_loaf loaf = skit_loaf_copy_cstr("xxfooabc");
	skit_slice slice0 = loaf.as_slice;
	skit_slice slice1 = skit_slice_ltrimx(slice0, sSLICE("x"));
	skit_slice slice2 = skit_slice_rtrimx(slice0, sSLICE("c"));
	skit_slice slice3 = skit_slice_trimx (slice0, sSLICE("xabc"));
	sASSERT_EQS( slice1, sSLICE("fooabc") );
	sASSERT_EQS( slice2, sSLICE("xxfooab") );
	sASSERT_EQS( slice3, sSLICE("foo") );
	skit_loaf_free(&loaf);
*/
skit_slice skit_slice_ltrimx(const skit_slice slice, const skit_slice char_class);
skit_slice skit_slice_rtrimx(const skit_slice slice, const skit_slice char_class);
skit_slice skit_slice_trimx(const skit_slice slice, const skit_slice char_class);

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

/**
Determines if the slice 'needle' equals the contents of the slice 'haystack'
at the position 'pos'.  It will only compare as many characters as there are
in 'needle'.
This is similar to calling 
skit_slice_eqs(skit_slice_of(haystack,pos,pos+sSLENGTH(needle)),needle)
with the exception that 'needle' is allowed to overlap the end of the 
'haystack' string (it will return 0 in such cases).
'pos' must be positive.  An assertion will trigger if it is not.
Returns 1 if 'needle' is equal to the contents of 'haystack' at 'pos'.
Returns 0 otherwise.
This function does not throw.  
It may assert if any of the slices contain NULL pointers, or if 'pos' does
not point to a location in the haystack slice.
Example:
	skit_slice haystack = sSLICE("foobarbaz");
	skit_slice needle = sSLICE("bar");
	sASSERT_EQ(skit_slice_match(haystack,needle,0),0);
	sASSERT_EQ(skit_slice_match(haystack,needle,3),1);
	sASSERT_EQ(skit_slice_match(haystack,needle,6),0);
	sASSERT_EQ(skit_slice_match(haystack,needle,8),0);
*/
int skit_slice_match(
	const skit_slice haystack,
	const skit_slice needle,
	ssize_t pos);

/**
Determines if there is a newline sequence at the given position 'pos' in the
'text'.
This will attempt to match any of the conventional newline sequences, in this
order:
"\r\n"
"\n"
"\r"
Returns 2 if "\r\n" matches.
Returns 1 if "\n" or "\r" matches.
Returns 0 if not.
Example:
	skit_slice haystack = sSLICE("foo\nbar\r\nbaz\rqux");
	sASSERT_EQ(skit_slice_match_nl(haystack,3) ,1);
	sASSERT_EQ(skit_slice_match_nl(haystack,7) ,2);
	sASSERT_EQ(skit_slice_match_nl(haystack,8) ,1);
	sASSERT_EQ(skit_slice_match_nl(haystack,12),1);
	sASSERT_EQ(skit_slice_match_nl(haystack,0) ,0);
	sASSERT_EQ(skit_slice_match_nl(haystack,2) ,0);
	sASSERT_EQ(skit_slice_match_nl(haystack,4) ,0);
	sASSERT_EQ(skit_slice_match_nl(haystack,13),0);
*/
int skit_slice_match_nl(
	const skit_slice text,
	ssize_t pos);

/// Finds the first occurrence of 'needle' in 'haystack'.
/// If 'output_pos' is non-NULL, it will be filled with 'needle's position
/// within the 'haystack', or -1 if it was not found.
///
/// Preconditions:
/// 'haystack' must be non-null.
/// 'needle' must be non-null.
///
/// Returns 1 if it was found, or 0 if it was not.
/// Example:
///   ssize_t pos = 0;
///   sASSERT(!skit_slice_find(sSLICE("foo"),sSLICE("x"),&pos));
///   sASSERT_EQ(pos,-1);
///   sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("o"),&pos));
///   sASSERT_EQ(pos,1);
///   sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("f"),&pos));
///   sASSERT_EQ(pos,0);
///   sASSERT(!skit_slice_find(sSLICE("foo"),sSLICE("x"),NULL));
///   sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("f"),NULL));
///   sASSERT(skit_slice_find(sSLICE("foo"),sSLICE("o"),NULL));
int skit_slice_find(
	const skit_slice haystack,
	const skit_slice needle,
	ssize_t *output_pos);

/// Finds the first occurrence of 'delimiter' in 'text' and splits it into
/// left (head) and right (tail) slices.
///
/// If either 'head' or 'tail' is a NULL pointer, then that argument will not
/// be populated.  Calling skit_slice_partition(a,b,NULL,NULL) is equivalent
/// to calling skit_slice_find(a,b,NULL): they both determine if the string
/// 'b' exists in 'a' and do nothing else.
///
/// Preconditions:
/// 'text' must be non-null.
/// 'delimiter' must be non-null.
///
/// Returns: 1 if 'delimiter' was found, 0 if it was not.
/// Example:
///   skit_slice text = sSLICE("Hello world!");
///   skit_slice head = skit_slice_null();
///   skit_slice tail = skit_slice_null();
///   skit_slice delim = sSLICE(" ");
///
///   sASSERT(skit_slice_partition(text, delim, &head, &tail));
///   sASSERT_EQS(head, sSLICE("Hello"));
///   sASSERT_EQS(tail, sSLICE("world!"));
///
///   tail = skit_slice_null();
///   sASSERT(skit_slice_partition(text, delim, &head, NULL));
///   sASSERT_EQS(head, sSLICE("Hello") );
///   sASSERT( skit_slice_is_null(tail) );
///
///   head = skit_slice_null();
///   sASSERT(skit_slice_partition(text, delim, NULL, &tail));
///   sASSERT( skit_slice_is_null(head) );
///   sASSERT_EQS(tail, sSLICE("world!"));
///
///   text = sSLICE("Hello");
///   sASSERT(!skit_slice_partition(text, delim, &head, &tail));
///   sASSERT_EQS(head, sSLICE("Hello"));
///   sASSERT_EQS(tail, sSLICE(""));
///
///   text = sSLICE("");
///   sASSERT(!skit_slice_partition(text, delim, &head, &tail));
///   sASSERT_EQS(head, sSLICE(""));
///   sASSERT_EQS(tail, sSLICE(""));
///
int skit_slice_partition(
	const skit_slice text,
	const skit_slice delimiter,
	skit_slice *head,
	skit_slice *tail);

/// A less-powerful but (sometimes) more convenient derivative of
/// skit_slice_partition.  It is designed to be easier to use in while-loops.
/// This will split 'text' at the first occurrence of 'delimiter', and return
/// the left part of the split.  'text' will be modified to contain only the
/// text that directly follows the found delimiter.
/// If 'delimiter' is not found in 'text', then the entire value of 'text' is
/// returned and 'text' is modified to contain an empty string (essentially,
/// skit_slice_of(text, SKIT_EOT, SKIT_EOT)).
///
/// If 'head' is NULL (doesn't point to a slice), then this argument simply
/// won't be populated.  The slice pointed to by 'text' will still be modified
/// as if a non-NULL 'head' reference was given.
///
/// Preconditions:
/// 'text' must be non-NULL.
/// '*text' must be non-null.
/// 'delimiter' must be non-null.
///
/// Returns: 1 if 'text' was a non-empty slice, 0 if 'text' was an empty slice.
///
/// Example:
///   skit_slice text = sSLICE("a b c");
///   skit_slice head = skit_slice_null();
///   skit_slice delim = sSLICE(" ");
///
///   sASSERT(skit_slice_take_head(&text,delim,&head));
///   sASSERT_EQS(head,sSLICE("a"));
///   sASSERT_EQS(text,sSLICE("b c"));
///
///   sASSERT(skit_slice_take_head(&text,delim,&head));
///   sASSERT_EQS(head,sSLICE("b"));
///   sASSERT_EQS(text,sSLICE("c"));
///
///   sASSERT(skit_slice_take_head(&text,delim,&head));
///   sASSERT_EQS(head,sSLICE("c"));
///   sASSERT_EQS(text,sSLICE(""));
///
///   sASSERT(!skit_slice_take_head(&text,delim,&head));
///   sASSERT_EQS(head,sSLICE(""));
///   sASSERT_EQS(text,sSLICE(""));
///
///   text = sSLICE("a b c");
///   int count = 0;
///   while ( skit_slice_take_head(&text,delim,NULL) )
///       count++;
///   sASSERT_EQ(count, 3);
///
int skit_slice_take_head(
	skit_slice *text,
	const skit_slice delimiter,
	skit_slice *head);

/**
Applies C-style escaping to the given string.  This allows non-printable
characters to be passed into places like terminal output and displayed.

'buffer' will be grown as needed to contain the resulting string.
*/
skit_slice skit_slice_escapify(skit_slice str, skit_loaf *buffer);

/* Unittests all string functions. */
void skit_string_unittest();

#endif
