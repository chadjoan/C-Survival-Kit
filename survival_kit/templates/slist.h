/**
Template slist: a singly-linked list implementation.

It is the caller's responsibility to undefine template parameters after 
#include'ing this file.

See "survival_kit/slist_builtins.h" for slist instantiations of common C builtin
  types, as well as unit tests.

Parameters:

SKIT_T_ELEM_TYPE (required) -
	The type for elements in this list, ex: int, float.

SKIT_T_PREFIX (required) -
	A unique name that is included in all type/function expansions created by
	this template.  It works like so:
	
	EXAMPLE1:
	#define SKIT_T_ELEM_TYPE int
	#define SKIT_T_PREFIX int
	#include "survival_kit/templates/slist.h"
	#undefine SKIT_T_ELEM_TYPE
	#undefine SKIT_T_PREFIX
	skit_int_slist  list;        // The type 'skit_int_slist' is defined.
	skit_int_snode  node;        // The type 'skit_int_snode' is defined.
	node.val = 42;
	skit_int_slist_init(&list);        // The function 'skit_int_slist_init' is defined.
	skit_int_slist_push(&list,&node);  // The function 'skit_int_slist_push' is defined.
	
	EXAMPLE2:
	#define SKIT_T_ELEM_TYPE my_type_with_a_really_long_name
	#define SKIT_T_PREFIX my_type
	#include "survival_kit/templates/slist.h"
	#undefine SKIT_T_ELEM_TYPE
	#undefine SKIT_T_PREFIX
	skit_my_type_slist  list;        // The type 'skit_my_type_slist' is defined.
	skit_my_type_snode  node;        // The type 'skit_my_type_snode' is defined.
	my_type_with_a_really_long_name foo = create_my_type(...);
	node.val = foo;
	skit_my_type_slist_init(&list);        // The function 'skit_my_type_slist_init' is defined.
	skit_my_type_slist_push(&list,&node);  // The function 'skit_my_type_slist_push' is defined.

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

#include <unistd.h> /* for ssize_t */

typedef struct SKIT_T(snode) SKIT_T(snode);
struct SKIT_T(snode)
{
	SKIT_T_ELEM_TYPE val;
	SKIT_T(snode) *next;
};

typedef struct SKIT_T(slist) SKIT_T(slist);
struct SKIT_T(slist)
{
	SKIT_T(snode) *front;
	SKIT_T(snode) *back;
	ssize_t length;
};

/**
Initializes a list object as a zero-length list. O(1) operation. 
This is an in-place operation.  The result is provided as a return value so
  that this function may appear within expressions.
Returns: The list object that was passed in, initialized to an empty list.
*/
SKIT_T(slist) *SKIT_T(slist_init)(SKIT_T(slist) *list);

/** Pushes a snode onto the front of the list. O(1) operation. */
void SKIT_T(slist_push)(SKIT_T(slist) *list, SKIT_T(snode) *node);

/** Pops a snode from the front of the list. O(1) operation. */
SKIT_T(snode) *SKIT_T(slist_pop)(SKIT_T(slist) *list);
