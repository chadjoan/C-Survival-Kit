
/* Make sure SKIT_T_HEADER isn't defined when #include'ing this. */
#include "survival_kit/slist_builtins.h"

#define SKIT_T_HEADER "survival_kit/templates/flist.c"
#include "survival_kit/flist_builtins.h"

#include "survival_kit/misc.h"

#include "survival_kit/setjmp/jmp_flist.h"
#include <setjmp.h>
#include <assert.h>

#include <stdio.h>

void skit_flist_unittest()
{
	printf("skit_flist_unittest()\n");
	
	int end_was_reached = 0;
	skit_jmp_flist list;
	skit_jmp_flist_init(&list);
	if (skit_jmp_flist_full(&list))
		skit_jmp_flist_grow(&list,alloca(sizeof(skit_jmp_snode)));
	if( setjmp(*skit_jmp_flist_push(&list)) == 0 )
	{
		assert(list.used.length == 1);
		longjmp(*skit_jmp_flist_pop(&list),1);
		assert(0);
	}
	else
	{
		assert(list.used.length == 0);
		end_was_reached = 1;
	}
	assert(end_was_reached);
	
	printf("  skit_flist_unittest passed!\n");
	printf("\n");
}
