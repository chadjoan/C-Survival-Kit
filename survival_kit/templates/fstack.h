/**
Template fstack: a free-stack implementation.

This construct can be used to attain very fast allocation for entities that
are frequently allocated and discarded.  When new memory is needed, the 
freestack will first attempt to use a previously freed node.  If there are 
no free nodes, only then will it call skit_malloc and do the potentially 
time-consuming form of memory allocation.

This is similar to a free list, though it is limited in expressiveness.  It
cannot free arbitrary nodes in its list.  However, this limitation allows it
to ensure that the nodes remain ordered: that is, the first chunk of memory
given to the freestack will only be considered free when all of the nodes
are free.

Instances of this template require instances of survival_kit/templates/stack.h
to be created with the same SKIT_T_NAME and SKIT_T_ELEM_TYPE.

It is the caller's responsibility to #include "survival_kit/templates/stack.h"
  before #include'ing this file.

It is the caller's responsibility to #include "survival_kit/templates/stack.c"
  in only one .c file being linked with the main program.  

It is the caller's responsibility to undefine template parameters after 
#include'ing this file.

EXAMPLE:
-------------------------------------------------------------------------------
// This is a way to stack-allocate memory for use with setjmp/longjmp such
//   that none of the elements in the fstack need to be declared anywhere.
// (This objective may seem unusual, but it is useful for writing macros
//    where having variable declarations in certain places is undesirable.)
#include "survival_kit/setjmp/jmp_fstack.h"
#include <setjmp.h>
#include <assert.h>

int end_was_reached = 0;
skit_jmp_fstack stack;
skit_jmp_fstack_ctor(&stack);
if (stack.used.length <= 0)
	skit_jmp_fstack_grow(&stack,alloca(sizeof(skit_jmp_stnode*)));
if( setjmp(*skit_jmp_fstack_push(&stack)) == 0 )
{
	assert(stack.used.length == 1);
	longjmp(*pop_jmp,1)
	assert(0);
}
else
{
	assert(stack.used.length == 0);
	end_was_reached = 1;
}
assert(end_was_reached);
-------------------------------------------------------------------------------


See "survival_kit/fstack_builtins.h" for stack instantiations of common C builtin
  types, as well as unit tests.

Parameters:

SKIT_T_ELEM_TYPE (required) -
	The type for elements in this stack, ex: int, float.
	
SKIT_T_NAMESPACE (optional) -
	The namespace that the instanced fstack will live in.
	
	With a SKIT_T_NAMESPACE of foobar, the resulting type will be
	foobar_abc_fstack.
	
	This is used when writing libraries to ensure that fstack types
	instantiated in the library do not collide with fstack types instantiated
	in the library user's code or other libraries that happen to have similar
	element names.
	
	This, along with SKIT_T_NAME, can be about 12 characters long in total.
	Any longer and the symbol names generated by the template could be too
	long for the OpenVMS linker to handle.
	
	By default, this is defined like so:
	#define SKIT_T_NAMESPACE skit

SKIT_T_NAME (required) -
	A unique name that is included in all type/function expansions created by
	this template.  It works like so:
	
	EXAMPLE:
	#define SKIT_T_ELEM_TYPE int
	#define SKIT_T_NAME int
	#include "survival_kit/templates/fstack.h"
	#undefine SKIT_T_ELEM_TYPE
	#undefine SKIT_T_NAME
	skit_int_fstack  stack;        // The type 'skit_int_fstack' is defined.
	skit_int_fstack_ctor(&stack);        // The function 'skit_int_fstack_ctor' is defined.

SKIT_T_FUNC_ATTR (optional, defaualts to nothing) - 
	Specifies function attributes to be used on all functions created.
	This is mostly present to allow functions to be instanced as 'static' so
	  that they will not cause linking collisions with instances found in
	  different and unrelated files that happen to have the same SKIT_T_NAME.

SKIT_T_DIE_ON_ERROR (optional) - 
	Defining this makes it so that invalid operations cause skit_die(...) to be 
	called instead of raising an exception.
	This is used by certain places like the feature_emulation.h module where 
	exception throwing can't be used because the stacks are being used to 
	implement the ability to throw exceptions.
*/

#ifndef SKIT_T_NAMESPACE
#define SKIT_T_NAMESPACE skit
#define SKIT_T_NAMESPACE_IS_DEFAULT 1
#endif

