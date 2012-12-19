
#ifdef __DECC
#pragma module skit_feature_emulation_funcs
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/misc.h" /* skit_die */
#include "survival_kit/memory.h"
#include "survival_kit/init.h"
#include "survival_kit/assert.h"

pthread_key_t skit_thread_context_key;

static void skit_thread_context_init( skit_thread_context *ctx )
{
	skit_jmp_fstack_init(&ctx->try_jmp_stack);
	skit_jmp_fstack_init(&ctx->exc_jmp_stack);
	skit_jmp_fstack_init(&ctx->scope_jmp_stack);
	skit_debug_fstack_init(&ctx->debug_info_stack);
	skit_exc_fstack_init(&ctx->exc_instance_stack);
	
	/* 16kB has GOT to be enough.  Riiight? */
	ctx->error_text_buffer_size = 16384;
	ctx->error_text_buffer = (char*)skit_malloc(ctx->error_text_buffer_size);
	if ( ctx->error_text_buffer == NULL )
		ctx->error_text_buffer_size = 0;
	
	/* TODO: BUG: This works amazingly well for how broken it is! */
	/*   The problem is that we setjmp and then return from the function. */
	/*   After that the main routine will call some other function that */
	/*   will almost assuredly take up residence in this functions stack */
	/*   space. */
	/*   Now, whenever we jump back to this place, the memory used to store */
	/*   state for this function (notable the '*ctx' argument) will be */
	/*   potentially ravaged.  */
	/*   This init function should probably be a MACRO that performs the */
	/*   setjmp inside the thread's entry function so that the stack space */
	/*   used here will for-sure be valid later on. */
	/*   For now, the undefined behavior seems to work quite well! */
	/*     *ducks and crosses fingers*      */
	if( setjmp( *skit_jmp_fstack_alloc(&ctx->exc_jmp_stack, &skit_malloc) ) != 0 )
	{
		/* jmp'ing from god-knows-where. */
		/* At least once the ctx->exc_instance_stack member was fubar'd once */
		/*   execution got to this point, so we should probably restore this */
		/*   context before proceeding.  Otherwise: random segfault time! */
		ctx = skit_thread_context_get();
	
		/* Uncaught exception(s)!  We're going down! */
		while ( ctx->exc_instance_stack.used.length > 0 )
		{
			fprintf(stderr,"Attempting to print exception:\n");
			skit_print_exception( skit_exc_fstack_pop(&ctx->exc_instance_stack) );
			fprintf(stderr,"\n");
		}
		skit_die("Uncaught exception(s).");
	}
}

static void skit_thread_context_dtor(void *ctx_ptr)
{
	skit_thread_context *ctx = (skit_thread_context*)ctx_ptr;
	(void)ctx;
	/* Do nothing for now. TODO: This will be important for multithreading. */
}

void skit_features_init()
{
	pthread_key_create(&skit_thread_context_key, skit_thread_context_dtor);
}

void skit_features_thread_init()
{
	skit_thread_context *ctx = (skit_thread_context*)skit_malloc(sizeof(skit_thread_context));
	skit_thread_context_init(ctx);
	pthread_setspecific(skit_thread_context_key, (void*)ctx);
}

skit_thread_context *skit_thread_context_get()
{
	return (skit_thread_context*)pthread_getspecific(skit_thread_context_key);
}

void skit_save_thread_context_pos( skit_thread_context *skit_thread_ctx, skit_thread_context_pos *pos )
{
	pos->try_jmp_pos    = skit_thread_ctx->try_jmp_stack.used.length;
	pos->exc_jmp_pos    = skit_thread_ctx->exc_jmp_stack.used.length;
	pos->scope_jmp_pos  = skit_thread_ctx->scope_jmp_stack.used.length;
	pos->debug_info_pos = skit_thread_ctx->debug_info_stack.used.length;
}

static void skit_fstack_reconcile_warn( ssize_t expected, ssize_t got, char *name )
{
	fprintf(stderr,"Warning: %s was unbalanced after the most recent return.\n", name);
	fprintf(stderr,"  Expected size: %li\n", expected );
	fprintf(stderr,"  Actual size:   %li\n", got );
	fprintf(stderr,"  This may mean that a goto, break, continue, or return was made while inside\n");
	fprintf(stderr,"  an sTRY-sCATCH block or a sSCOPE guard.  Jumping away from within a\n");
	fprintf(stderr,"  sTRY-sCATCH block or sSCOPE guard with raw C primitives can lead to \n");
	fprintf(stderr,"  very buggy, bizarre, and inconsistent runtime behavior.\n");
	fprintf(stderr,"  Just don't do it.\n");
	/* TODO: there should be some non-primitive control constructs that should be mentioned here
	 *   as a way of accomplishing desired logic in sTRY-sCATCH statements. */
	/* TODO: Although we can probably fix any problems the user creates for themselves, dieing might be better than warning. */
	
	printf("Printing stack trace:\n");
	printf("%s",skit_stack_trace_to_str());
}

