/** See survival_kit/templates/slist.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/features.h"
#endif

#include "survival_kit/misc.h"
#include "survival_kit/templates/slist.h"

#include <stdio.h>

SKIT_T(slist) *SKIT_T(slist_init)(SKIT_T(slist) *list)
{
	list->front = NULL;
	list->back = NULL;
	list->length = 0;
	return list;
}

void SKIT_T(slist_push)(SKIT_T(slist) *list, SKIT_T(snode) *node)
{
	if ( list->front == NULL )
	{
		node->next = NULL; /* This line is necessary for list->front to be NULL when the last node is popped. */
		list->front = node;
		list->back = node;
	}
	else
	{
		node->next = list->front;
		list->front = node;
	}
	
	list->length += 1;
}

SKIT_T(snode) *SKIT_T(slist_pop)(SKIT_T(slist) *list)
{
	SKIT_T(snode) *result;
	
	if ( list->front == NULL )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to pop a value from a zero-length " SKIT_T_STR(slist) ".");
#		else
			RAISE(OUT_OF_BOUNDS,"Attempt to pop a value from a zero-length slist.");
#		endif
	}
	else
	{
		result = list->front;
		list->front = result->next;
		result->next = NULL;
		if ( list->front == NULL )
			list->back = NULL;
	}
	
	(list->length) -= 1;
	
	return result;
}