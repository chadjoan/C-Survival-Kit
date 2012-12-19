
#ifndef SKIT_THROW_INCLUDED
#define SKIT_THROW_INCLUDED

#include "survival_kit/macro.h"
#include "survival_kit/feature_emulation/compile_time_errors.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/feature_emulation/scope.h"
#include "survival_kit/feature_emulation/exceptions.h"

#if 0
/**
Throws the exception 'e'.
This is usually used with the new_exception function, and allows for
  exceptions to be created in one place and then thrown in another.
While this is the strictly more powerful way to throw exceptions, the more
  convenient way for common cases would be to use the RAISE macro.
This macro expands to a statement and may not be nested inside expressions.
Example usage:
	RAISE(new_exception(SKIT_EXCEPTION,"Something bad happened!")); 

TODO: regain this functionality.
*/
#endif

#if defined(THROW) && !defined(SKIT_ALLOW_OTHER_TRY_CATCH)
#undef THROW
#endif

/** 
Creates an exception of the type given and then throws it.
The single argument version accepts an exception type as the first argument
  and uses the default message for that exception type.
  (TODO: The single arg version is not implemented yet.)
The double (or more) argument version accepts an exception type as the first 
  argument and a message as its second argument.  The message may be a C
  format string with substitution values given in subsequent arguments.
This macro expands to a statement and may not be nested inside expressions.

The string arguments passed into this do not need to survive during any 
  sSCOPE_EXIT or sSCOPE_FAILURE statements operations that result from the
  throwing of the exception.  In other words: the sTHROW statement can be
  expected to copy its string argument into an internal buffer somewhere
  before the exception begins propogating.  This allows calling code to
  allocate strings that are passed into the sTHROW statement and then use
  a sSCOPE_EXIT statement to ensure that the strings are free'd.

It is named sTHROW instead of THROW because other libraries may define their
own exception handling with a macro by that name.  By default, any definitions
for THROW will be undefined by this file as a way to prevent a potentially
dangerous typo.  To allow the use of other THROW macros, #define the 
SKIT_ALLOW_OTHER_TRY_CATCH macro symbol before #including this file.
See "survival_kit/feature_emulation/try_catch.h" for more naming documentation.

Example usage:
	sTHROW(SKIT_EXCEPTION); // Use the exception type's default message.
	sTHROW(SKIT_EXCEPTION,"Something bad happened!"); // More convenient syntax.
	sTHROW(SKIT_EXCEPTION,"Bad index: %d", index);    // Formatting is allowed.
*/
#define sTHROW(...) SKIT_MACRO_DISPATCHER3(sTHROW, __VA_ARGS__)(__VA_ARGS__)

#define sTHROW1(etype) \
	do { \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
		skit_push_exception(skit_thread_ctx, __LINE__, __FILE__, __func__, etype); \
		__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
	} while(0)
	
#define sTHROW2(etype, emsg) \
	do { \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
		skit_push_exception(skit_thread_ctx, __LINE__, __FILE__, __func__, etype, emsg); \
		__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
	} while(0)

#define sTHROW3(etype, emsg, ...) \
	do { \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
		skit_push_exception(skit_thread_ctx, __LINE__, __FILE__, __func__, etype, emsg, __VA_ARGS__); \
		__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
	} while(0)
	
#define SKIT_NEW_EXCEPTION(...) SKIT_MACRO_DISPATCHER3(SKIT_NEW_EXCEPTION, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_NEW_EXCEPTION1(etype) \
	( \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1, \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY, \
		skit_new_exception(skit_thread_ctx, __LINE__, __FILE__, __func__, etype) \
	)
	
#define SKIT_NEW_EXCEPTION2(etype, emsg) \
	( \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1, \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY, \
		skit_new_exception(skit_thread_ctx, __LINE__, __FILE__, __func__, etype, emsg) \
	)

#define SKIT_NEW_EXCEPTION3(etype, emsg, ...) \
	( \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1, \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY, \
		skit_new_exception(skit_thread_ctx, __LINE__, __FILE__, __func__, etype, emsg, __VA_ARGS__) \
	)

#define SKIT_THROW_EXCEPTION(exc) \
	do { \
		SKIT_USE_FEATURES_IN_FUNC_BODY = 1; \
		(void)SKIT_USE_FEATURES_IN_FUNC_BODY; \
		skit_push_exception_obj(skit_thread_ctx, exc); \
		/* DO NOT use skit_exception_free. */ \
		/* skit_free is necessary instead. */ \
		/* skit_push_exception_obj now has a shallow copy. */ \
		/* The catch statement that will be encountered later will handle the rest of the resource cleanup. */ \
		skit_free(exc); \
		__SKIT_PROPOGATE_THROWN_EXCEPTIONS; \
	} while (0)

/* __SKIT_PROPOGATE_THROWN_EXCEPTIONS is an implementation detail.
// It does as the name suggests.  Do not call it from code that is not a part
// of this feature emulation module.  It may change in the future if needed
// to fix bugs or add new features.
NOTE: Do not expand this macro in any function besides the one with the
  scope guards that this will scan.  Scope guard scanning involves returning
  to the point where the scan was initiated (and one such point is in this
  exception propogation macro).  The method of return is a longjmp, and
  placing it in any function called by the function with the scope guards
  will cause a longjmp to an expired stack frame: bad news.
  See the __SKIT_SCAN_SCOPE_GUARDS macro documentation in 
  "survival_kit/feature_emulation/scope.h" for more details.
*/
#define __SKIT_PROPOGATE_THROWN_EXCEPTIONS /* */ \
	( \
		__SKIT_SCAN_SCOPE_GUARDS(SKIT_SCOPE_FAILURE_EXIT), \
		skit_propogate_exceptions(skit_thread_ctx), \
		1 \
	)

#if 0
#define __SKIT_PROPOGATE_THROWN_EXCEPTIONS /* */ \
	( \
		__SKIT_SCAN_SCOPE_GUARDS(SKIT_SCOPE_FAILURE_EXIT), \
		(__skit_propogate_exception_tmp = &(skit_thread_ctx->exc_instance_stack.used.front->val)), \
		SKIT_FEATURE_TRACE("%s, %d.95 in %s: __PROPOGATE\n", __FILE__, __LINE__,__func__), \
		/* SKIT_FEATURE_TRACE("frame_info_index: %li\n",__frame_info_end-1); */ \
		( skit_thread_ctx->exc_jmp_stack.used.length > 0 ) ? \
		( \
			SKIT_FEATURE_TRACE("%s, %d.99 in %s: __PROPOGATE longjmp.\n", __FILE__, __LINE__,__func__), \
			longjmp( \
				skit_thread_ctx->exc_jmp_stack.used.front->val, \
				__skit_propogate_exception_tmp->error_code), \
			1 \
		) : ( \
			SKIT_FEATURE_TRACE("%s, %d.106 in %s: __PROPOGATE failing.\n", __FILE__, __LINE__,__func__), \
			skit_print_exception(__skit_propogate_exception_tmp), /* TODO: this is probably a dynamic allocation and should be replaced by fprint_exception or something. */ \
			skit_die("Exception thrown with no handlers left in the stack."), \
			1 \
		), \
		1 \
	)
#endif

#endif
