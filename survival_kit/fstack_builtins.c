
#ifdef __DECC
#pragma module skit_fstack_builtins
#endif

/* Although this module may or may not use feature_emulation, fstack.c does. */
/* As a consequence, fstack.c can #include exception.h which #includes */
/* frame_info.h which #includes ... fstack.h.  This then attempts to redefine */
/* SKIT_T_ELEM_TYPE which and causes all kinds of havoc. */
/* To avoid this nasty circular macro invokation, we get feature_emulation.h */
/* out of the way at the very beginning and let it define its fstack types */
/* first.  After that we will no longer need to worry about accidentally */
/* creating recursive macro inclusions against feature_emulation submodules. */
#include "survival_kit/feature_emulation.h"

#include <setjmp.h>
#include <assert.h>

#include <stdio.h>

/* Make sure SKIT_T_HEADER isn't defined when #include'ing this. */
#include "survival_kit/stack_builtins.h"

#define SKIT_T_HEADER "survival_kit/templates/fstack.h"
#include "survival_kit/fstack_builtins.h"
#undef SKIT_T_HEADER
#undef SKIT_FSTACK_BUILTINS_INCLUDED

#define SKIT_T_HEADER "survival_kit/templates/fstack.c"
#include "survival_kit/fstack_builtins.h"
#undef SKIT_T_HEADER

#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/cstr.h"

typedef struct skit_fstack_unittest_context skit_fstack_unittest_context;
struct skit_fstack_unittest_context
{
	uint32_t expected_vals[3];
	int count;
};

int skit_fstack_walk_unittest(void *context, const skit_i32_stnode *node )
{
	skit_fstack_unittest_context *ctx = context;
	
	sASSERT_EQ( node->val, ctx->expected_vals[ctx->count] );
	
	(ctx->count)++;
	
	return 1;
}

void skit_fstack_unittest()
{
	printf("skit_fstack_unittest()\n");
	
	skit_fstack_unittest_context ctx;
	
	int32_t *i32;
	skit_i32_stnode *node_save;
	skit_i32_fstack i32stack;
	skit_i32_fstack_ctor(&i32stack);
	i32 = skit_i32_fstack_alloc(&i32stack,&skit_malloc);
	*i32 = 1;
	
	i32 = skit_i32_fstack_alloc(&i32stack,&skit_malloc);
	*i32 = 2;
	
	i32 = skit_i32_fstack_alloc(&i32stack,&skit_malloc);
	*i32 = 3;
	
	sASSERT_EQ(i32stack.used.length,   3);
	sASSERT_EQ(i32stack.unused.length, 0);
	
	ctx.count = 0;
	ctx.expected_vals[0] = 3;
	ctx.expected_vals[1] = 2;
	ctx.expected_vals[2] = 1;
	
	skit_i32_fstack_walk( &i32stack, &skit_fstack_walk_unittest, &ctx, NULL, NULL);
	sASSERT_EQ(ctx.count, 3);
	
	i32 = skit_i32_fstack_pop(&i32stack);
	sASSERT_EQ(*i32,3);
	
	i32 = skit_i32_fstack_pop(&i32stack);
	sASSERT_EQ(*i32,2);
	
	i32 = skit_i32_fstack_alloc(&i32stack,&skit_malloc);
	*i32 = 42;
	node_save = i32stack.used.front;
	
	ctx.count = 0;
	ctx.expected_vals[0] = 42;
	ctx.expected_vals[1] = 1;
	skit_i32_fstack_walk( &i32stack, &skit_fstack_walk_unittest, &ctx, node_save, NULL);
	sASSERT_EQ(ctx.count, 2);
	
	i32 = skit_i32_fstack_pop(&i32stack);
	sASSERT_EQ(*i32,42);
	
	i32 = skit_i32_fstack_pop(&i32stack);
	sASSERT_EQ(*i32,1);
	
	sASSERT_EQ(i32stack.used.length,   0);
	sASSERT_EQ(i32stack.unused.length, 3);
	
	ctx.count = 0;
	ctx.expected_vals[0] = 3;
	ctx.expected_vals[1] = 42;
	ctx.expected_vals[2] = 1;
	
	skit_i32_fstack_walk( &i32stack, &skit_fstack_walk_unittest, &ctx, NULL, NULL);
	sASSERT_EQ(ctx.count, 3);
	
	ctx.count = 0;
	ctx.expected_vals[0] = 42;
	ctx.expected_vals[1] = 1;
	skit_i32_fstack_walk( &i32stack, &skit_fstack_walk_unittest, &ctx, node_save, NULL);
	sASSERT_EQ(ctx.count, 2);
	
	ctx.count = 0;
	ctx.expected_vals[0] = 3;
	ctx.expected_vals[1] = 42;
	skit_i32_fstack_walk( &i32stack, &skit_fstack_walk_unittest, &ctx, NULL, node_save);
	sASSERT_EQ(ctx.count, 2);
	
	
	printf("  skit_fstack_unittest passed!\n");
	printf("\n");
}
