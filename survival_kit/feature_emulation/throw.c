
#include "survival_kit/feature_emulation/throw.h"
#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"

#include <stdarg.h>
#include <stdio.h>

#include "survival_kit/misc.h"

void skit_throw_exception(
	int line,
	const char *file,
	const char *func,
	skit_err_code etype,
	const char *fmtMsg,
	...)
{
	skit_thread_context *skit_thread_ctx = skit_thread_context_get();
	
	skit_exception *exc = skit_exc_fstack_alloc(&skit_thread_ctx->exc_instance_stack, &skit_malloc);
	skit_frame_info *fi = skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc);

	
	/* TODO: BUG: skit_error_text_buffer is global data.  Not thread safe and prevents more than one exception at a time from working. */
	va_list vl;
	va_start(vl, fmtMsg);
	vsnprintf(skit_error_text_buffer, SKIT_ERROR_BUFFER_SIZE, fmtMsg, vl);
	va_end(vl);
	
	exc->error_code = etype;
	exc->error_text = skit_error_text_buffer;
	
	SKIT_FEATURE_TRACE("%s, %d.136: THROW\n", file, line);
	skit_debug_info_store(fi, line, file, func);
	exc->frame_info_node = skit_thread_ctx->debug_info_stack.used.front;
	
	skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack);
	
	__PROPOGATE_THROWN_EXCEPTIONS;
}
