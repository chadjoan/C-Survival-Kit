/** See survival_kit/templates/stack.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/feature_emulation/throw.h"
#endif

#include "survival_kit/misc.h"
#include "survival_kit/templates/stack.h"

#include <stdio.h>

SKIT_T(stack) *SKIT_T(stack_init)(SKIT_T(stack) *list)
{
	list->front = NULL;
	list->length = 0;
	return list;
}

void SKIT_T(stack_push)(SKIT_T(stack) *list, SKIT_T(stnode) *node)
{
	if ( list->front == NULL )
	{
		node->next = NULL; /* This line is necessary for list->front to be NULL when the last node is popped. */
		list->front = node;
	}
	else
	{
		node->next = list->front;
		list->front = node;
	}
	
	list->length += 1;
}

SKIT_T(stnode) *SKIT_T(stack_pop)(SKIT_T(stack) *list)
{
	SKIT_T(stnode) *result;
	
	if ( list->front == NULL )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to pop a value from a zero-length " SKIT_T_STR(stack) ".");
#		else
			THROW(OUT_OF_BOUNDS,"Attempt to pop a value from a zero-length stack.");
#		endif
	}
	else
	{
		result = list->front;
		list->front = result->next;
		result->next = NULL;
	}
	
	(list->length) -= 1;
	
	return result;
}