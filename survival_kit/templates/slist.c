/** See survival_kit/templates/list.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/features.h"
#endif

#include "survival_kit/templates/slist.h"

SKIT_T(slist) *SKIT_T(slist_init)(SKIT_T(slist) *list)
{
	list->start = NULL;
	list->end = NULL;
	list->length = 0;
	return list;
}

void SKIT_T(slist_push)(SKIT_T(slist) *list, SKIT_T(snode) *node)
{
	if ( list->start == NULL )
	{
		node->next = NULL; /* This line is necessary for list->start to be NULL when the last node is popped. */
		list->start = node;
		list->end = node;
	}
	else
	{
		node->next = list->start;
		list->start = node;
	}
	
	list->length += 1;
}

SKIT_T(snode) *SKIT_T(slist_pop)(SKIT_T(slist) *list)
{
	SKIT_T(snode) *result;
	
	if ( list->start == NULL )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to pop a value from a zero-length slist.");
#		else
			RAISE(OUT_OF_BOUNDS,"Attempt to pop a value from a zero-length slist.");
#		endif
	}
	else
	{
		result = list->start;
		list->start = result->next;
		result->next = NULL;
		if ( list->start == NULL )
			list->end = NULL;
	}
	
	list->length -= 1;
	
	return result;
}