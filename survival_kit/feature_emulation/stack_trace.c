
#ifdef __DECC
#pragma module skit_feature_emu_stack_trace
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/init.h"
#include "survival_kit/feature_emulation/stack_trace.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/debug.h"


/* ------------------------------------------------------------------------- */

#ifdef __VMS
static void best_effort_vms_path_parse_device(
	const char *path,
	      char **device,
	const char **rest )
{
	size_t i = 0;
	char c = path[i];
	while ( c != '\0' )
	{
		if ( c == ':' )
		{
			*device = skit_malloc(i+1);
			memcpy(*device, path, i);
			(*device)[i] = '\0';
			*rest = path+i+1;
			return;
		}
		
		c = path[++i];
	}
	
	/* Return the empty string for device. */
	*device = skit_malloc(1);
	(*device)[0] = '\0';
	*rest = path;
	return;
}

/* ------------------------------------------------------------------------- */

static void best_effort_vms_path_parse_directory(
	const char *path,
	      char **directory,
	const char **rest )
{
	size_t i = 0;
	char endchar = '\0';
	char c = path[i];
	if ( c == '[' || c == '<' )
	while ( c != '\0' )
	{
		if ( c == '[' )
		{
			endchar = ']';
		}
		else if ( c == '<' )
		{
			endchar = '>';
		}
		else if ( i > 0 && c == endchar )
		{
			*directory = skit_malloc(i);
			memcpy(*directory, path+1, i);
			(*directory)[i] = '\0';
			*rest = path+i+1;
			return;
		}
		
		c = path[++i];
	}
	
	/* Return the empty string for directory. */
	*directory = skit_malloc(1);
	(*directory)[0] = '\0';
	*rest = path;
	return;
}

/* ------------------------------------------------------------------------- */

/* Attempts to break apart an OpenVMS path. */
/* This does not make system calls, so it could become outdated. */
static void best_effort_vms_path_parse(
	const char *path,        /* input: the openvms path to parse. */
	char **device,
	char **directory,
	char **name)
{
	const char *rest = path;
	best_effort_vms_path_parse_device(rest, device, &rest);
	best_effort_vms_path_parse_directory(rest, directory, &rest);
	
	size_t rest_len = strlen(rest);
	*name = skit_malloc(rest_len+1);
	memcpy(*name, rest, rest_len);
	(*name)[rest_len] = '\0';
	return;
}
#endif

/* ------------------------------------------------------------------------- */

typedef struct skit_stack_to_str_context skit_stack_to_str_context;
struct skit_stack_to_str_context
{
	char *msg_pos;
	ssize_t msg_rest_length;
};

static int skit_stack_to_str_each(void *context, const skit_debug_stnode *node)
{
	skit_stack_to_str_context *ctx = (skit_stack_to_str_context*)context;
	
	const skit_frame_info *fi = &node->val;
	
#ifdef __VMS
	char *device;
	char *directory;
	char *name;
	best_effort_vms_path_parse(fi->file_name, &device, &directory, &name);
#elif defined(__linux__)
	const char *name = fi->file_name;
#else
#error "Unsupported target.  This code needs porting."
#endif
	
	ssize_t nchars = snprintf(
		ctx->msg_pos, ctx->msg_rest_length,
		"%s: at line %d in %s: %s\r\n",
		name,
		fi->line_number,
		fi->func_name,
		fi->specifics);
	
#ifdef __VMS
	free(device);
	free(directory);
	free(name);
#endif
	
	if ( nchars < 0 )
		return 1; /* Keep iterating, don't adjust msg_pos. */
	
	ctx->msg_pos += nchars;
	ctx->msg_rest_length -= nchars;
	if ( ctx->msg_rest_length < 0 )
		return 0; /* Stop iterating. */
	
	return 1; /* Keep iterating. */
}

/* ------------------------------------------------------------------------- */

/* This will print all of the debug info in the given stack, including
already-popped frames if dictated by the location of stack_start. */
char *skit_fstack_to_str_internal(
	const skit_thread_context *skit_thread_ctx,
	const skit_debug_fstack *stack,
	const skit_debug_stnode *stack_start)
{
	sASSERT_NO_TRACE(skit_thread_ctx != NULL);
	sASSERT_NO_TRACE(stack_start != NULL);
	skit_stack_to_str_context ctx;
	
	
	ctx.msg_pos = skit_thread_ctx->error_text_buffer;
	ctx.msg_rest_length = skit_thread_ctx->error_text_buffer_size;

	skit_debug_fstack_walk(stack, 
		&skit_stack_to_str_each, &ctx, stack_start, NULL);
	
	return skit_thread_ctx->error_text_buffer;
}

/* ------------------------------------------------------------------------- */

/* This is primarily used for printing stack traces that were saved much
earlier in execution when the stack trace would have looked much different.
*/
char *skit_stack_to_str_internal(
	const skit_thread_context *skit_thread_ctx,
	const skit_debug_stack  *stack,
	const skit_debug_stnode *stack_start)
{
	sASSERT_NO_TRACE(skit_thread_ctx != NULL);
	sASSERT_NO_TRACE(stack_start != NULL);
	skit_stack_to_str_context ctx;
	
	
	ctx.msg_pos = skit_thread_ctx->error_text_buffer;
	ctx.msg_rest_length = skit_thread_ctx->error_text_buffer_size;

	skit_debug_stack_walk(stack, 
		&skit_stack_to_str_each, &ctx, stack_start, NULL);
	
	return skit_thread_ctx->error_text_buffer;
}

/* ------------------------------------------------------------------------- */

static char skit_no_init_no_trace_msg[] = "No stack trace available because neither skit_init nor skit_thread_init were called.\n";

const char *skit_stack_trace_to_str_expr( uint32_t line, const char *file, const char *func, const char *specifics )
{
	char *result = NULL;
	if ( skit_thread_init_was_called() )
	{
		skit_thread_context *skit_thread_ctx = skit_thread_context_get();
		skit_frame_info *fi = skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc);
		skit_debug_info_store(fi, line, file, func, specifics);
		result = skit_fstack_to_str_internal(
			skit_thread_ctx, 
			&skit_thread_ctx->debug_info_stack,
			skit_thread_ctx->debug_info_stack.used.front);
		skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack);
	}
	else
	{
		result = skit_no_init_no_trace_msg;
	}
	return result;
}

/* ------------------------------------------------------------------------- */

/* TODO: stack trace printing should block signals. */
void skit_print_stack_trace_func( uint32_t line, const char *file, const char *func, const char *specifics )
{
	printf("Attempting to print stack trace.\n");
	const char *result = skit_stack_trace_to_str_expr(line,file,func,specifics);
	printf("Stack trace:\n%s\n", result);
}
