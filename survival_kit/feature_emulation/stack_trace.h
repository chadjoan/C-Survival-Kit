
#ifndef SKIT_FEMU_STACK_TRACE_INCLUDED
#define SKIT_FEMU_STACK_TRACE_INCLUDED

#include <inttypes.h>

/* Break #include recursion by forward-referencing these types. */
/* This header only uses pointers to them anyways, so it does not need to know details. */
struct skit_thread_context;
struct skit_debug_stack;
struct skit_debug_fstack;
struct skit_debug_stnode;

/** 
Prints the current stack trace to a string and returns it.
In order to prevent dynamic allocation, this will use a thread-local buffer 
for storing the string.  This buffer may be overwritten by other calls to
skit_* functions, so be sure to copy it to somewhere else before calling
those functions.
*/
#define skit_stack_trace_to_str() skit_stack_trace_to_str_expr(__LINE__,__FILE__,__func__, " <- skit_stack_trace_to_str() called.")
const char *skit_stack_trace_to_str_expr( uint32_t line, const char *file, const char *func, const char *specifics );

/** Prints the current stack trace to stdout. */
/* These definitions are duplicated in "survival_kit/assert.h" to prevent macro recursion. */
/* Be sure to change those definitions if altering these. */
#define skit_print_stack_trace() skit_print_stack_trace_func(__LINE__,__FILE__,__func__, " <- skit_print_stack_trace() called.")
void skit_print_stack_trace_func( uint32_t line, const char *file, const char *func, const char *specifics );


/* 
Internal use (feature_emulation submodules) only.
This is primarily used for printing stack traces that were saved much
earlier in execution when the stack trace would have looked much different.
*/
char *skit_stack_to_str_internal(
	const struct skit_thread_context *skit_thread_ctx,
	const struct skit_debug_stack  *stack,
	const struct skit_debug_stnode *stack_start);

/* 
Internal use (feature_emulation submodules) only.
This will print all of the debug info in the given stack, including
already-popped frames if dictated by the location of stack_start.
*/
char *skit_fstack_to_str_internal(
	const struct skit_thread_context *skit_thread_ctx,
	const struct skit_debug_fstack *stack,
	const struct skit_debug_stnode *stack_start);

#endif
