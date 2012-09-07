
/* #include "survival_kit/feature_emulation/unittest.h" */
#include "survival_kit/feature_emulation.h"

/* Unittesting functions. */
static int baz()
{
	printf("Baz doesn't throw.\n");
	printf("It does print a stack trace before returning, though:\n");
	printf("%s",stack_trace_to_str());
	printf("\n");
	return 0;
}

static float qux()
{
	THROW(GENERIC_EXCEPTION,"qux does throw!\n");
	printf("qux should not print this.\n");
	return 0.0;
}

static void bar(int val1, int val2)
{
	printf("This shouldn't get executed.\n");
}

static int foo()
{
	/* TODO: bar(TRYR(baz()),TRYR(qux())); */
	CALL(bar(baz(), qux()));
	printf("This shouldn't get executed either.\n");
	return 42;
}

static void unittest_exceptions()
{
	if( !exception_is_a(GENERIC_EXCEPTION, GENERIC_EXCEPTION) )  skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if( !exception_is_a(BREAK_IN_TRY_CATCH, GENERIC_EXCEPTION) ) skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	if(  exception_is_a(FATAL_EXCEPTION, GENERIC_EXCEPTION) )    skit_die("%s, %d: Assertion failed.",__FILE__,__LINE__);
	
	TRY
		baz();
		printf("No exception thrown.\n");
	CATCH(GENERIC_EXCEPTION, e)
		print_exception(e);
	ENDTRY
	
	printf("------\n");
	
	TRY
		TRY
			printf("Nested TRY-TRY-CATCH test.\n");
		CATCH(GENERIC_EXCEPTION, e)
			print_exception(e);
		ENDTRY
		
		foo();
		printf("No exception thrown.\n");
	CATCH(GENERIC_EXCEPTION, e)
		TRY
			printf("Nested CATCH-TRY-CATCH test.\n");
		CATCH(GENERIC_EXCEPTION, e2)
			printf("This shouldn't happen.\n");
			UNUSED(e2);
			/* print_exception(e2);*/
		ENDTRY
		
		print_exception(e);
	ENDTRY
	
	TRY
		while(1)
		{
			TRY
				printf("I'm in a loop.\n");
				printf("Now to bail without stopping first!\n");
				break;
			CATCH(GENERIC_EXCEPTION,e)
				UNUSED(e);
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
			ENDTRY
		}
	CATCH(BREAK_IN_TRY_CATCH, e)
		print_exception(e);
	ENDTRY
	
	
	
	TRY
		while(1)
		{
			TRY
				printf("I'm in a loop (again).\n");
				printf("Now to bail without stopping first!\n");
				continue;
			CATCH(GENERIC_EXCEPTION,e)
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
				UNUSED(e);
			ENDTRY
		}
	CATCH(CONTINUE_IN_TRY_CATCH, e)
		print_exception(e);
	ENDTRY
	
	printf("  exception_handling unittest passed!\n");
}


/* All scope unittest subfunctions should return 0. */
/*
static int unittest_scope_exit_normal(int *result)
{
	int *result = 2;
	SCOPE_EXIT(*result--);
	SCOPE_EXIT
		*result--;
	ENDSCOPE
	RETURN;
}

static void unittest_scope_exit_exceptional(int *result)
{
	*result = 2;
	SCOPE_EXIT(*result--);
	SCOPE_EXIT
		*result--;
	ENDSCOPE
	THROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

static void unittest_scope_failure_noraml(int *result)
{
	*result = 0;
	SCOPE_FAILURE(*result--);
	SCOPE_FAILURE
		*result--;
	ENDSCOPE
	RETURN;
}

static void unittest_scope_failure_exceptional(int *result)
{
	int *result = 2;
	SCOPE_FAILURE(*result--);
	SCOPE_FAILURE
		*result--;
	ENDSCOPE
	THROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

static void unittest_scope_success_normal(int *result)
{
	int *result = 2;
	SCOPE_SUCCESS(*result--);
	SCOPE_SUCCESS
		*result--;
	ENDSCOPE
	RETURN;
}

static void unittest_scope_success_exceptional(int *result)
{
	*result = 0;
	SCOPE_SUCCESS(*result--);
	SCOPE_SUCCESS
		*result--;
	ENDSCOPE
	THROW(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

static void unittest_scope()
{
	int val = 1;
	
	unittest_scope_exit_normal(&val);
	assert_eq(val,0);
	
	unittest_scope_success_normal(&val);
	assert_eq(val,0);
	
	unittest_scope_failure_normal(&val);
	assert_eq(val,0);
	
	TRY
		unittest_scope_exit_exceptional(&val);
	CATCH(GENERIC_EXCEPTION,e)
		assert_eq(val,0);
	ENDTRY
	
	TRY
		unittest_scope_success_exceptional(&val);
	CATCH(GENERIC_EXCEPTION,e)
		assert_eq(val,0);
	ENDTRY
	
	TRY
		unittest_scope_failure_exceptional(&val);
	CATCH(GENERIC_EXCEPTION,e)
		assert_eq(val,0);
	ENDTRY
	
	printf("  scope unittest passed!\n");
}
*/

void skit_unittest_features()
{
	printf("skit_unittest_features()\n");
	unittest_exceptions();
	/* unittest_scope(); */
	printf("  features unittest passed!\n");
}
/* End unittests. */