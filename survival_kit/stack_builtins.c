
#define SKIT_T_HEADER "survival_kit/templates/stack.c"
#include "survival_kit/stack_builtins.h"

#include <stdio.h>
#include <assert.h>

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE int
#define SKIT_T_PREFIX utest_int
#define SKIT_T_FUNC_ATTR static
#include SKIT_T_HEADER
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_PREFIX
#undef SKIT_T_FUNC_ATTR

void skit_stack_unittest()
{
	printf("skit_stack_unittest()\n");
	skit_utest_int_stack list;
	skit_utest_int_stnode node1;
	skit_utest_int_stnode node2;
	skit_utest_int_stnode node3;
	skit_utest_int_stnode node_res;
	node1.val = 9;
	node2.val = 42;
	node3.val = 1337;
	
	skit_utest_int_stack_init(&list);
	assert(list.length == 0);
	assert(list.front == NULL);
	
	skit_utest_int_stack_push(&list,&node1);
	assert(list.length == 1);
	assert(list.front == &node1);
	
	skit_utest_int_stack_push(&list,&node2);
	assert(list.length == 2);
	
	node_res = *skit_utest_int_stack_pop(&list);
	assert(list.length == 1);
	assert(node_res.val == 42);
	
	skit_utest_int_stack_push(&list,&node3);
	assert(list.length == 2);
	
	node_res = *skit_utest_int_stack_pop(&list);
	assert(list.length == 1);
	assert(node_res.val == 1337);
	
	node_res = *skit_utest_int_stack_pop(&list);
	assert(list.length == 0);
	assert(list.front == NULL);
	assert(node_res.val == 9);
	
	printf("  skit_stack_unittest passed!\n");
	printf("\n");
}