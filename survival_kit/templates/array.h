
/**
Template array: defines a templated array type.

This array falls into "loaf" and "slice" designations just like the string
module.  Most function calls in this module will just adjust indices for
correct stride and then call skit_loaf and skit_slice functions.

The advantage of this is that array access can be made typesafe: it will not
be possible to, for example, place a 64 bit integer into an array of 32 bit 
integers, as would be with conventional C array implementations that require 
the caller to cast between the element types and void* pointers.  It should
also make calling code look a lot cleaner by removing macro invocations and
casts.

All "length" values given in the array API specify the count of elements, not
the number of bytes.  

It is the caller's responsibility to undefine template parameters after 
#include'ing this file.

See "survival_kit/array_builtins.h" for array instantiations of common C builtin
  types, as well as unit tests.

Parameters:

SKIT_T_ELEM_TYPE (required) -
	The type for elements in this list, ex: int, float.

SKIT_T_PREFIX (required) -
	A unique name that is included in all type/function expansions created by
	this template.  This can be about 9 characters long. Any longer and the
	symbol names generated by the template could be too long for the OpenVMS
	linker to handle.
	It works like so:
	
	EXAMPLE1:
	
	#define SKIT_T_ELEM_TYPE int
	#define SKIT_T_PREFIX int
	#include "survival_kit/templates/stack.h"
	#undefine SKIT_T_ELEM_TYPE
	#undefine SKIT_T_PREFIX
	
	skit_int_loaf   arrl;   // The type 'skit_int_loaf' is defined.
	skit_int_slice  arrs;   // The type 'skit_int_slice' is defined.
	
	// These functions are now defined.
	arrl = skit_int_loaf_alloc(16);
	arrs = skit_int_slice_of(arrl.as_slice, 0, 8);
	
	EXAMPLE2:
	#define SKIT_T_ELEM_TYPE my_type_with_a_really_long_name
	#define SKIT_T_PREFIX my_type
	#include "survival_kit/templates/stack.h"
	#undefine SKIT_T_ELEM_TYPE
	#undefine SKIT_T_PREFIX
	
	skit_my_type_loaf   arrl;   // The type 'skit_my_type_loaf' is defined.
	skit_my_type_slice  arrs;   // The type 'skit_my_type_slice' is defined.
	my_type_with_a_really_long_name foo = create_my_type(...);
	
	// These functions are now defined.
	arrl = skit_my_type_loaf_alloc(16);
	skit_my_type_loaf_ptr(arrl)[0] = foo;
	
SKIT_T_FUNC_ATTR (optional, defaualts to nothing) - 
	Specifies function attributes to be used on all functions created.
	This is mostly present to allow functions to be instanced as 'static' so
	  that they will not cause linking collisions with instances found in
	  different and unrelated files that happen to have the same SKIT_T_PREFIX.

SKIT_T_DIE_ON_ERROR (optional) - 
	Defining this makes it so that invalid operations cause skit_die(...) to be 
	called instead of raising an exception.
	This is esed by certain places like the feature_emulation.h module where 
	exception throwing can't be used because the lists are being used to 
	implement the ability to throw exceptions.
*/

#include "survival_kit/templates/skit_t.h"

#ifndef SKIT_T_ELEM_TYPE
#error "SKIT_T_ELEM_TYPE is needed but was not defined."
#endif

extern const int SKIT_T(loaf_stride);

#ifndef SKIT_TEMPLATES_ARRAY_COMMON_DEFINED
#define SKIT_TEMPLATES_ARRAY_COMMON_DEFINED

/* Common definitions to all array instantiations: */
/* do not use SKIT_T(X) in this section, because this */
/* section only gets instantiated once. */

#include "survival_kit/string.h"

/* #define sLENGTH(arr) This seems hard to do concisely without a runtime stride.  There's no room for it. */

