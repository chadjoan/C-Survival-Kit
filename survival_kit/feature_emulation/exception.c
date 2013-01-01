
#ifdef __DECC
#pragma module skit_feature_emu_exception
#endif

#include <inttypes.h>
#include <assert.h>
#include <stdio.h> /* printf */

#include "survival_kit/memory.h"
#include "survival_kit/init.h"
#include "survival_kit/feature_emulation/thread_context.h"
#include "survival_kit/feature_emulation/exception.h"
#include "survival_kit/feature_emulation/frame_info.h"
#include "survival_kit/feature_emulation/stack_trace.h"
#include "survival_kit/feature_emulation/debug.h"

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_NAME exc
#include "survival_kit/templates/stack.c"
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_DIE_ON_ERROR

skit_err_code SKIT_EXCEPTION       = SKIT_EXCEPTION_INITIAL_VAL;
skit_err_code SKIT_FATAL_EXCEPTION = SKIT_EXCEPTION_INITIAL_VAL;
skit_err_code SKIT_BREAK_IN_TRY_CATCH;
skit_err_code SKIT_CONTINUE_IN_TRY_CATCH;
skit_err_code SKIT_SOCKET_EXCEPTION;
skit_err_code SKIT_OUT_OF_BOUNDS;
skit_err_code SKIT_NOT_IMPLEMENTED;
skit_err_code SKIT_IO_EXCEPTION;
skit_err_code SKIT_FILE_IO_EXCEPTION;
skit_err_code SKIT_TCP_IO_EXCEPTION;
skit_err_code SKIT_FILE_NOT_FOUND;

static int skit_exceptions_initialized = 0;

