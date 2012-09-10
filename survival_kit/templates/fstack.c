/** See survival_kit/templates/fstack.h for documentation. */

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */

#include "survival_kit/feature_emulation/generated_exception_defs.h"

	/*
	This skit_throw_exception_no_ctx definition is used to break macro recursion.
	This definition is duplicated in "survival_kit/feature_emulation/stack.c" and
	  "survival_kit/feature_emulation/funcs.h".  If this ever changes, then those
	  must be updated.
	See "survival_kit/feature_emulation/funcs.h" for rationale.
	*/
void skit_throw_exception_no_ctx(
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...);
#endif

#include "survival_kit/misc.h"
#include "survival_kit/assert.h"
#include "survival_kit/templates/fstack.h"

#include <stdlib.h> /* for size_t */
#include <stdio.h>

void SKIT_T(fstack_init)( SKIT_T(fstack) *stack )
{
	SKIT_ASSERT(stack != NULL);
	SKIT_T(stack_init)(&(stack->unused));
	SKIT_T(stack_init)(&(stack->used));
}

void SKIT_T(fstack_grow)( SKIT_T(fstack) *stack, void *node )
{
	SKIT_ASSERT(stack != NULL);
	SKIT_ASSERT(node != NULL);
	SKIT_T(stack_push)(&(stack->unused), (SKIT_T(stnode)*)node);
}

void SKIT_T(fstack_shrink)( SKIT_T(fstack) *stack, int num_nodes )
{
	SKIT_ASSERT(stack != NULL);
	/* SKIT_T(stack_push)(&(stack->unused), (SKIT_T(stnode)*)node); */
	skit_die("fstack_shrink(*stack,num_nodes) is not implemented.");
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_push)( SKIT_T(fstack) *stack )
{
	SKIT_ASSERT(stack != NULL);
	if ( stack->unused.length == 0 )
	{
#		if defined(SKIT_T_DIE_ON_ERROR)
			SKIT_ASSERT_MSG(0, "Attempt to push a value in a " SKIT_T_STR(fstack) " that has no free nodes.");
#		else
			skit_throw_exception_no_ctx( __LINE__, __FILE__, __func__,
				OUT_OF_BOUNDS,"Attempt to push a value in a freelist that has no free nodes.");
#		endif
	}

	SKIT_T(stnode) *result = SKIT_T(stack_pop)(&(stack->unused));
	SKIT_T(stack_push)(&(stack->used),result);
	return &(result->val);
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_alloc)( SKIT_T(fstack) *stack, void *allocate(size_t)  )
{
	SKIT_ASSERT(stack != NULL);
	SKIT_ASSERT(allocate != NULL);
	
	if ( stack->unused.length == 0 )
	{
		/* Directly allocate onto the "used" stack. */
		SKIT_T(stnode) *result = (SKIT_T(stnode)*)allocate(sizeof(SKIT_T(stnode)));
		SKIT_T(stack_push)(&(stack->used),result);
		return &(result->val);
	}
	else
	{
		/* Re-use existing nodes. */
		return SKIT_T(fstack_push)(stack);
	}
}

SKIT_T_ELEM_TYPE *SKIT_T(fstack_pop)( SKIT_T(fstack) *stack )
{
	SKIT_ASSERT(stack != NULL);
	
	SKIT_T(stnode) *result = SKIT_T(stack_pop)(&(stack->used));
	SKIT_T(stack_push)(&(stack->unused),result);
	
	return &result->val;
}

void SKIT_T(fstack_walk)(
	const SKIT_T(fstack) *stack,
	int predicate(void *context, const SKIT_T(stnode) *node),
	void *context,
	const SKIT_T(stnode) *start_node,
	const SKIT_T(stnode) *end_node
)
{
	int start_node_reached = 0;
	int end_node_reached = 0;
	
	/* NULL is used to signal that the very beginning of the fstack is to
	be at the start of the walk. */
	if ( start_node == NULL )
		start_node_reached = 1;
	
	/* We'll need some random-access memory for storing the unused nodes that 
	we walk so that we can walk them backwards (in stack order, forwards in
	fstack order). */
	char reversing_buffer_stack_mem[1024];
	SKIT_T(stnode) **reversing_buffer;
	if ( stack->unused.length * sizeof(SKIT_T(stnode)*) < 1024 )
		reversing_buffer = (SKIT_T(stnode)**)reversing_buffer_stack_mem;
	else
		reversing_buffer = (SKIT_T(stnode)**)malloc(stack->unused.length * sizeof(SKIT_T(stnode)));
	
	/* Populate the buffer used to walk the unused stack backwards. */
	SKIT_T(stnode) *cur_node;
	ssize_t reversing_index = 0;
	
	cur_node = stack->unused.front;
	while ( cur_node != NULL )
	{
		reversing_buffer[reversing_index] = cur_node;
		reversing_index += 1;
		
		if ( cur_node == start_node )
		{
			start_node_reached = 1;
			break;
		}
		
		cur_node = cur_node->next;
	}
	
	/* Walk the part that exists in the unused buffer. */
	/* We only need to do this if the start_node is in there or the caller is
	   requesting that we start at the very beginning (eg. start_node==NULL). */
	if ( start_node_reached || start_node == NULL )
	{
		while ( reversing_index > 0 )
		{
			reversing_index--;
			
			cur_node = reversing_buffer[reversing_index];
			
			if ( predicate(context, cur_node) == 0 )
			{
				end_node_reached = 1;
				break;
			}
			
			if ( cur_node == end_node )
			{
				end_node_reached = 1;
				break;
			}
		}
	}
	
	/* Now walk the used buffer. */
	if ( !end_node_reached )
	{
		cur_node = stack->used.front;
		while ( cur_node != NULL )
		{
			if ( cur_node == start_node )
				start_node_reached = 1;
			
			if ( start_node_reached && (predicate(context, cur_node) == 0) )
			{
				end_node_reached = 1;
				break;
			}
			
			if ( cur_node == end_node )
			{
				end_node_reached = 1;
				break;
			}
			
			cur_node = cur_node->next;
		}
	}
	
	/* TODO: THROW with OUT_OF_BOUNDS if things start/end nodes are non-NULL and aren't found. */
	
	/* Free heap memory if we used it. */
	if ( ((char*)reversing_buffer) != reversing_buffer_stack_mem )
		free((void*)reversing_buffer);
}