#include "survival_kit/templates/skit_t.h"

#ifndef SKIT_T_ELEM_TYPE
#error "SKIT_T_ELEM_TYPE is needed but was not defined."
#endif

typedef struct SKIT_T(fstack) SKIT_T(fstack);
struct SKIT_T(fstack)
{
	SKIT_T(stack) unused;
	SKIT_T(stack) used;
};

/**
Initializes the freestack and any substacks.
Call this on a freestack before calling any of the other freestack functions.
*/
void SKIT_T(fstack_ctor)( SKIT_T(fstack) *stack );

/**
This is a way to check if a free stack has any free nodes left or not.
If this returns 1, then skit_*_fstack_grow must be called and
  provided with memory before skit_*_fstack_push is called.
Returns:
	1 if there are no free nodes in the stack.
	0 if there are free nodes that can be used by skit_*_fstack_push()
*/
int SKIT_T(fstack_full)( SKIT_T(fstack) *stack );

/**
Assigns memory to the freestack.
'node' should be a pointer to an amount of newly allocated memory with
  enough size to fit a skit_*_stnode value: sizeof(skit_mytype_stnode).
*/
void SKIT_T(fstack_grow)( SKIT_T(fstack) *stack, void *node );

/**
Currently not implemented.
This would be a way to reclaim memory from a freestack.
*/
void SKIT_T(fstack_shrink)( SKIT_T(fstack) *stack, int num_nodes );

/**
Grabs an element from the freestack and pushes its node onto the 'used' stack.
A pointer to the element's memory is returned.
The result of this operation can be passed to other routines that might
want to use this value: most commonly these are initialization routines.
There must be free nodes in this stack before calling this.  If there are no
such free nodes, then call skit_*_fstack_grow and provide some memory before
calling this.
Alternatively, use skit_*_fstack_alloc to simplify allocation if your source
of memory is an allocation function.
*/
SKIT_T_ELEM_TYPE *SKIT_T(fstack_push)( SKIT_T(fstack) *stack );

/**
This is like push, except that it does not require there to be free nodes in
the stack and subsequently doesn't require checking used space and calling 
skit_*_fstack_grow.  Instead, if this function needs memory to to grow
the given stack, then it will call the provided 'allocate' function to acquire
more memory.  The signature of the 'allocate' function is expected to be the
same as the common 'malloc' function.
*/
SKIT_T_ELEM_TYPE *SKIT_T(fstack_alloc)( SKIT_T(fstack) *stack, void *allocate(size_t)  );

/**
Moves the last "pushed" element into the 'unused' stack.
The return value is a pointer to the moved node's element.
The data pointed to may be used as long as no other freestack functions are
  called on the stack during the pointer's usage.
*/
SKIT_T_ELEM_TYPE *SKIT_T(fstack_pop)( SKIT_T(fstack) *stack );

/**
Walks the stack, calling 'predicate' on each node.
If 'predicate' returns 0, then the walk will abort.
The end of the unused substack is considered the "first" node and the end of the
  used substack is "last" node.  In other words: walking forward through the 
  freestack involves first walking backwards through the unused stack and then
  forwards through the used stack.
The 'context' variable will be passed directly into the calls on 'predicate'.
'predicate' will not be called on nodes before 'start_node' or after 'end_node'.
If 'start_node' is NULL, the walk will begin at the very beginning of the stack.
If 'start_node' is otherwise not in the stack, then 'predicate' will never be
  be called and an OUT_OF_BOUNDS exception will be thrown. (Currently unimplemented)
If 'end_node' is NULL, the walk will end at the very end of the stack.
If 'end_node' is otherwise not in the stack, then the walk will continue through
  the last node and then throw an OUT_OF_BOUNDS exception. (Currently unimplemented)
If 'end_node' is encountered before 'start_node', then 'predicate' will never
  be called.
*/
void SKIT_T(fstack_walk)(
	const SKIT_T(fstack) *stack,
	int predicate(void *context, const SKIT_T(stnode) *node),
	void *context,
	const SKIT_T(stnode) *start_node,
	const SKIT_T(stnode) *end_node
);


#ifdef SKIT_T_NAMESPACE_IS_DEFAULT
#undef SKIT_T_NAMESPACE
#undef SKIT_T_NAMESPACE_IS_DEFAULT
#endif
