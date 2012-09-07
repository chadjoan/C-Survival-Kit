/** See survival_kit/templates/fstack.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/feature_emulation/throw.h"
#endif

#include "survival_kit/misc.h"
#include "survival_kit/assert.h"
#include "survival_kit/templates/fstack.h"

#include <stdlib.h> /* for size_t */
#include <stdio.h>

void SKIT_T(fstack_init)( SKIT_T(fstack) *stack )
{
	SKIT_ASSERT(stack != NULL);
	SKIT_T(stack_init)(&(stack->unused));
	SKIT_T(stack_init)(&(stack->used));
}

void SKIT_T(fstack_grow)( SKIT_T(fstack) *stack, void *node )
{
	SKIT_ASSERT(stack != NULL);
	SKIT_ASSERT(node != NULL);
	SKIT_T(stack_push)(&(stack->unused), (SKIT_T(stnode)*)node);
}

void SKIT_T(fstack_shrink)( SKIT_T(fstack) *stack, int num_nodes )
{
	SKIT_ASSERT(stack != NULL);
	/* SKIT_T(stack_push)(&(stack->unused), (SKIT_T(stnode)*)node); */
	skit_die("fstack_shrink(*stack,num_nodes) is not implemented.");
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_push)( SKIT_T(fstack) *stack )
{
	SKIT_ASSERT(stack != NULL);
	if ( stack->unused.length == 0 )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to push a value in a " SKIT_T_STR(fstack) " that has no free nodes.");
#		else
			THROW(OUT_OF_BOUNDS,"Attempt to push a value in a freelist that has no free nodes.");
#		endif
	}

	SKIT_T(stnode) *result = SKIT_T(stack_pop)(&(stack->unused));
	SKIT_T(stack_push)(&(stack->used),result);
	return &(result->val);
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_alloc)( SKIT_T(fstack) *stack, void *allocate(size_t)  )
{
	SKIT_ASSERT(stack != NULL);
	SKIT_ASSERT(allocate != NULL);
	
	if ( stack->unused.length == 0 )
	{
		/* Directly allocate onto the "used" stack. */
		SKIT_T(stnode) *result = (SKIT_T(stnode)*)allocate(sizeof(SKIT_T(stnode)));
		SKIT_T(stack_push)(&(stack->used),result);
		return &(result->val);
	}
	else
	{
		/* Re-use existing nodes. */
		return SKIT_T(fstack_push)(stack);
	}
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_pop)( SKIT_T(fstack) *stack )
{
	SKIT_ASSERT(stack != NULL);
	
	SKIT_T(stnode) *result = SKIT_T(stack_pop)(&(stack->used));
	SKIT_T(stack_push)(&(stack->unused),result);
	
	return &result->val;
}
