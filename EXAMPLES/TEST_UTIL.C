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
	THROW_NUM(new_exception("qux does throw!\n"));
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
	TRY_NUM(bar(baz(), qux()));
	printf("This shouldn't get executed either.\n");
	return 42;
}

void main()
{
	TRY_CATCH(baz())
		printf("No exception thrown.\n");
	CATCH(e)
		print_exception(e);
	ENDTRY
	
	printf("------\n");
	
	TRY_CATCH(foo())
		printf("No exception thrown.\n");
	CATCH(e)
		print_exception(e);
	ENDTRY
}
