
#ifndef SKIT_THROW_INCLUDED
#define SKIT_THROW_INCLUDED

#include "survival_kit/macro.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/generated_exception_defs.h"

#if 0
/**
Throws the exception 'e'.
This is usually used with the new_exception function, and allows for
  exceptions to be created in one place and then thrown in another.
While this is the strictly more powerful way to throw exceptions, the more
  convenient way for common cases would be to use the RAISE macro.
This macro expands to a statement and may not be nested inside expressions.
Example usage:
	RAISE(new_exception(GENERIC_EXCEPTION,"Something bad happened!")); 
*/
#endif




#if 0
#define __SKIT_RAISE(e) \
	do { \
		SKIT_FEATURE_TRACE("%s, %d.136: THROW\n", __FILE__, __LINE__); \
		(e); \
		if ( skit_thread_ctx->exc_instance_stack.used.length <= 0 ) \
			skit_new_exception(skit_thread_ctx, -1, "NULL was thrown."); \
		skit_debug_fstack_alloc(&skit_thread_ctx->debug_info_stack, &skit_malloc); \
		skit_debug_info_store(&skit_thread_ctx->debug_info_stack.used.front.val, \
			__LINE__,__FILE__,__func__); \
		skit_thread_ctx->exc_instance_stack.used.front.val.frame_info \
			= &skit_thread_ctx->debug_info_stack.used.front.val; \
		skit_debug_fstack_pop(&skit_thread_ctx->debug_info_stack); \
		__PROPOGATE_THROWN_EXCEPTIONS; \
	} while (0)
#endif

/** 
Creates an exception of the type given and then throws it.
This is a more concise variant of the THROW macro.
The single argument version accepts an exception type as the first argument
  and uses the default message for that exception type.
  (TODO: The single arg version is not implemented yet.)
The double (or more) argument version accepts an exception type as the first 
  argument and a message as its second argument.  The message may be a C
  format string with substitution values given in subsequent arguments.
This macro expands to a statement and may not be nested inside expressions.
Example usage:
	THROW(GENERIC_EXCEPTION); // Use the exception type's default message.
	THROW(GENERIC_EXCEPTION,"Something bad happened!"); // More convenient syntax.
	THROW(GENERIC_EXCEPTION,"Bad index: %d", index);    // Formatting is allowed.
*/
#define THROW(...) MACRO_DISPATCHER3(THROW, __VA_ARGS__)(__VA_ARGS__)

#define THROW1(e) \
		skit_throw_exception(skit_thread_ctx, skit_scope_ctx, __LINE__, __FILE__, __func__, etype)
	
#define THROW2(etype, emsg) \
		skit_throw_exception(skit_thread_ctx, skit_scope_ctx, __LINE__, __FILE__, __func__, etype, emsg)

#define THROW3(etype, emsg, ...) \
		skit_throw_exception(skit_thread_ctx, skit_scope_ctx, __LINE__, __FILE__, __func__, etype, emsg, __VA_ARGS__)

/* __PROPOGATE_THROWN_EXCEPTIONS is an implementation detail.
// It does as the name suggests.  Do not call it from code that is not a part
// of this exception handling module.  It may change in the future if needed
// to fix bugs or add new features.
*/
/* TODO: BUG?  __SKIT_SCAN_SCOPE_GUARDS should probably happen when the exception is caught (or uncaught but unwinds the stack).
   As it is, functions not called with the CALL statement might be able to throw and have the exception jump past the scope guarding logic. */
#define __PROPOGATE_THROWN_EXCEPTIONS /* */ \
	do { \
		__SKIT_SCAN_SCOPE_GUARDS; \
		skit_exception *exc = &(skit_thread_ctx->exc_instance_stack.used.front->val); \
		SKIT_FEATURE_TRACE("%s, %d.117: __PROPOGATE\n", __FILE__, __LINE__); \
		/* SKIT_FEATURE_TRACE("frame_info_index: %li\n",__frame_info_end-1); */ \
		if ( skit_thread_ctx->exc_jmp_stack.used.length > 0 ) \
			longjmp( \
				skit_thread_ctx->exc_jmp_stack.used.front->val, \
				exc->error_code); \
		else \
		{ \
			skit_print_exception(exc); /* TODO: this is probably a dynamic allocation and should be replaced by fprint_exception or something. */ \
			skit_die("Exception thrown with no handlers left in the stack."); \
		} \
	} while (0)
	
#endif
