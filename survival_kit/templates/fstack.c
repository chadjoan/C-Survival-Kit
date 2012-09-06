/** See survival_kit/templates/fstack.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/features.h"
#endif

#include "survival_kit/misc.h"
#include "survival_kit/templates/fstack.h"

#include <stdlib.h> /* for size_t */
#include <stdio.h>

void SKIT_T(fstack_init)( SKIT_T(fstack) *list )
{
	SKIT_T(stack_init)(&(list->unused));
	SKIT_T(stack_init)(&(list->used));
}

void SKIT_T(fstack_grow)( SKIT_T(fstack) *list, void *node )
{
	SKIT_T(stack_push)(&(list->unused), (SKIT_T(stnode)*)node);
}

void SKIT_T(fstack_shrink)( SKIT_T(fstack) *list, int num_nodes )
{
	/* SKIT_T(stack_push)(&(list->unused), (SKIT_T(stnode)*)node); */
	skit_die("fstack_shrink(*list,num_nodes) is not implemented.");
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_push)( SKIT_T(fstack) *list )
{
	if ( list->unused.length == 0 )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to push a value in a " SKIT_T_STR(fstack) " that has no free nodes.");
#		else
			RAISE(OUT_OF_BOUNDS,"Attempt to push a value in a freelist that has no free nodes.");
#		endif
	}

	SKIT_T(stnode) *result = SKIT_T(stack_pop)(&(list->unused));
	SKIT_T(stack_push)(&(list->used),result);
	return &(result->val);
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_alloc)( SKIT_T(fstack) *list, void *function(size_t) allocate )
{
	if ( list->unused.length == 0 )
	{
		/* Directly allocate onto the "used" stack. */
		SKIT_T(stnode) *result = (SKIT_T(stnode)*)allocate(sizeof(stnode));
		SKIT_T(stack_push)(&(list->used),result);
		return &(result->val);
	}
	else
	{
		/* Re-use existing nodes. */
		return SKIT_T(fstack_push)(list);
	}
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_pop)( SKIT_T(fstack) *list )
{
	SKIT_T(stnode) *result = SKIT_T(stack_pop)(&(list->used));
	SKIT_T(stack_push)(&(list->unused),result);
	return &result->val;
}
