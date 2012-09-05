/**
Template flist: a free-list implementation.

Free lists are a great way to accelerate access to a frequently allocated and 
discarded type. The idea is simple - instead of deallocating an object when 
done with it, put it on a free list. When allocating, pull one off the free 
list first. 

Instances of this template require instances of survival_kit/templates/slist.h
to be created with the same SKIT_T_PREFIX and SKIT_T_ELEM_TYPE.

It is the caller's responsibility to #include "survival_kit/templates/slist.h"
  before #include'ing this file.

It is the caller's responsibility to #include "survival_kit/templates/slist.c"
  in only one .c file being linked with the main program.  

It is the caller's responsibility to undefine template parameters after 
#include'ing this file.

EXAMPLE:
-------------------------------------------------------------------------------
// This is a way to stack-allocate memory for use with setjmp/longjmp such
//   that none of the elements in the flist need to be declared anywhere.
// (This objective may seem unusual, but it is useful for writing macros
//    where having variable declarations in certain places is undesirable.)
#include "survival_kit/setjmp/jmp_flist.h"
#include <setjmp.h>
#include <assert.h>

int end_was_reached = 0;
skit_jmp_flist list;
skit_jmp_flist_init(&list);
if (skit_jmp_flist_full(&list))
	skit_jmp_flist_grow(&list,alloca(sizeof(skit_jmp_snode*)));
if( setjmp(*skit_jmp_flist_push(&list)) == 0 )
{
	assert(list.used.length == 1);
	longjmp(*pop_jmp,1)
	assert(0);
}
else
{
	assert(list.used.length == 0);
	end_was_reached = 1;
}
assert(end_was_reached);
-------------------------------------------------------------------------------


See "survival_kit/flist_builtins.h" for slist instantiations of common C builtin
  types, as well as unit tests.

Parameters:

SKIT_T_ELEM_TYPE (required) -
	The type for elements in this list, ex: int, float.

SKIT_T_PREFIX (required) -
	A unique name that is included in all type/function expansions created by
	this template.  It works like so:
	
	EXAMPLE:
	#define SKIT_T_ELEM_TYPE int
	#define SKIT_T_PREFIX int
	#include "survival_kit/templates/flist.h"
	#undefine SKIT_T_ELEM_TYPE
	#undefine SKIT_T_PREFIX
	skit_int_flist  list;        // The type 'skit_int_flist' is defined.
	skit_int_flist_init(&list);        // The function 'skit_int_flist_init' is defined.

SKIT_T_FUNC_ATTR (optional, defaualts to nothing) - 
	Specifies function attributes to be used on all functions created.
	This is mostly present to allow functions to be instanced as 'static' so
	  that they will not cause linking collisions with instances found in
	  different and unrelated files that happen to have the same SKIT_T_PREFIX.

SKIT_T_DIE_ON_ERROR (optional) - 
	Defining this makes it so that invalid operations cause skit_die(...) to be 
	called instead of raising an exception.
	This is esed by certain places like the features.h module where 
	exception throwing can't be used because the lists are being used to 
	implement the ability to throw exceptions.
*/

#include "survival_kit/templates/skit_t.h"

#ifndef SKIT_T_ELEM_TYPE
#error "SKIT_T_ELEM_TYPE is needed but was not defined."
#endif

typedef struct SKIT_T(flist) SKIT_T(flist);
struct SKIT_T(flist)
{
	SKIT_T(slist) unused;
	SKIT_T(slist) used;
};

/**
Initializes the freelist and any sublists.
Call this on a freelist before calling any of the other freelist functions.
*/
void SKIT_T(flist_init)( SKIT_T(flist) *list );

/**
This is a way to check if a free list has any free nodes left or not.
If this returns 1, then skit_*_flist_grow must be called and
  provided with memory before skit_*_flist_push is called.
Returns:
	1 if there are no free nodes in the list.
	0 if there are free nodes that can be used by skit_*_flist_push()
*/
int SKIT_T(flist_full)( SKIT_T(flist) *list );

/**
Assigns memory to the freelist.
'node' should be a pointer to an amount of newly allocated memory with
  enough size to fit a skit_*_snode value: sizeof(skit_mytype_snode).
*/
void SKIT_T(flist_grow)( SKIT_T(flist) *list, void *node );

/**
Currently not implemented.
This would be a way to reclaim memory from a freelist.
*/
void SKIT_T(flist_shrink)( SKIT_T(flist) *list, int num_nodes );

/**
Grabs an element from the freelist and pushes its node onto the 'used' list.
A pointer to the element's memory is returned.
The result of this operation can be passed to other routines that might
want to use this value: most commonly these are initialization routines.
*/
SKIT_T_ELEM_TYPE *SKIT_T(flist_push)( SKIT_T(flist) *list );

/**
Moves the last "pushed" element into the 'unused' list.
The return value is a pointer to the moved node's element.
The data pointed to may be used as long as no other freelist functions are
  called on the list during the pointer's usage.
*/
SKIT_T_ELEM_TYPE *SKIT_T(flist_pop)( SKIT_T(flist) *list );
