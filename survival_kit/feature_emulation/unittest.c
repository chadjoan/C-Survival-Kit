
#ifdef __DECC
#pragma module skit_feature_emu_unittests
#endif

#include "survival_kit/feature_emulation/unittest.h"
#include "survival_kit/feature_emulation.h"
#include <assert.h>

static skit_err_code SKIT_EXCEPTION_UTEST = SKIT_EXCEPTION_INITIAL_VAL;

/* Unittesting functions. */
static int baz()
{
	printf("Baz doesn't throw.\n");
	printf("It does print a stack trace before returning, though:\n");
	printf("%s",skit_stack_trace_to_str());
	printf("\n");
	return 0;
}

static float qux()
{
	SKIT_USE_FEATURE_EMULATION;
	sTHROW(SKIT_EXCEPTION,"qux does throw!\n");
	assert(0);
	return 0.0;
}

static void bar(int val1, int val2)
{
	assert(0);
}

static int foo()
{
	SKIT_USE_FEATURE_EMULATION;
	sTRACE(bar(baz(), qux()));
	assert(0);
	return 42;
}

static int foo2()
{
	SKIT_USE_FEATURE_EMULATION;
	bar(baz(), sETRACE(qux()));
	/* TODO: bar(sETRACE(baz()), sETRACE(qux())) without compiler warnings. */
	assert(0);
	return 42;
}

