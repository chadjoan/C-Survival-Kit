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

#include <stdio.h>

SKIT_T(stack) *SKIT_T(stack_init)(SKIT_T(stack) *stack)
{
	sASSERT(stack != NULL);
	stack->front = NULL;
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
	}
	else
	{
		node->next = stack->front;
		stack->front = node;
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
	
	return result;
}

#ifdef SKIT_T_NAMESPACE_IS_DEFAULT
#undef SKIT_T_NAMESPACE
#undef SKIT_T_NAMESPACE_IS_DEFAULT
#endif

