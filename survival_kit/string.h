#ifndef SKIT_STRING_INCLUDED
#define SKIT_STRING_INCLUDED

#include <inttypes.h> /* uint8_t, uint16_t, uint32_t */
#include <unistd.h> /* For ssize_t */

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
Dynamically allocates a loaf with the same length as the given literal and
then copies the literal into the loaf.
The allocation will be performed with skit_malloc.
A nul character will be placed loaf.chars[strlen(literal)].
*/
skit_loaf skit_loaf_copy_lit(const char *literal);

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
*/
int skit_string_check_init(skit_string str);
int skit_loaf_check_init (skit_loaf loaf);
int skit_slice_check_init(skit_slice slice);

/**
Returns 1 if the given 'str' is actually a loaf.
Returns 0 otherwise.
*/
int skit_string_is_loaf(skit_string str);

/**
Returns 1 if the given 'str' is actually a slice.
Returns 0 otherwise.
*/
int skit_string_is_slice(skit_string str);

/** 
Does a checked cast of the given string into a loaf.
This is NOT a conversion.  It just reinterprets the bytes of the given string.
If the original string is a slice, then the safe way of preparing it for
use in operations requiring a loaf is to call skit_string_dup(slice.as_string)
on the slice to obtain a loaf, and then free the loaf when done using
skit_loaf_free(loaf).
*/
skit_loaf skit_string_as_loaf(skit_string str);

/** 
Does a checked cast of the given string into a slice.
This is NOT a conversion.  It just reinterprets the bytes of the given string.
*/
skit_slice skit_string_as_slice(skit_string str);

/**
Resizes the given 'loaf' to the given 'length'.
This will call skit_realloc to adjust the underlying memory size.
'loaf' will be altered in-place, and the result will also be returned.
*/
skit_loaf *skit_loaf_resize(skit_loaf *loaf, size_t length);


/**
Appends 'str2' onto the end of 'loaf1'.
The additional memory needed is created using realloc. 
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
*/
skit_loaf skit_string_join(skit_string str1, skit_string str2);

/**
Duplicates the given string 'str'.
This is the recommended way to obtain a loaf when given a slice.
This function allocates memory, so be sure to clean up as needed.
*/
skit_loaf skit_string_dup(skit_string str);

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
side of the array:
  skit_string str = skit_string_copy_lit("foobar");
  skit_slice slice = skit_slice_of(str, 3, -1);
  char *cstr = skit_slice_dup_as_cstr(slice);
  sASSERT_EQS("bar", cstr);
  skit_free(cstr);
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
*/
skit_loaf *skit_loaf_free(skit_loaf *loaf);

/* ------------------------- string misc functions ------------------------- */

/** 
Trim whitespace from 'str'.
Always returns a slice. 
*/
skit_slice str_ltrim(const skit_string str);
skit_slice str_rtrim(const skit_string str);
skit_slice str_trim(const skit_string str);

/**
Truncate 'nchars' from the left or right side of 'str'.
Always returns a slice. 
*/
skit_slice str_ltruncate(const skit_string str, size_t nchars);
skit_slice str_rtruncate(const skit_string str, size_t nchars);

#endif