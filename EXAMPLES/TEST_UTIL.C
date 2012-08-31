#include "ERR_UTIL.H"
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
	/*THROWV(NULL);*/
	THROW(new_exception(10,"qux does throw!\n"));
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

void main()
{
	TRY
		baz();
		printf("No exception thrown.\n");
	CATCH(10, e)
		print_exception(e);
	ENDTRY
	
	printf("------\n");
	
	TRY
		TRY
			printf("Nested TRY-TRY-CATCH test.\n");
		CATCH(10, e)
			print_exception(e);
		ENDTRY
		
		foo();
		printf("No exception thrown.\n");
	CATCH(10, e)
		TRY
			printf("Nested CATCH-TRY-CATCH test.\n");
		CATCH(10, e2)
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
			CATCH(10,e)
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
			CATCH(10,e)
				printf("%s, %d: This shouldn't happen.", __FILE__, __LINE__);
			ENDTRY
		}
	CATCH(CONTINUE_IN_TRY_CATCH, e)
		print_exception(e);
	ENDTRY
	
}