void skit_init_exceptions()
{
	if ( skit_exceptions_initialized )
		return;
	
	/* Make sure the first exception ever is the unrecoverable one. */
	/* That way, if 0 or NULL is thrown, it will cause a fatal exception and halt the program. */	
	/* This can happen if someone forgets to register an exception but creates a variable for it */
	/*   and then throws that variable.  This could wreak havoc, so crashing immediately is a */
	/*   relatively forgiving outcome. */
	SKIT_REGISTER_EXCEPTION(SKIT_FATAL_EXCEPTION,       SKIT_FATAL_EXCEPTION,   "An unrecoverable exception was thrown.");
	SKIT_REGISTER_EXCEPTION(SKIT_EXCEPTION,             SKIT_EXCEPTION,         "An exception was thrown.");
	SKIT_REGISTER_EXCEPTION(SKIT_BREAK_IN_TRY_CATCH,    SKIT_EXCEPTION,         "Attempt to use the built-in 'break' statement while in an sTRY-sCATCH block.");
	SKIT_REGISTER_EXCEPTION(SKIT_CONTINUE_IN_TRY_CATCH, SKIT_EXCEPTION,         "Attempt to use the built-in 'continue' statement while in an sTRY-sCATCH block.");
	SKIT_REGISTER_EXCEPTION(SKIT_SOCKET_EXCEPTION,      SKIT_EXCEPTION,         "socket exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_OUT_OF_BOUNDS,         SKIT_EXCEPTION,         "Out of bounds exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_NOT_IMPLEMENTED,       SKIT_EXCEPTION,         "The requested feature is not implemented.");
	SKIT_REGISTER_EXCEPTION(SKIT_IO_EXCEPTION,          SKIT_EXCEPTION,         "Generic I/O exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_FILE_IO_EXCEPTION,     SKIT_IO_EXCEPTION,      "File I/O exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_TCP_IO_EXCEPTION,      SKIT_IO_EXCEPTION,      "TCP Network I/O exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_FILE_NOT_FOUND,        SKIT_FILE_IO_EXCEPTION, "File not found.");

	skit_exceptions_initialized = 1;
}

static skit_err_code *skit__exc_inheritance_table = NULL;
static size_t skit__exc_table_size = 0;

void skit__register_exception( skit_err_code *ecode, const skit_err_code *parent, const char *ecode_name, const char *default_msg )
{
	/* 
	Note that parent is passed by reference and not value.
	This is to ensure that cases like 
	  SKIT_REGISTER_EXCEPTION(SKIT_EXCEPTION, SKIT_EXCEPTION, "...")
	will actually do what is expected: make the exception inherit from itself.
	If parent were passed by value, then the above expression might fill the
	'parent' argument with whatever random garbage the compiler left in the
	initial value of SKIT_EXCEPTION.  *ecode = ...; will assign the original
	variable a sane value, but the parent value in the table will still end
	up being the undefined garbage value.  By passing parent as a pointer we
	ensure that *ecode = ...; will also populate the parent value in cases
	where an exception is defined as its own parent (root-level exceptions). */
	
	assert(ecode != NULL);
	assert(parent != NULL);
	assert(ecode_name != NULL);
	assert(default_msg != NULL);
	
	*ecode = skit__exc_table_size;
	
	if ( skit__exc_inheritance_table == NULL )
	{
		/* If this doesn't already exist, create the table and give it 1 element. */
		skit__exc_inheritance_table = skit_malloc(sizeof(skit_err_code));
		skit__exc_table_size = 1;
	}
	else
	{
		/* If this does exist, it will need to grow by one element. */
		skit__exc_table_size += 1;
		skit__exc_inheritance_table = skit_realloc(
			skit__exc_inheritance_table,
			sizeof(skit_err_code) * skit__exc_table_size);
	}
	
	skit__exc_inheritance_table[*ecode] = *parent;
	/* printf("Defined %s as %ld\n", ecode_name, *ecode); */
	/* TODO: do something with the ecode_name and default_msg. */
}

/* ------------------------------------------------------------------------- */

int skit_exception_is_a( skit_err_code ecode1, skit_err_code ecode2 )
{
	skit_err_code child_code = ecode2;
	skit_err_code parent_code = ecode1;
	skit_err_code last_parent = 0;
	while (1)
	{
		/* Invalid parent_codes. */
		if ( parent_code >= skit__exc_table_size )
			assert(0);
		
		/* Match found! */
		if ( child_code == parent_code )
			return 1;

		/* We've hit a root-level exception type without establishing a parent-child relationship. No good. */
		if ( last_parent == parent_code )
			return 0;
		
		last_parent = parent_code;
		parent_code = skit__exc_inheritance_table[parent_code];
	}
	assert(0);
}

/* ------------------------------------------------------------------------- */

void skit_exception_dtor( skit_thread_context *skit_thread_ctx, skit_exception *exc )
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

/* ------------------------------------------------------------------------- */

void skit_propogate_exceptions(skit_thread_context *skit_thread_ctx, int line, const char *file, const char *func)
{
	skit_exception *exc = &(skit_thread_ctx->exc_instance_stack.used.front->val);
	SKIT_FEATURE_TRACE("%s, %d in %s: skit_propogate_exception\n", __FILE__, __LINE__,__func__);
	/* SKIT_FEATURE_TRACE("frame_info_index: %li\n",__frame_info_end-1); */
	if ( skit_thread_ctx->exc_jmp_stack.used.length > 0 )
	{
		SKIT_FEATURE_TRACE("%s, %d in %s: skit_propogate_exception longjmp.\n", __FILE__, __LINE__,__func__);
		SKIT_THREAD_UNWIND_EVENT_LFF(skit_thread_ctx, line, file, func);
		longjmp(
			skit_thread_ctx->exc_jmp_stack.used.front->val,
			exc->error_code);
	}
	else
	{
		/* This code path shouldn't really happen. */
		/* The code establishing the thread context will add an extra setjmp */
		/*   that is used for handling exceptions. */
		SKIT_FEATURE_TRACE("%s, %d in %s: skit_propogate_exception failing.\n", __FILE__, __LINE__,__func__);
		skit_print_exception(exc); /* TODO: this is probably a dynamic allocation and should be replaced by fprint_exception or something. */
		skit_die("Exception thrown with no handlers left in the stack.");
	}
}

/* ------------------------------------------------------------------------- */

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
	exc->error_code = etype;
	
	if ( skit_thread_ctx == NULL )
	{
		/* Slightly hackier path requiring dynamic allocations and magic values. */
		/* This exists so that SKIT_NEW_EXCEPTION() can be called without having */
		/*   a thread context available. */
		const int error_buffer_size = 1024; /* We have to guess this size because we don't have a buffer from context to experiment on. */
		char *error_text_buffer = skit_malloc(error_buffer_size);
		int error_len = vsnprintf(
			error_text_buffer,
			error_buffer_size, fmtMsg, var_args);
		
		/* vsnprintf can return lengths greater than the buffer.  In these cases, truncation occured. */
		if ( error_len >= error_buffer_size )
			error_len = error_buffer_size - 1;
		
		exc->error_text = error_text_buffer;
		exc->error_len  = error_len;
		
		exc->debug_info_stack = skit_malloc(sizeof(skit_debug_stack));
		skit_debug_stack_init(exc->debug_info_stack);
		
		skit_debug_stnode *fi = skit_malloc(sizeof(skit_debug_stnode));
		skit_debug_info_store(&fi->val, line, file, func, " <- exception happened here.");
		skit_debug_stack_push(exc->debug_info_stack, fi);
		
		exc->frame_info_node = exc->debug_info_stack->front;
	}
	else /* More desirable path that uses buffers in the thread context. */
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
		exc->error_text = (char*)skit_malloc(error_len+1); /* +1 to make room for the \0 at the end. */
		strcpy(exc->error_text, skit_thread_ctx->error_text_buffer);
		exc->error_len = error_len;
		
		SKIT_FEATURE_TRACE("%s, %d.136: sTHROW\n", file, line);
		skit_debug_info_store(fi, line, file, func, " <- exception happened here.");
		
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
	
}