#define SKIT_FSTACK_RECONCILE(stack, prev_length, name, name_str) \
	do { \
		if ( (stack).used.length > prev_length ) \
		{ \
			skit_fstack_reconcile_warn(prev_length, (stack).used.length, name_str); \
			while ( (stack).used.length > prev_length ) \
				name##_pop(&(stack)); \
		} \
		else if ( (stack).used.length < prev_length ) \
		{ \
			skit_fstack_reconcile_warn(prev_length, (stack).used.length, name_str); \
			skit_die("\nStack was popped too far!  This cannot be recovered.\n"); \
		} \
	} while (0)

void skit_reconcile_thread_context( skit_thread_context *ctx, skit_thread_context_pos *pos )
{
	SKIT_FSTACK_RECONCILE(ctx->try_jmp_stack,    pos->try_jmp_pos,    skit_jmp_fstack,   "try_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->exc_jmp_stack,    pos->exc_jmp_pos,    skit_jmp_fstack,   "exc_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->scope_jmp_stack,  pos->scope_jmp_pos,  skit_jmp_fstack,   "scope_jmp_stack");
	SKIT_FSTACK_RECONCILE(ctx->debug_info_stack, pos->debug_info_pos, skit_debug_fstack, "debug_info_stack");
}

void skit_debug_info_store( skit_frame_info *dst, int line, const char *file, const char *func )
{
	SKIT_FEATURE_TRACE("%s, %d: skit_debug_info_store(...,%d, %s, %s)\n", file, line, line, file, func);
	
	dst->line_number = line;
	dst->file_name = file;
	dst->func_name = func;
}

void skit_exception_free( skit_thread_context *skit_thread_ctx, skit_exception *exc )
{
	if ( exc->error_text != NULL )
	{
		skit_free(exc->error_text);
		exc->error_text = NULL;
	}
	exc->error_len = 0;
	exc->error_code = 0;
	exc->frame_info_node = NULL;
	
	/* Free any copies of debug stacks. */
	if ( exc->debug_info_stack != NULL &&
	     exc->debug_info_stack != &skit_thread_ctx->debug_info_stack.used )
	{
		skit_debug_stack *stack = exc->debug_info_stack;
		while ( stack->length > 0 )
			skit_free(skit_debug_stack_pop(stack));
		skit_free(stack);
	}
	exc->debug_info_stack = NULL;
}

static void skit_fill_exception(
	skit_thread_context *skit_thread_ctx,
	skit_exception *exc,
	int saveStack,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	va_list var_args)
{
	skit_frame_info *fi = skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc);

	int error_len = vsnprintf(
		skit_thread_ctx->error_text_buffer,
		skit_thread_ctx->error_text_buffer_size, fmtMsg, var_args);
	
	/* vsnprintf can return lengths greater than the buffer.  In these cases, truncation occured. */
	if ( error_len >= (skit_thread_ctx->error_text_buffer_size) )
		error_len = skit_thread_ctx->error_text_buffer_size - 1;
	
	/* TODO: it might be nice to have a pre-allocated thread-local buffer 
	for exception messages.  It would need to support multiple exception
	messages at once and grow if there is not enough space.  This would
	make it less likely that we do a dynamic allocation.  Not sure how
	important this is though, since out-of-memory errors would (ideally)
	cause the program to call skit_die, which would not allocate an
	exception.  OTOH, if we want OOM to be catchable, then this should 
	be considered or at least special-cased for OOM exceptions. */
	exc->error_code = etype;
	exc->error_text = (char*)skit_malloc(error_len+1); /* +1 to make room for the \0 at the end. */
	strcpy(exc->error_text, skit_thread_ctx->error_text_buffer);
	exc->error_len = error_len;
	
	SKIT_FEATURE_TRACE("%s, %d.136: sTHROW\n", file, line);
	skit_debug_info_store(fi, line, file, func);
	
	if ( saveStack )
	{
		/* Slower version used when the caller needs to accumulate exceptions
		that may have completely different debug stacks. */
		exc->debug_info_stack = skit_debug_stack_dup(&skit_thread_ctx->debug_info_stack.used);
	}
	else
	{
		/* Optimized path with less allocations/copies that can be used when
		the caller is going to unwind the stack. */
		exc->debug_info_stack = &skit_thread_ctx->debug_info_stack.used;
	}
	
	exc->frame_info_node = exc->debug_info_stack->front;
	
	skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack);
	
}

