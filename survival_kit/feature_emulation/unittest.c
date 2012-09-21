
#include "survival_kit/feature_emulation/unittest.h"
#include "survival_kit/feature_emulation.h"

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
	printf("qux should not print this.\n");
	return 0.0;
}

static void bar(int val1, int val2)
{
	printf("This shouldn't get executed.\n");
}

static int foo()
{
	SKIT_USE_FEATURE_EMULATION;
	/* TODO: bar(sCALL(baz()),sCALL(qux())); (Not sure it's possible at all.) */
	sCALL(bar(baz(), qux()));
	printf("This shouldn't get executed either.\n");
	return 42;
}

static void unittest_exceptions()
{
	SKIT_USE_FEATURE_EMULATION;
	
	if( !skit_exception_is_a(SKIT_EXCEPTION,          SKIT_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if( !skit_exception_is_a(SKIT_BREAK_IN_TRY_CATCH, SKIT_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if(  skit_exception_is_a(SKIT_FATAL_EXCEPTION,    SKIT_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	
	sTRY
		sCALL(baz());
		printf("No exception thrown.\n");
	sCATCH(SKIT_EXCEPTION, e)
		skit_print_exception(e);
	sEND_TRY
	
	printf("------\n");
	
	sTRY
		sTRY
			printf("Nested sTRY-sTRY-sCATCH test.\n");
		sCATCH(SKIT_EXCEPTION, e)
			skit_print_exception(e);
		sEND_TRY
		
		sCALL(foo());
		printf("No exception thrown.\n");
	sCATCH(SKIT_EXCEPTION, e)
		sTRY
			printf("Nested sCATCH-sTRY-sCATCH test.\n");
		sCATCH(SKIT_EXCEPTION, e2)
			printf("This shouldn't happen.\n");
			UNUSED(e2);
			/* skit_print_exception(e2);*/
		sEND_TRY
		
		skit_print_exception(e);
	sEND_TRY
	
	sTRY
		while(1)
		{
			sTRY
				printf("I'm in a loop.\n");
				printf("Now to bail without stopping first!\n");
				break;
			sCATCH(SKIT_EXCEPTION,e)
				UNUSED(e);
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
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
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
				UNUSED(e);
			sEND_TRY
		}
	sCATCH(SKIT_CONTINUE_IN_TRY_CATCH, e)
		skit_print_exception(e);
	sEND_TRY
	
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
	sCALL(unittest_scope_fn_that_throws());
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