/**
This is analogous to SKIT_LOAF_ON_STACK from "survival_kit/string.h".
See that macro for specifics on usage and behavior.
*/
#define SKIT_ARRAY_ON_STACK(prefix, array_name, size) \
	const char *__SKIT_LOAF_##array_name [ \
		(size  * skit_##prefix##_loaf_stride) + SKIT_ARRAY_EMPLACEMENT_OVERHEAD]; \
	skit_##prefix##_loaf array_name = skit_loaf_emplace( \
		__SKIT_LOAF_##array_name, (size  * skit_##prefix##_loaf_stride) + SKIT_ARRAY_EMPLACEMENT_OVERHEAD);


#endif

/**
This is analogous to a skit_slice, except that operations on it will return
elements of type SKIT_T_ELEM_TYPE instead of individual characters.
*/
typedef union SKIT_T(slice) SKIT_T(slice);
union SKIT_T(slice)
{
	void         *data_handle;
	skit_slice   as_skit_slice;
};

/**
This is analogous to a skit_loaf, except that operations on it will return
elements of type SKIT_T_ELEM_TYPE instead of individual characters.
*/
typedef union SKIT_T(loaf) SKIT_T(loaf);
union SKIT_T(loaf)
{
	void          *data_handle;
	SKIT_T(slice) as_slice;
	skit_loaf     as_skit_loaf;
	skit_slice    as_skit_slice;
};


/* --------------------- fundamental array functions ---------------------- */

/*
A note about name lengths:
skit_<prefix>_slice_<name>
The <prefix> part is allowed to be 9 characters long.
This means the <name> part can be no longer than 10 characters without creating
symbols that are too long for the OpenVMS linker to handle.
The only exception is loaf-specific function names: those can be 11 characters
because "loaf" is shorter than "slice".
(OpenVMS symbols can't be longer than 31 characters.)
*/

/**
These are functions for creating initialized-but-null values for all of the
different types of arrays.
*/
SKIT_T(slice) SKIT_T(slice_null)();
SKIT_T(loaf)  SKIT_T(loaf_null)();

/**
Dynamically allocates a loaf of zero length using skit_malloc.
The allocated memory will contain a nul character and calling 
skit_<prefix>_loaf_len on the resulting loaf will return 0.
*/
SKIT_T(loaf) SKIT_T(loaf_new)();

/**
Dynamically allocates a skit_<prefix>_loaf with the same length as the given
C-style pointer-as-array, and then copies the C-style array into the loaf.
The allocation will be performed with skit_malloc.
*/
SKIT_T(loaf) SKIT_T(loaf_copy_carr)(const SKIT_T_ELEM_TYPE array[], size_t length);

/**
Dynamically allocates a skit_<prefix>_loaf of the given 'length' using 
skit_malloc.  The resulting array will be uninitialzed.
*/
SKIT_T(loaf) SKIT_T(loaf_alloc)(size_t length);

/**
The length of a loaf returned from skit_<prefix>_loaf_emplace will be smaller
than the original memory allocated by this much.  The overhead is used to do
things like allocate a handle that prevents resizes from invalidating all 
slices.
*/
#define SKIT_ARRAY_EMPLACEMENT_OVERHEAD SKIT_LOAF_EMPLACEMENT_OVERHEAD

/**
This is analogous to skit_loaf_emplace from "survival_kit/string.h".
See that function for specifics on usage and behavior.

The only difference is that it is possible to pass enough memory to allocate
only part of an element at the end of the array.  In this case, the partial
element at the end will not be accessible from array functions and will not
contribute to the array's length.
*/
SKIT_T(loaf) SKIT_T(loaf_emplace)( void *mem, size_t mem_size );

/**
These functions calculate the length of the given loaf/slice.
This is an O(1) operation because the length information is stored
in an array's 'meta' field.  It is safe to assume, however, that this 
operation is slightly more complicated than a simple variable access, so any
high-performance code should probably cache array lengths whenever it is
safe to do so.
*/
ssize_t SKIT_T(loaf_len) (SKIT_T(loaf) loaf);
ssize_t SKIT_T(slice_len)(SKIT_T(slice) slice); /** ditto */

/**
Returns the pointer to the array's data, thus allowing direct
manipulation of the array's individual elements.

Since any resize operation on a loaf can move the underlying memory for that
loaf, it is important to avoid doing that when working with a pointer returned
from this function.  The loaf could get updated, but the pointer won't, and
will then point to unallocated memory.  Alternatively, always refresh any
pointers to the loaf's data whenever a potentially resizing operation is
called.

Example:
	SKIT_T(loaf)  loaf = skit_loaf_copy_cstr("foobarbaz");
	SKIT_T(slice) slice = skit_slice_of(loaf.as_slice, 3, 6);
	sASSERT_EQ(sSPTR(slice) - sLPTR(loaf), 3, "%d");
	sASSERT_EQS(slice, sSLICE("bar"));
	skit_loaf_free(&loaf);
*/
SKIT_T_ELEM_TYPE *SKIT_T(loaf_ptr) ( SKIT_T(loaf)  loaf );
SKIT_T_ELEM_TYPE *SKIT_T(slice_ptr)( SKIT_T(slice) slice ); /** ditto */


/**
This is analogous to skit_loaf_is_null in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
int SKIT_T(loaf_is_null) (SKIT_T(loaf) loaf);
int SKIT_T(slice_is_null)(SKIT_T(slice) slice);

/** 
These are analogous to functions in "survival_kit/string.h".
See those functions for specifics on usage and behavior.
*/
int SKIT_T(loaf_check_init) (SKIT_T(loaf) loaf);
int SKIT_T(slice_check_init)(SKIT_T(slice) slice);

/**
Creates a slice of the given C-style pointer-as-array.
*/
SKIT_T(slice) SKIT_T(slice_of_carr)(const SKIT_T_ELEM_TYPE array[], int length );

/**
This is analogous to skit_loaf_resize in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(loaf) *SKIT_T(loaf_resize)(SKIT_T(loaf) *loaf, size_t length);


/**
This is analogous to skit_loaf_append in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(loaf) *SKIT_T(loaf_append)(SKIT_T(loaf) *loaf1, SKIT_T(slice) str2);

/**
This is similar to loaf_append, except that it appends only a single element.
*/
SKIT_T(loaf) *SKIT_T(loaf_put)(SKIT_T(loaf) *loaf1, SKIT_T_ELEM_TYPE elem);

/** 
This is analogous to skit_slice_concat in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(loaf) SKIT_T(slice_concat)(SKIT_T(slice) str1, SKIT_T(slice) str2);

/**
This is analogous to skit_slice_buffered_resize in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(slice) *SKIT_T(slice_bfd_resize)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	ssize_t    new_buf_slice_length);

/** 
This is analogous to skit_slice_buffered_append in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(slice) *SKIT_T(slice_bfd_append)(
	SKIT_T(loaf)  *buffer,
	SKIT_T(slice) *buf_slice,
	SKIT_T(slice) suffix);

/**
This is similar to slice_bfd_append, except that it appends only a single
element.
*/
SKIT_T(slice) *SKIT_T(slice_bfd_put)(
	SKIT_T(loaf)     *buffer,
	SKIT_T(slice)    *buf_slice,
	SKIT_T_ELEM_TYPE elem);

/**
This is analogous to skit_loaf_dup in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(loaf) SKIT_T(loaf_dup)(SKIT_T(slice) slice);

/**
This is analogous to skit_loaf_assign_cstr in "survival_kit/string.h".
See that function for specifics on usage and behavior.

(The name "xfer" is short for "transfer" and is used instead of "assign" as a
way to shorten the symbol name due to OpenVMS linker limitations.)
*/
SKIT_T(slice) SKIT_T(loaf_xfer_carr)(SKIT_T(loaf) *loaf, const SKIT_T_ELEM_TYPE array[], size_t length);

/**
This is analogous to skit_loaf_assign_slice in "survival_kit/string.h".
See that function for specifics on usage and behavior.

(The name "xfer" is short for "transfer" and is used instead of "assign" as a
way to shorten the symbol name due to OpenVMS linker limitations.)
*/
SKIT_T(slice) SKIT_T(loaf_xfer_slice)(SKIT_T(loaf) *loaf, SKIT_T(slice) slice);

/**
This is analogous to skit_slice_of in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(slice) SKIT_T(slice_of)(SKIT_T(slice) slice, ssize_t index1, ssize_t index2);

/** 
Returns the loaf's contents as a C-style pointer-as-array.  Since C pointers
usually require a length value to be passed around in tandem, the caller is
advised to use skit_<prefix>_loaf_len(loaf) to obtain the length value if
needed.
*/
SKIT_T_ELEM_TYPE *SKIT_T(loaf_as_carr)(SKIT_T(loaf) loaf);

/** 
Returns a copy of the slice's contents as a C-style pointer-as-array.
This operation calls skit_malloc to allocate the required memory.
The caller is responsible for free'ing the returned C array.
The returned array will have the same length as that returned from
skit_<prefix>_slice_len(slice).
*/
SKIT_T_ELEM_TYPE *SKIT_T(slice_dup_carr)(SKIT_T(slice) slice);

/**
This is analogous to skit_loaf_free in "survival_kit/string.h".
See that function for specifics on usage and behavior.
*/
SKIT_T(loaf) SKIT_T(loaf_free)(SKIT_T(loaf) *loaf);

/* ------------------------- array misc functions ------------------------- */

/**
Truncate 'nelems' from the left or right side of 'slice'.
Always returns a slice. 

This is analogous to skit_slice_*truncate in "survival_kit/string.h".
See those functions for specifics on usage and behavior.
*/
SKIT_T(slice) SKIT_T(slice_ltruncate)(const SKIT_T(slice) slice, size_t nelems);
SKIT_T(slice) SKIT_T(slice_rtruncate)(const SKIT_T(slice) slice, size_t nelems);

/**
Returns a pointer to the element at the given index in the slice.
If this appears in a loop, it may be preferable to use skit_t_slice_ptr to get
a pointer to the slice's data and then increment that to get the pointer
instead of calling skit_t_slice_index.  Both skit_t_slice_ptr and 
skit_t_slice_index have potentially more overhead than a mere dereference or
add, so calling them infrequently may be more performant.  Just be sure to
always recalculate these pointers any time the slice or it's parent loaf is
resized: such resizes invalidate pointers.
*/
SKIT_T_ELEM_TYPE *SKIT_T(slice_index)( const SKIT_T(slice) slice, size_t index );

/** ditto */
SKIT_T_ELEM_TYPE *SKIT_T(loaf_index) ( const SKIT_T(loaf) loaf, size_t index );

/*
TODO:
slice_find(slice,elem)
slice_get(slice,index)
slice_set(*slice,index,val)
slice_bsearch(slice,elem)
slice_qsort(*slice) (maybe sorting functions should be their own template?)
*/
