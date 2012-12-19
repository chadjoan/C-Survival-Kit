
#ifdef __DECC
#pragma module skit_stack_builtins
#endif

#define SKIT_T_HEADER "survival_kit/templates/stack.h"
#include "survival_kit/stack_builtins.h"
#undef SKIT_T_HEADER
#undef SKIT_STACK_BUILTINS_INCLUDED

#define SKIT_T_HEADER "survival_kit/templates/stack.c"
#include "survival_kit/stack_builtins.h"
#undef SKIT_T_HEADER

#include <stdio.h>
#include <assert.h>

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE int
#define SKIT_T_NAME utest_int
#define SKIT_T_FUNC_ATTR static
#include "survival_kit/templates/stack.h"
#include "survival_kit/templates/stack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_FUNC_ATTR

typedef struct skit_stack_unittest_context skit_stack_unittest_context;
struct skit_stack_unittest_context
{
	uint32_t expected_vals[3];
	int count;
};

int skit_stack_walk_unittest(void *context, const skit_utest_int_stnode *node )
{
	skit_stack_unittest_context *ctx = context;
	
	sASSERT_EQ( node->val, ctx->expected_vals[ctx->count], "%d" );
	
	(ctx->count)++;
	
	return 1;
}

void skit_stack_unittest()
{
	printf("skit_stack_unittest()\n");
	
	skit_stack_unittest_context ctx;
	
	skit_utest_int_stack list;
	skit_utest_int_stnode node1;
	skit_utest_int_stnode node2;
	skit_utest_int_stnode node3;
	skit_utest_int_stnode node_res;
	skit_utest_int_stnode *copy_node;
	node1.val = 9;
	node2.val = 42;
	node3.val = 1337;
	
	skit_utest_int_stack_init(&list);
	assert(list.length == 0);
	assert(list.front == NULL);
	assert(list.back == NULL);
	
	skit_utest_int_stack_push(&list,&node1);
	assert(list.length == 1);
	assert(list.front == &node1);
	assert(list.back == &node1);
	
	skit_utest_int_stack_push(&list,&node2);
	assert(list.length == 2);
	
	node_res = *skit_utest_int_stack_pop(&list);
	assert(list.length == 1);
	assert(node_res.val == 42);
	
	skit_utest_int_stack_push(&list,&node3);
	assert(list.length == 2);
	sASSERT_EQ(list.front->val, 1337, "%d");
	sASSERT_EQ(list.back->val, 9, "%d");
	
	ctx.count = 0;
	ctx.expected_vals[0] = 1337;
	ctx.expected_vals[1] = 9;
	
	skit_utest_int_stack_walk( &list, &skit_stack_walk_unittest, &ctx, NULL, NULL);
	sASSERT_EQ(ctx.count, 2, "%d");
	
	skit_utest_int_stack *the_copy = skit_utest_int_stack_dup( &list );
	
	ctx.count = 0;
	skit_utest_int_stack_walk( the_copy, &skit_stack_walk_unittest, &ctx, NULL, NULL);
	sASSERT_EQ(ctx.count, 2, "%d");
	
	
	copy_node = skit_utest_int_stack_pop(the_copy);
	sASSERT_EQ(the_copy->length, 1, "%d");
	sASSERT_EQ(copy_node->val, 1337, "%d");
	skit_free(copy_node);
	
	copy_node = skit_utest_int_stack_pop(the_copy);
	sASSERT_EQ(the_copy->length, 0, "%d");
	sASSERT(the_copy->front == NULL);
	sASSERT(the_copy->back == NULL);
	sASSERT_EQ(copy_node->val, 9, "%d");
	skit_free(copy_node);
	
	skit_free(the_copy);
	
	the_copy = skit_utest_int_stack_rdup( &list );
	
	ctx.count = 0;
	ctx.expected_vals[0] = 9;
	ctx.expected_vals[1] = 1337;
	skit_utest_int_stack_walk( the_copy, &skit_stack_walk_unittest, &ctx, NULL, NULL);
	sASSERT_EQ(ctx.count, 2, "%d");
	
	skit_utest_int_stack_reverse( the_copy );
	
	ctx.count = 0;
	ctx.expected_vals[0] = 1337;
	ctx.expected_vals[1] = 9;
	skit_utest_int_stack_walk( the_copy, &skit_stack_walk_unittest, &ctx, NULL, NULL);
	sASSERT_EQ(ctx.count, 2, "%d");
	
	copy_node = skit_utest_int_stack_pop(the_copy);
	sASSERT_EQ(the_copy->length, 1, "%d");
	sASSERT_EQ(copy_node->val, 1337, "%d");
	skit_free(copy_node);
	
	copy_node = skit_utest_int_stack_pop(the_copy);
	sASSERT_EQ(the_copy->length, 0, "%d");
	sASSERT(the_copy->front == NULL);
	sASSERT(the_copy->back == NULL);
	sASSERT_EQ(copy_node->val, 9, "%d");
	skit_free(copy_node);
	
	
	sASSERT_EQ(list.front->val, 1337, "%d");
	sASSERT_EQ(list.back->val, 9, "%d");
	
	node_res = *skit_utest_int_stack_pop(&list);
	assert(list.length == 1);
	assert(node_res.val == 1337);
	
	node_res = *skit_utest_int_stack_pop(&list);
	assert(list.length == 0);
	assert(list.front == NULL);
	assert(list.back == NULL);
	assert(node_res.val == 9);
	
	printf("  skit_stack_unittest passed!\n");
	printf("\n");
}