static void unittest_exceptions()
{
	SKIT_USE_FEATURE_EMULATION;
	int pass;
	
	if( !skit_exception_is_a(SKIT_EXCEPTION,          SKIT_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if( !skit_exception_is_a(SKIT_BREAK_IN_TRY_CATCH, SKIT_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if(  skit_exception_is_a(SKIT_FATAL_EXCEPTION,    SKIT_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	
	SKIT_EXCEPTION_UTEST = SKIT_EXCEPTION_INITIAL_VAL;
	SKIT_REGISTER_EXCEPTION(SKIT_EXCEPTION_UTEST, SKIT_EXCEPTION, "Exceptions unittest failure.");
	
	sTRY
		sTRACE(baz());
		printf("No exception thrown.\n");
	sCATCH(SKIT_EXCEPTION, e)
		skit_print_exception(e);
		assert(0);
	sEND_TRY
	
	printf("------\n");
	printf("Exception pattern matching.\n");
	pass = 0;
	
	sTRY
		sTHROW(SKIT_EXCEPTION_UTEST, "SKIT_EXCEPTION_UTEST");
		assert(0);
	sCATCH(SKIT_EXCEPTION_UTEST, e)
		pass = 1;
	sCATCH(SKIT_EXCEPTION, e)
		assert(0);
	sEND_TRY
	
	assert(pass);
	
	pass = 0;
	sTRY
		sTHROW(SKIT_EXCEPTION_UTEST, "SKIT_EXCEPTION_UTEST");
		assert(0);
	sCATCH(SKIT_EXCEPTION, e)
		pass = 1;
	sCATCH(SKIT_EXCEPTION_UTEST, e)
		assert(0);
	sEND_TRY
	
	assert(pass);
	printf("Passed.\n\n");
	
	printf("------\n");
	
	sTRY
		sTRY
			printf("Nested sTRY-sTRY-sCATCH test.\n");
		sCATCH(SKIT_EXCEPTION, e)
			skit_print_exception(e);
			assert(0);
		sEND_TRY
		
		sTRACE(foo());
		assert(0);

	sCATCH(SKIT_EXCEPTION, e)
		sTRY
			printf("Nested sCATCH-sTRY-sCATCH test.\n");
		sCATCH(SKIT_EXCEPTION, e2)
			assert(0);
			/* skit_print_exception(e2);*/
		sEND_TRY
		
		skit_print_exception(e);
	sEND_TRY
	
	printf("------\n");
	printf("Testing sETRACE(...):\n");
	
	sTRY
		sTRACE(foo2());
		assert(0);
	sCATCH(SKIT_EXCEPTION, e)
		skit_print_exception(e);
	sEND_TRY
	
	printf("------\n");
	
	sTRY
		while(1)
		{
			sTRY
				printf("I'm in a loop.\n");
				printf("Now to bail without stopping first!\n");
				break;
			sCATCH(SKIT_EXCEPTION,e)
				assert(0);
			sEND_TRY
		}
	sCATCH(SKIT_BREAK_IN_TRY_CATCH, e)
		skit_print_exception(e);
	sEND_TRY
	
	
	sTRY
		while(1)
		{
			sTRY
				printf("I'm in a loop (again).\n");
				printf("Now to bail without stopping first!\n");
				continue;
			sCATCH(SKIT_EXCEPTION,e)
				assert(0);
			sEND_TRY
		}
	sCATCH(SKIT_CONTINUE_IN_TRY_CATCH, e)
		skit_print_exception(e);
	sEND_TRY
	
	int etrace_test = sETRACE(42);
	sASSERT_EQ(etrace_test, 42, "%d");
	
	printf("  exception_handling unittest passed!\n");
}

static void unittest_scope_fn_that_throws()
{
	SKIT_USE_FEATURE_EMULATION;
	sTHROW(SKIT_EXCEPTION,"Testing scope statements: this should never print.\n");
}

/* All scope unittest subfunctions should return 0. */
static void unittest_scope_exit_normal(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 2;
	sSCOPE_EXIT((*result)--);
	sSCOPE_EXIT_BEGIN
		(*result)--;
	sEND_SCOPE_EXIT
sEND_SCOPE

static void unittest_scope_exit_exceptional(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 2;
	sSCOPE_EXIT((*result)--);
	sSCOPE_EXIT_BEGIN
		(*result)--;
	sEND_SCOPE_EXIT
	unittest_scope_fn_that_throws();
sEND_SCOPE

static void unittest_scope_exit_exceptional_call(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 2;
	sSCOPE_EXIT((*result)--);
	sSCOPE_EXIT_BEGIN
		(*result)--;
	sEND_SCOPE_EXIT
	sTRACE(unittest_scope_fn_that_throws());
sEND_SCOPE

static void unittest_scope_failure_normal(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 0;
	sSCOPE_FAILURE((*result)--);
	sSCOPE_FAILURE_BEGIN
		(*result)--;
	sEND_SCOPE_FAILURE
sEND_SCOPE

static void unittest_scope_failure_exceptional(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 2;
	sSCOPE_FAILURE((*result)--);
	sSCOPE_FAILURE_BEGIN
		(*result)--;
	sEND_SCOPE_FAILURE
	sTHROW(SKIT_EXCEPTION,"Testing scope statements: this should never print.\n");
sEND_SCOPE

static void unittest_scope_success_normal(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 2;
	sSCOPE_SUCCESS((*result)--);
	sSCOPE_SUCCESS_BEGIN
		(*result)--;
	sEND_SCOPE_SUCCESS
sEND_SCOPE

static void unittest_scope_success_exceptional(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 0;
	sSCOPE_SUCCESS((*result)--);
	sSCOPE_SUCCESS_BEGIN
		(*result)--;
	sEND_SCOPE_SUCCESS
	sTHROW(SKIT_EXCEPTION,"Testing scope statements: this should never print.\n");
sEND_SCOPE

static int unittest_scope_exit_sRETURN(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 3;
	sSCOPE_EXIT((*result)--);
	sSCOPE_EXIT_BEGIN
		(*result)--;
	sEND_SCOPE_EXIT
	
	/* Make sure sRETURN expands correctly next to if-statements. */
	if ( 0 )
		sRETURN(0);
	
	(*result)--;
	
	sRETURN(0);
	
	sTHROW(SKIT_EXCEPTION,"Testing scope statements: this should never print.\n");
	
sEND_SCOPE

static int skit_scope_layered_try_test(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 3;
	sSCOPE_EXIT((*result)--);
	
	sTRY
		(*result)--;
		sTHROW(SKIT_EXCEPTION, "  Thrown from a try-catch inside a scope-end_scope.");
		(*result)--;
	sCATCH(SKIT_EXCEPTION, e)
		printf("  Printing an exception, just as planned.\n");
		skit_print_exception(e);
		printf("  Printed!\n");
		(*result)--;
	sEND_TRY
	
	sRETURN(0);
sEND_SCOPE

static int skit_scope_layered_try_test2(int *result)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	*result = 2;
	sSCOPE_EXIT((*result)--);
	
	/* Try throwing an uncaught exception from within try-catch, not just function scope. */
	/* Will it execute the sSCOPE_EXIT AND get caught later? It should. */
	sTRY
		(*result)--;
		sTHROW(SKIT_EXCEPTION, "  Thrown from a try-catch inside a scope-end_scope.");
		(*result)--;
	sEND_TRY
	
	(*result)--;
	sASSERT_MSG(0, "This should be unreachable.");
	sRETURN(0);
sEND_SCOPE

static void unittest_scope()
{
	SKIT_USE_FEATURE_EMULATION;
	int val = 1;
	
	unittest_scope_exit_normal(&val);
	sASSERT_EQ(val,0,"%d");
	
	unittest_scope_success_normal(&val);
	sASSERT_EQ(val,0,"%d");
	
	unittest_scope_failure_normal(&val);
	sASSERT_EQ(val,0,"%d");
	
	int ret = unittest_scope_exit_sRETURN(&val);
	sASSERT_EQ(val,0,"%d");
	sASSERT_EQ(ret,0,"%d");
	
	sTRY
		unittest_scope_exit_exceptional(&val);
	sCATCH(SKIT_EXCEPTION,e)
		sASSERT_EQ(val,0,"%d");
	sEND_TRY
	
	sTRY
		unittest_scope_exit_exceptional_call(&val);
	sCATCH(SKIT_EXCEPTION,e)
		sASSERT_EQ(val,0,"%d");
	sEND_TRY
	
	sTRY
		unittest_scope_success_exceptional(&val);
	sCATCH(SKIT_EXCEPTION,e)
		sASSERT_EQ(val,0,"%d");
	sEND_TRY
	
	sTRY
		unittest_scope_failure_exceptional(&val);
	sCATCH(SKIT_EXCEPTION,e)
		sASSERT_EQ(val,0,"%d");
	sEND_TRY
	
	sTRY
		skit_scope_layered_try_test(&val);
		sASSERT_EQ(val,0,"%d");
	sCATCH(SKIT_EXCEPTION,e)
		sASSERT_MSG(0, "This should be unreachable.");
	sEND_TRY
	
	sTRY
		skit_scope_layered_try_test2(&val);
	sCATCH(SKIT_EXCEPTION,e)
		sASSERT_EQ(val,0,"%d");
	sEND_TRY

	printf("  scope unittest passed!\n");
	
	/* sRETURN();*/ /* This should compile.  It tests for zero-arg sRETURN functionality. */
	/* It is currently broken though, pending some fairly complicated macro trickery. */
	/* For now, use this slightly uglier version instead: */
	sRETURN_;
}

void skit_unittest_features()
{
	/* goto gets redefined in compile_time_errors.h.  
	It's used uncommonly enough that we should make sure it
	still compiles in places where it should compile. */
	goto foolabel;
	foolabel:
	
	printf("skit_unittest_features()\n");
	unittest_exceptions();
	unittest_scope();
	printf("  features unittest passed!\n");
	printf("\n");
}
/* End unittests. */