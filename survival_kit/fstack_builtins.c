
/* Make sure SKIT_T_HEADER isn't defined when #include'ing this. */
#include "survival_kit/stack_builtins.h"

#define SKIT_T_HEADER "survival_kit/templates/fstack.c"
#include "survival_kit/fstack_builtins.h"

#include "survival_kit/misc.h"

#include "survival_kit/setjmp/jmp_fstack.h"
#include <setjmp.h>
#include <assert.h>

#include <stdio.h>

void skit_fstack_unittest()
{
	printf("skit_fstack_unittest()\n");
	
	int end_was_reached = 0;
	skit_jmp_fstack list;
	skit_jmp_fstack_init(&list);
	if (skit_jmp_fstack_full(&list))
		skit_jmp_fstack_grow(&list,alloca(sizeof(skit_jmp_stnode)));
	if( setjmp(*skit_jmp_fstack_push(&list)) == 0 )
	{
		assert(list.used.length == 1);
		longjmp(*skit_jmp_fstack_pop(&list),1);
		assert(0);
	}
	else
	{
		assert(list.used.length == 0);
		end_was_reached = 1;
	}
	assert(end_was_reached);
	
	printf("  skit_fstack_unittest passed!\n");
	printf("\n");
}
