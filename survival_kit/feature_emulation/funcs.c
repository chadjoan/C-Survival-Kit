#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/misc.h"
#include "survival_kit/assert.h"

pthread_key_t skit_thread_context_key;

static void skit_thread_context_init( skit_thread_context *ctx )
{
	skit_jmp_fstack_init(&ctx->try_jmp_stack);
	skit_jmp_fstack_init(&ctx->exc_jmp_stack);
	skit_jmp_fstack_init(&ctx->scope_jmp_stack);
	skit_debug_fstack_init(&ctx->debug_info_stack);
	skit_exc_fstack_init(&ctx->exc_instance_stack);
	
	if( setjmp( *skit_jmp_fstack_alloc(&ctx->exc_jmp_stack, &skit_malloc) ) != 0 )
	{
		/* Uncaught exception(s)!  We're going down! */
		while ( ctx->exc_instance_stack.used.length > 0 )
		{
			skit_print_exception( skit_exc_fstack_pop(&ctx->exc_instance_stack) );
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
	fprintf(stderr,"  a TRY-CATCH block or a SCOPE guard.  Jumping away from within a TRY-CATCH\n");
	fprintf(stderr,"  block or SCOPE guard with raw C primitives can lead to very buggy, bizarre,\n");
	fprintf(stderr,"  and inconsistent runtime behavior.  Just don't do it.\n");
	/* TODO: there should be some non-primitive control constructs that should be mentioned here
	 *   as a way of accomplishing desired logic in TRY-CATCH statements. */
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

/* TODO: use skit_malloc instead of global/static stuff. */
#define MSG_BUF_SIZE 32768
static char msg_buf[MSG_BUF_SIZE];
static char *skit_stack_to_str_internal(const skit_debug_stnode *stack_end)
{
	char *msg_pos = msg_buf;
	ssize_t msg_rest_length = MSG_BUF_SIZE;
	
	/* FIRST: scan the unused stack for stack_end. */
	/*   if it's there, then that will need to be printed backwards. */
	/* SECOND: print the used stack.  If stack_end wasn't in unused land, then only print from stack_end. */
	
	while ( stack_end != NULL )
	{
		const skit_frame_info *fi = &stack_end->val;
		
#ifdef __VMS
		char *device;
		char *directory;
		char *name;
		best_effort_vms_path_parse(fi.file_name, &device, &directory, &name);
#elif defined(__linux__)
		const char *name = fi->file_name;
#else
#error "Unsupported target.  This code needs porting."
#endif
		
		ssize_t nchars = snprintf(
			msg_pos, msg_rest_length,
			"%s: at line %d in function %s\r\n",
			name,
			fi->line_number,
			fi->func_name);
		
#ifdef __VMS
		free(device);
		free(directory);
		free(name);
#endif
		stack_end = stack_end->next;
		
		if ( nchars < 0 )
			continue;
		
		msg_pos += nchars;
		msg_rest_length -= nchars;
		if ( msg_rest_length < 0 )
			break;
	}
	
	return msg_buf;
}

void skit_print_exception(skit_exception *e)
{
	printf("%s\n",e->error_text);
	printf("%s\n",skit_stack_to_str_internal(e->frame_info_node));
}

char *skit_stack_trace_to_str_expr( uint32_t line, const char *file, const char *func )
{
	skit_thread_context *skit_thread_ctx = skit_thread_context_get();
	skit_frame_info *fi = skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc);
	skit_debug_info_store(fi, line, file, func);
	char *result = skit_stack_to_str_internal(skit_thread_ctx->debug_info_stack.used.front);
	skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack);
	return result;
}