void skit_propogate_exceptions(skit_thread_context *skit_thread_ctx)
{
	skit_exception *exc = &(skit_thread_ctx->exc_instance_stack.used.front->val);
	SKIT_FEATURE_TRACE("%s, %d in %s: skit_propogate_exception\n", __FILE__, __LINE__,__func__);
	/* SKIT_FEATURE_TRACE("frame_info_index: %li\n",__frame_info_end-1); */
	if ( skit_thread_ctx->exc_jmp_stack.used.length > 0 )
	{
		SKIT_FEATURE_TRACE("%s, %d in %s: skit_propogate_exception longjmp.\n", __FILE__, __LINE__,__func__);
		longjmp(
			skit_thread_ctx->exc_jmp_stack.used.front->val,
			exc->error_code);
	}
	else
	{
		SKIT_FEATURE_TRACE("%s, %d in %s: skit_propogate_exception failing.\n", __FILE__, __LINE__,__func__);
		skit_print_exception(exc); /* TODO: this is probably a dynamic allocation and should be replaced by fprint_exception or something. */
		skit_die("Exception thrown with no handlers left in the stack.");
	}
}

void skit_throw_exception_no_ctx(
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...)
{
	skit_thread_context *skit_thread_ctx = skit_thread_context_get();
	skit_exception *exc = skit_exc_fstack_alloc(&skit_thread_ctx->exc_instance_stack, &skit_malloc);

	/* Forward var args to the real exception throwing function. */
	va_list vl;
	va_start(vl, fmtMsg);
	skit_fill_exception(
		skit_thread_ctx,
		exc,
		0, /* Reference existing stack trace info. */
		line,
		file,
		func,
		etype,
		fmtMsg,
		vl);
	va_end(vl);
	
	skit_propogate_exceptions(skit_thread_ctx);
}

void skit_push_exception_obj(skit_thread_context *skit_thread_ctx, skit_exception *exc)
{
	skit_exception *newb = skit_exc_fstack_alloc(&skit_thread_ctx->exc_instance_stack, &skit_malloc);
	memcpy(newb, exc, sizeof(skit_exception));
}

void skit_push_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...)
{
	skit_exception *exc = skit_exc_fstack_alloc(&skit_thread_ctx->exc_instance_stack, &skit_malloc);

	/* Forward the debug info to the exception filling function. */
	va_list vl;
	va_start(vl, fmtMsg);
	skit_fill_exception(
		skit_thread_ctx,
		exc,
		0, /* Reference existing stack trace info. */
		line,
		file,
		func,
		etype,
		fmtMsg,
		vl);
	va_end(vl);
}

skit_exception *skit_new_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...)
{
	skit_exception *exc = skit_malloc(sizeof(skit_exception));

	/* Forward the debug info to the exception filling function. */
	va_list vl;
	va_start(vl, fmtMsg);
	skit_fill_exception(
		skit_thread_ctx,
		exc,
		1, /* Save the stack trace in newly allocated memory. */
		line,
		file,
		func,
		etype,
		fmtMsg,
		vl);
	va_end(vl);
	
	return exc;
}

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
		"%s: at line %d in function %s\r\n",
		name,
		fi->line_number,
		fi->func_name);
	
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

/* This will print all of the debug info in the given stack, including
already-popped frames if dictated by the location of stack_start. */
static char *skit_fstack_to_str_internal(
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

/* This is primarily used for printing stack traces that were saved much
earlier in execution when the stack trace would have looked much different.
*/
static char *skit_stack_to_str_internal(
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

void skit_print_exception(skit_exception *e)
{
	sASSERT(skit_thread_init_was_called());
	skit_thread_context *skit_thread_ctx = skit_thread_context_get();
	if ( e->error_text != NULL )
		printf("%s\n",e->error_text);
	else
		printf("skit internal error: Exception error text is NULL!\n");
	
	if ( e->debug_info_stack == NULL || 
	     e->debug_info_stack == &skit_thread_ctx->debug_info_stack.used )
	{
		/*
		The exception's debug info is on the current stack.
		Trace that one, including things in stack frames that have already unwound.
		(This is why we use an fstack walk instead of a stack walk.)
		*/
		printf("%s\n",skit_fstack_to_str_internal(
			skit_thread_ctx,
			&skit_thread_ctx->debug_info_stack,
			e->frame_info_node));
	}
	else
	{
		/* Print a stack saved in dynamic memory by the exception. */
		printf("%s\n",skit_stack_to_str_internal(
			skit_thread_ctx,
			e->debug_info_stack,
			e->frame_info_node));
	}
}

static char skit_no_init_no_trace_msg[] = "No stack trace available because neither skit_init nor skit_thread_init were called.\n";

const char *skit_stack_trace_to_str_expr( uint32_t line, const char *file, const char *func )
{
	char *result = NULL;
	if ( skit_thread_init_was_called() )
	{
		skit_thread_context *skit_thread_ctx = skit_thread_context_get();
		skit_frame_info *fi = skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc);
		skit_debug_info_store(fi, line, file, func);
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

/* TODO: stack trace printing should block signals. */
void skit_print_stack_trace_func( uint32_t line, const char *file, const char *func )
{
	printf("Attempting to print stack trace.\n");
	const char *result = skit_stack_trace_to_str_expr(line,file,func);
	printf("Stack trace:\n%s\n", result);
}