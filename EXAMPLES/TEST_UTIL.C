#include "survival_kit/features.h"
#include <stdio.h>

int baz()
{
	printf("Baz doesn't throw.\n");
	printf("It does print a stack trace before returning, though:\n");
	printf("%s",stack_trace_to_str());
	printf("\n");
	return 0;
}

float qux()
{
	RAISE(GENERIC_EXCEPTION,"qux does throw!\n");
	printf("qux should not print this.\n");
	return 0.0;
}

void bar(int val1, int val2)
{
	printf("This shouldn't get executed.\n");
}

int foo()
{
	/* TODO: bar(TRYR(baz()),TRYR(qux())); */
	CALL(bar(baz(), qux()));
	printf("This shouldn't get executed either.\n");
	return 42;
}

/* All scope unittest subfunctions should return 0. */
/*
int unittest_scope_exit_normal(int *result)
{
	int *result = 2;
	SCOPE_EXIT(*result--);
	SCOPE_EXIT
		*result--;
	ENDSCOPE
	RETURN;
}

void unittest_scope_exit_exceptional(int *result)
{
	*result = 2;
	SCOPE_EXIT(*result--);
	SCOPE_EXIT
		*result--;
	ENDSCOPE
	RAISE(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

void unittest_scope_failure_noraml(int *result)
{
	*result = 0;
	SCOPE_FAILURE(*result--);
	SCOPE_FAILURE
		*result--;
	ENDSCOPE
	RETURN;
}

void unittest_scope_failure_exceptional(int *result)
{
	int *result = 2;
	SCOPE_FAILURE(*result--);
	SCOPE_FAILURE
		*result--;
	ENDSCOPE
	RAISE(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

void unittest_scope_success_normal(int *result)
{
	int *result = 2;
	SCOPE_SUCCESS(*result--);
	SCOPE_SUCCESS
		*result--;
	ENDSCOPE
	RETURN;
}

void unittest_scope_success_exceptional(int *result)
{
	*result = 0;
	SCOPE_SUCCESS(*result--);
	SCOPE_SUCCESS
		*result--;
	ENDSCOPE
	RAISE(GENERIC_EXCEPTION,"Testing scope statements: this should never print.\n");
}

void unittest_scope()
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
}
*/


void main()
{
	/*printf("nargs: %d\n", VA_NARG(1,2,3));
	printf("max: %d\n", MAX(1, 2, 4, 3));
	exit(0);
	*/
	unittest_err_util();

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
			ENDTRY
		}
	CATCH(CONTINUE_IN_TRY_CATCH, e)
		print_exception(e);
	ENDTRY
}