/* ------------------------------------------------------------------------- */

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
	
	skit_propogate_exceptions(skit_thread_ctx, line, file, func);
}

/* ------------------------------------------------------------------------- */

void skit_push_exception_obj(skit_thread_context *skit_thread_ctx, skit_exception *exc)
{
	skit_exception *newb = skit_exc_fstack_alloc(&skit_thread_ctx->exc_instance_stack, &skit_malloc);
	memcpy(newb, exc, sizeof(skit_exception));
}

/* ------------------------------------------------------------------------- */

void skit_push_exception_va(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	va_list var_args)
{
	skit_exception *exc = skit_exc_fstack_alloc(&skit_thread_ctx->exc_instance_stack, &skit_malloc);

	/* Forward the debug info to the exception filling function. */
	skit_fill_exception(
		skit_thread_ctx,
		exc,
		0, /* Reference existing stack trace info. */
		line,
		file,
		func,
		etype,
		fmtMsg,
		var_args);
}

/* ------------------------------------------------------------------------- */

void skit_push_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...)
{
	/* Expand variadic arguments and forward. */
	va_list vl;
	va_start(vl, fmtMsg);
	skit_push_exception_va(
		skit_thread_ctx,
		line,
		file,
		func,
		etype,
		fmtMsg,
		vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

skit_exception *skit_new_exception_va(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	va_list var_args)
{
	skit_exception *exc = skit_malloc(sizeof(skit_exception));

	/* Forward the debug info to the exception filling function. */
	skit_fill_exception(
		skit_thread_ctx,
		exc,
		1, /* Save the stack trace in newly allocated memory. */
		line,
		file,
		func,
		etype,
		fmtMsg,
		var_args);
	
	return exc;
}

/* ------------------------------------------------------------------------- */

skit_exception *skit_new_exception(
	skit_thread_context *skit_thread_ctx,
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...)
{
	/* Expand variadic arguments and forward. */
	va_list vl;
	va_start(vl, fmtMsg);
	skit_exception *exc = skit_new_exception_va(
		skit_thread_ctx,
		line,
		file,
		func,
		etype,
		fmtMsg,
		vl);
	va_end(vl);
	
	return exc;
}

/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */

void skit_print_uncaught_exceptions(skit_thread_context *skit_thread_ctx)
{
	while ( skit_thread_ctx->exc_instance_stack.used.length > 0 )
	{
		fprintf(stderr,"Attempting to print exception:\n");
		skit_print_exception( skit_exc_fstack_pop(&skit_thread_ctx->exc_instance_stack) );
		fprintf(stderr,"\n");
	}
}