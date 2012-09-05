/** See survival_kit/templates/flist.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
#	include "survival_kit/features.h"
#endif

#include "survival_kit/misc.h"
#include "survival_kit/templates/flist.h"

#include <stdio.h>

void SKIT_T(flist_init)( SKIT_T(flist) *list )
{
	SKIT_T(slist_init)(&(list->unused));
	SKIT_T(slist_init)(&(list->used));
}

int SKIT_T(flist_full)( SKIT_T(flist) *list )
{
	if ( list->unused.length == 0 )
		return 1;
	else
		return 0;
}

void SKIT_T(flist_grow)( SKIT_T(flist) *list, void *node )
{
	SKIT_T(slist_push)(&(list->unused), (SKIT_T(snode)*)node);
}

void SKIT_T(flist_shrink)( SKIT_T(flist) *list, int num_nodes )
{
	/* SKIT_T(slist_push)(&(list->unused), (SKIT_T(snode)*)node); */
	skit_die("flist_shrink(*list,num_nodes) is not implemented.");
}

SKIT_T_ELEM_TYPE *SKIT_T(flist_push)( SKIT_T(flist) *list )
{
	if ( list->unused.length == 0 )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			skit_die("Attempt to push a value in a " SKIT_T_STR(flist) " that has no free nodes.");
#		else
			RAISE(OUT_OF_BOUNDS,"Attempt to push a value in a freelist that has no free nodes.");
#		endif
	}

	SKIT_T(snode) *result = SKIT_T(slist_pop)(&(list->unused));
	SKIT_T(slist_push)(&(list->used),result);
	return &(result->val);
}

SKIT_T_ELEM_TYPE *SKIT_T(flist_pop)( SKIT_T(flist) *list )
{
	SKIT_T(snode) *result = SKIT_T(slist_pop)(&(list->used));
	SKIT_T(slist_push)(&(list->unused),result);
	return &result->val;
}
