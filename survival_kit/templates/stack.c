/** See survival_kit/templates/stack.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/feature_emulation/throw.h"
#endif

#include "survival_kit/misc.h"
#include "survival_kit/assert.h"
#include "survival_kit/templates/stack.h"

#include <stdio.h>

SKIT_T(stack) *SKIT_T(stack_init)(SKIT_T(stack) *stack)
{
	SKIT_ASSERT(stack != NULL);
	stack->front = NULL;
	stack->length = 0;
	return stack;
}

void SKIT_T(stack_push)(SKIT_T(stack) *stack, SKIT_T(stnode) *node)
{
	SKIT_ASSERT(stack != NULL);
	if ( stack->front == NULL )
	{
		node->next = NULL; /* This line is necessary for stack->front to be NULL when the last node is popped. */
		stack->front = node;
	}
	else
	{
		node->next = stack->front;
		stack->front = node;
	}
	
	stack->length += 1;
}

SKIT_T(stnode) *SKIT_T(stack_pop)(SKIT_T(stack) *stack)
{
	SKIT_ASSERT(stack != NULL);
	SKIT_T(stnode) *result;
	
	if ( stack->front == NULL )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to pop a value from a zero-length " SKIT_T_STR(stack) ".");
#		else
			THROW(OUT_OF_BOUNDS,"Attempt to pop a value from a zero-length stack.");
#		endif
	}
	else
	{
		result = stack->front;
		stack->front = result->next;
		result->next = NULL;
	}
	
	(stack->length) -= 1;
	
	return result;
}