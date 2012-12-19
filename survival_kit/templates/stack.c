/** See survival_kit/templates/stack.h for documentation. */

#ifndef SKIT_T_NAMESPACE
#define SKIT_T_NAMESPACE skit
#define SKIT_T_NAMESPACE_IS_DEFAULT 1
#endif

#ifndef SKIT_T_DIE_ON_ERROR
	/* Throw exceptions instead. */
	#include "survival_kit/feature_emulation/exceptions.h"

	/*
	This skit_throw_exception_no_ctx definition is used to break macro recursion.
	This definition is duplicated in "survival_kit/feature_emulation/fstack.c" and
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
#include "survival_kit/memory.h"

#include <stdio.h>

SKIT_T(stack) *SKIT_T(stack_init)(SKIT_T(stack) *stack)
{
	sASSERT(stack != NULL);
	stack->front = NULL;
	stack->back = NULL;
	stack->length = 0;
	return stack;
}

void SKIT_T(stack_push)(SKIT_T(stack) *stack, SKIT_T(stnode) *node)
{
	sASSERT(stack != NULL);
	if ( stack->front == NULL )
	{
		node->next = NULL; /* This line is necessary for stack->front to be NULL when the last node is popped. */
		stack->front = node;
		stack->back = node;
	}
	else
	{
		node->next = stack->front;
		stack->front = node;
	}
	
	stack->length += 1;
}

void SKIT_T(stack_append)(SKIT_T(stack) *stack, SKIT_T(stnode) *node)
{
	sASSERT(stack != NULL);
	if ( stack->back == NULL )
	{
		node->next = NULL; /* This line is necessary for stack->front to be NULL when the last node is popped. */
		stack->front = node;
		stack->back = node;
	}
	else
	{
		node->next = NULL;
		stack->back->next = node;
		stack->back = node;
	}
	
	stack->length += 1;
}

SKIT_T(stnode) *SKIT_T(stack_pop)(SKIT_T(stack) *stack)
{
	sASSERT(stack != NULL);
	SKIT_T(stnode) *result;
	
	if ( stack->front == NULL )
	{

#		if defined(SKIT_T_DIE_ON_ERROR)
			sASSERT_MSG(0, "Attempt to pop a value from a zero-length " SKIT_T_STR(stack) ".");
#		else
			skit_throw_exception_no_ctx( __LINE__, __FILE__, __func__,
				SKIT_OUT_OF_BOUNDS,"Attempt to pop a value from a zero-length stack.");
#		endif
	}
	else
	{
		result = stack->front;
		stack->front = result->next;
		result->next = NULL;
	}
	
	(stack->length) -= 1;
	
	if ( stack->front == NULL )
		stack->back = NULL;
	
	return result;
}

static int SKIT_T(stack_rdup_iter)(void *context, const SKIT_T(stnode) *node)
{
	SKIT_T(stack) *new_stack = context;
	SKIT_T(stnode) *new_node = skit_malloc(sizeof(SKIT_T(stnode)));
	memcpy(new_node, node, sizeof(SKIT_T(stnode)));
	SKIT_T(stack_push)(new_stack, new_node);
	return 1;
}

SKIT_T(stack) *SKIT_T(stack_rdup)(SKIT_T(stack) *stack)
{
	sASSERT(stack != NULL);
	
	SKIT_T(stack) *new_stack = skit_malloc(sizeof(SKIT_T(stack)));
	SKIT_T(stack_init)(new_stack);
	
	SKIT_T(stack_walk)(stack, &SKIT_T(stack_rdup_iter), new_stack, NULL, NULL);
	
	return new_stack;
}

static int SKIT_T(stack_dup_iter)(void *context, const SKIT_T(stnode) *node)
{
	SKIT_T(stack) *new_stack = context;
	SKIT_T(stnode) *new_node = skit_malloc(sizeof(SKIT_T(stnode)));
	memcpy(new_node, node, sizeof(SKIT_T(stnode)));
	SKIT_T(stack_append)(new_stack, new_node);
	return 1;
}

SKIT_T(stack) *SKIT_T(stack_dup)(SKIT_T(stack) *stack)
{
	sASSERT(stack != NULL);
	
	SKIT_T(stack) *new_stack = skit_malloc(sizeof(SKIT_T(stack)));
	SKIT_T(stack_init)(new_stack);
	
	SKIT_T(stack_walk)(stack, &SKIT_T(stack_dup_iter), new_stack, NULL, NULL);
	
	return new_stack;
}

void SKIT_T(stack_walk)(
	const SKIT_T(stack) *stack,
	int predicate(void *context, const SKIT_T(stnode) *node),
	void *context,
	const SKIT_T(stnode) *start_node,
	const SKIT_T(stnode) *end_node
)
{
	SKIT_T(stnode) *cur_node = NULL;
	int start_node_reached = 0;
	int end_node_reached = 0;
	
	/* NULL is used to signal that the very beginning of the stack is to
	be at the start of the walk. */
	if ( start_node == NULL )
		start_node_reached = 1;
	
	cur_node = stack->front;
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
	/* TODO: THROW with OUT_OF_BOUNDS if things start/end nodes are non-NULL and aren't found. */
}

void SKIT_T(stack_reverse)( SKIT_T(stack) *stack )
{
	SKIT_T(stnode) *orig_front = stack->front;
	SKIT_T(stnode) *tmp;
	
	/* The reverse of the empty stack is itself. */
	if ( orig_front == NULL )
		return;
	
	while ( 1 )
	{
		tmp = orig_front->next;
		
		/* When there are no more nodes to move to the front, all is done. */
		if ( tmp == NULL )
		{
			stack->back = orig_front;
			return;
		}
		
		/* Remove the node just past the old front of the list. */
		orig_front->next = tmp->next;
		
		/* 
		Place that node at the front of the list.
		Don't push.  It'll be a bunch of extra calculations, and then you'll
		have to compensate for the changes in length too.
		Instead, we use specialized code to move the node to the front.
		*/
		tmp->next = stack->front;
		stack->front = tmp;
	}
}

#ifdef SKIT_T_NAMESPACE_IS_DEFAULT
#undef SKIT_T_NAMESPACE
#undef SKIT_T_NAMESPACE_IS_DEFAULT
#endif

