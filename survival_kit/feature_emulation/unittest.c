
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
	USE_FEATURE_EMULATION;
	STHROW(GENERIC_EXCEPTION,"qux does throw!\n");
	printf("qux should not print this.\n");
	return 0.0;
}

static void bar(int val1, int val2)
{
	printf("This shouldn't get executed.\n");
}

static int foo()
{
	USE_FEATURE_EMULATION;
	/* TODO: bar(CALL(baz()),CALL(qux())); */
	CALL(bar(baz(), qux()));
	printf("This shouldn't get executed either.\n");
	return 42;
}

static void unittest_exceptions()
{
	USE_FEATURE_EMULATION;
	
	if( !exception_is_a(GENERIC_EXCEPTION, GENERIC_EXCEPTION) )  skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if( !exception_is_a(BREAK_IN_TRY_CATCH, GENERIC_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if(  exception_is_a(FATAL_EXCEPTION, GENERIC_EXCEPTION) )    skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	
	STRY
		CALL(baz());
		printf("No exception thrown.\n");
	SCATCH(GENERIC_EXCEPTION, e)
		skit_print_exception(e);
	END_STRY
	
	printf("------\n");
	
	STRY
		STRY
			printf("Nested STRY-STRY-SCATCH test.\n");
		SCATCH(GENERIC_EXCEPTION, e)
			skit_print_exception(e);
		END_STRY
		
		CALL(foo());
		printf("No exception thrown.\n");
	SCATCH(GENERIC_EXCEPTION, e)
		STRY
			printf("Nested SCATCH-STRY-SCATCH test.\n");
		SCATCH(GENERIC_EXCEPTION, e2)
			printf("This shouldn't happen.\n");
			UNUSED(e2);
			/* skit_print_exception(e2);*/
		END_STRY
		
		skit_print_exception(e);
	END_STRY
	
	STRY
		while(1)
		{
			STRY
				printf("I'm in a loop.\n");
				printf("Now to bail without stopping first!\n");
				break;
			SCATCH(GENERIC_EXCEPTION,e)
				UNUSED(e);
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
			END_STRY
		}
	SCATCH(BREAK_IN_TRY_CATCH, e)
		skit_print_exception(e);
	END_STRY
	
	
	STRY
		while(1)
		{
			STRY
				printf("I'm in a loop (again).\n");
				printf("Now to bail without stopping first!\n");
				continue;
			SCATCH(GENERIC_EXCEPTION,e)
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
				UNUSED(e);
			END_STRY
		}
	SCATCH(CONTINUE_IN_TRY_CATCH, e)
		skit_print_exception(e);
	END_STRY
	
	printf("  exception_handling unittest passed!\n");
}

static void unittest_scope_fn_that_throws()
{
	USE_FEATURE_EMULATION;
	STHROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

/* All scope unittest subfunctions should return 0. */
static void unittest_scope_exit_normal(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 2;
	SCOPE_EXIT((*result)--);
	SCOPE_EXIT_BEGIN
		(*result)--;
	END_SCOPE_EXIT
END_SCOPE

static void unittest_scope_exit_exceptional(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 2;
	SCOPE_EXIT((*result)--);
	SCOPE_EXIT_BEGIN
		(*result)--;
	END_SCOPE_EXIT
	unittest_scope_fn_that_throws();
END_SCOPE

static void unittest_scope_exit_exceptional_call(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 2;
	SCOPE_EXIT((*result)--);
	SCOPE_EXIT_BEGIN
		(*result)--;
	END_SCOPE_EXIT
	CALL(unittest_scope_fn_that_throws());
END_SCOPE

static void unittest_scope_failure_normal(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 0;
	SCOPE_FAILURE((*result)--);
	SCOPE_FAILURE_BEGIN
		(*result)--;
	END_SCOPE_FAILURE
END_SCOPE

static void unittest_scope_failure_exceptional(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 2;
	SCOPE_FAILURE((*result)--);
	SCOPE_FAILURE_BEGIN
		(*result)--;
	END_SCOPE_FAILURE
	STHROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
END_SCOPE

static void unittest_scope_success_normal(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 2;
	SCOPE_SUCCESS((*result)--);
	SCOPE_SUCCESS_BEGIN
		(*result)--;
	END_SCOPE_SUCCESS
END_SCOPE

static void unittest_scope_success_exceptional(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 0;
	SCOPE_SUCCESS((*result)--);
	SCOPE_SUCCESS_BEGIN
		(*result)--;
	END_SCOPE_SUCCESS
	STHROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
END_SCOPE

static int unittest_scope_exit_RETURN(int *result)
SCOPE
	USE_FEATURE_EMULATION;
	
	*result = 3;
	SCOPE_EXIT((*result)--);
	SCOPE_EXIT_BEGIN
		(*result)--;
	END_SCOPE_EXIT
	
	/* Make sure RETURN expands correctly next to if-statements. */
	if ( 0 )
		RETURN(0);
	
	(*result)--;
	
	RETURN(0);
	
	STHROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
	
END_SCOPE

static void unittest_scope()
{
	USE_FEATURE_EMULATION;
	int val = 1;
	
	unittest_scope_exit_normal(&val);
	SKIT_ASSERT_EQ(val,0,"%d");
	
	unittest_scope_success_normal(&val);
	SKIT_ASSERT_EQ(val,0,"%d");
	
	unittest_scope_failure_normal(&val);
	SKIT_ASSERT_EQ(val,0,"%d");
	
	int ret = unittest_scope_exit_RETURN(&val);
	SKIT_ASSERT_EQ(val,0,"%d");
	SKIT_ASSERT_EQ(ret,0,"%d");
	
	STRY
		unittest_scope_exit_exceptional(&val);
	SCATCH(GENERIC_EXCEPTION,e)
		SKIT_ASSERT_EQ(val,0,"%d");
	END_STRY
	
	STRY
		unittest_scope_exit_exceptional_call(&val);
	SCATCH(GENERIC_EXCEPTION,e)
		SKIT_ASSERT_EQ(val,0,"%d");
	END_STRY
	
	STRY
		unittest_scope_success_exceptional(&val);
	SCATCH(GENERIC_EXCEPTION,e)
		SKIT_ASSERT_EQ(val,0,"%d");
	END_STRY
	
	STRY
		unittest_scope_failure_exceptional(&val);
	SCATCH(GENERIC_EXCEPTION,e)
		SKIT_ASSERT_EQ(val,0,"%d");
	END_STRY

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
}
/* End unittests. */