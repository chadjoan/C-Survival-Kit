#ifndef SKIT_MACRO_INCLUDED
#define SKIT_MACRO_INCLUDED

void skit_macro_unittest();

/**
Counts the number of arguments passed into a variadic macro.

Limitation: For now, the 0 argument case does not work.
Limitation: For now this only supports up to 15 arguments.
(The zero arg case should be doable.  Jens Gustedt's P99 library can do it, and make it work without the GNU ##__VA_ARGS__ extentsion.)
(The many-arg case is of course doable, but pending a rebalance of laziness and necessity.)

Examples:
	// assert( SKIT_NARGS()        == 0 ); // May work in the future.
	assert( SKIT_NARGS(9000)    == 1 );
	assert( SKIT_NARGS(1,2,3,4) == 4 );
*/
#define SKIT_NARGS(...)  SKIT_NARGS_(__VA_ARGS__,SKIT_RSEQ_N())
#define SKIT_NARGS_(...) SKIT_ARG_N(__VA_ARGS__)
#define SKIT_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define SKIT_RSEQ_N() 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

/**
Limitation: For now, the 0 argument case does not work.
Returns 0 if there are 0 arguments passed.
Returns 1 if there are any other number of arguments passed.
This is mostly useful in macro generation or other meta-programming tasks
  where the result of the macro must be a numeric /literal/ rather than a
  parenthesized expression yielding a number.  The latter could be obtained
  by using MIN(SKIT_NARGS(args),1).  However, func ## MIN(SKIT_NARGS(1,2),1)
  yields 'func(((2)<(1)?(2):(1)))' instead of the desired 'func1'.

Examples:
	// assert( SKIT_NARGS1()        == 0 ); // May work in the future.
	assert( SKIT_NARGS1(9000)    == 1 );
	assert( SKIT_NARGS1(1,2,3,4) == 1 );
*/
#define SKIT_NARGS1(...)  SKIT_NARGS1_(__VA_ARGS__,SKIT_RSEQ_1())
#define SKIT_NARGS1_(...) SKIT_ARG_1(__VA_ARGS__)
#define SKIT_ARG_1(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define SKIT_RSEQ_1() 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0

/**
Limitation: For now, the 0 argument case does not work.
Returns 0 if there are 0 arguments passed.
Returns 1 if there is 1 argument passed.
Returns 2 if there are any other number of arguments passed.
See SKIT_NARGS1 for an explanation of purpose.

Examples:
	// assert( SKIT_NARGS2()        == 0 ); // May work in the future.
	assert( SKIT_NARGS2(9000)    == 1 );
	assert( SKIT_NARGS2(1,2,3,4) == 2 );
*/
#define SKIT_NARGS2(...)  SKIT_NARGS2_(__VA_ARGS__,SKIT_RSEQ_2())
#define SKIT_NARGS2_(...) SKIT_ARG_2(__VA_ARGS__)
#define SKIT_ARG_2(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define SKIT_RSEQ_2() 2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0

/**
Limitation: For now, the 0 argument case does not work.
Returns 0 if there are 0 arguments passed.
Returns 1 if there is 1 argument passed.
Returns 2 if there are 2 arguments passed.
Returns 3 if there are any other number of arguments passed.
See SKIT_NARGS1 for an explanation of purpose.

Examples:
	// assert( SKIT_NARGS3()        == 0 ); // May work in the future.
	assert( SKIT_NARGS3(9000)    == 1 );
	assert( SKIT_NARGS3(1,2,3,4) == 3 );
*/
#define SKIT_NARGS3(...)  SKIT_NARGS3_(__VA_ARGS__,SKIT_RSEQ_3())
#define SKIT_NARGS3_(...) SKIT_ARG_3(__VA_ARGS__)
#define SKIT_ARG_3(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define SKIT_RSEQ_3() 3,3,3,3,3,3,3,3,3,3,3,3,3,2,1,0

/**
Limitation: For now, the 0 argument case does not work.
Returns 0 if there are 0 arguments passed.
Returns 1 if there is 1 argument passed.
Returns 2 if there are 2 arguments passed.
Returns 3 if there are 3 arguments passed.
Returns 4 if there are any other number of arguments passed.
See SKIT_NARGS1 for an explanation of purpose.
*/
#define SKIT_NARGS4(...)  SKIT_NARGS4_(__VA_ARGS__,SKIT_RSEQ_4())
#define SKIT_NARGS4_(...) SKIT_ARG_4(__VA_ARGS__)
#define SKIT_ARG_4(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define SKIT_RSEQ_4() 4,4,4,4,4,4,4,4,4,4,4,4,3,2,1,0

/**
Limitation: For now, the 0 argument case does not work.
Returns 0 if there are 0 arguments passed.
Returns 1 if there is 1 argument passed.
Returns 2 if there are 2 arguments passed.
Returns 3 if there are 3 arguments passed.
Returns 4 if there are 4 arguments passed.
Returns 5 if there are any other number of arguments passed.
See SKIT_NARGS1 for an explanation of purpose.
*/
#define SKIT_NARGS5(...)  SKIT_NARGS5_(__VA_ARGS__,SKIT_RSEQ_5())
#define SKIT_NARGS5_(...) SKIT_ARG_5(__VA_ARGS__)
#define SKIT_ARG_5(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define SKIT_RSEQ_5() 5,5,5,5,5,5,5,5,5,5,5,4,3,2,1,0

/**
SKIT_MACRO_DISPATCHER allows a variadic macro to select other macros to call based
on the number of variadic arguments provided.

Examples:
	#define MAX(...) SKIT_MACRO_DISPATCHER(MAX, __VA_ARGS__)(__VA_ARGS__)
	#define MAX1(a) a
	#define MAX2(a, b) ((a)>(b)?(a):(b))
	#define MAX3(a, b, c) MAX2(MAX2(a, b), c)
	assert( MAX3(1,2,3) == 3 );
*/
#define SKIT_MACRO_DISPATCHER(func, ...) \
	SKIT_MACRO_DISPATCHER_(func, SKIT_NARGS(__VA_ARGS__))
#define SKIT_MACRO_DISPATCHER_(func, nargs) \
	SKIT_MACRO_DISPATCHER__(func, nargs)
#define SKIT_MACRO_DISPATCHER__(func, nargs) \
	func ## nargs

/**
Specialization of SKIT_MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if any other number of arguments are supplied.
*/
#define SKIT_MACRO_DISPATCHER1(func, ...) \
	SKIT_MACRO_DISPATCHER_(func, SKIT_NARGS1(__VA_ARGS__))

/**
Specialization of SKIT_MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if 1 argument is supplied.  
It will call func2 if any other number of arguments are supplied.
*/
#define SKIT_MACRO_DISPATCHER2(func, ...) \
	SKIT_MACRO_DISPATCHER_(func, SKIT_NARGS2(__VA_ARGS__))

/**
Specialization of SKIT_MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if 1 argument is supplied.  
It will call func2 if 2 argument is supplied.  
It will call func3 if any other number of arguments are supplied.
*/
#define SKIT_MACRO_DISPATCHER3(func, ...) \
	SKIT_MACRO_DISPATCHER_(func, SKIT_NARGS3(__VA_ARGS__))
	
/**
Specialization of SKIT_MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if 1 argument is supplied.  
It will call func2 if 2 argument is supplied.  
It will call func3 if 3 argument is supplied.  
It will call func4 if any other number of arguments are supplied.
*/
#define SKIT_MACRO_DISPATCHER4(func, ...) \
	SKIT_MACRO_DISPATCHER_(func, SKIT_NARGS4(__VA_ARGS__))

/**
Specialization of SKIT_MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if 1 argument is supplied.  
It will call func2 if 2 argument is supplied.  
It will call func3 if 3 argument is supplied.  
It will call func4 if 4 argument is supplied.  
It will call func5 if any other number of arguments are supplied.
*/
#define SKIT_MACRO_DISPATCHER5(func, ...) \
	SKIT_MACRO_DISPATCHER_(func, SKIT_NARGS5(__VA_ARGS__))

/** 
Use this to silence warnings about unused variables when the variable is
intentionally declared but not used.
Example:
	void foo(...)
	{
		int a; // Normally the compiler might print "warning: unused variable 'a'"
		SKIT_UNUSED(a); // Now the compiler will be silenced.
	}
*/
#define SKIT_UNUSED(expr) do { (void)(expr); } while (0)

/**
Returns the first argument from its list of arguments.
Example:
#define MY_VARIADIC_FUNC(...) (SKIT_FIRST_VARG(__VA_ARGS__))
assert(MY_VARIADIC_FUNC(3,2,1) == 3);
*/
#define SKIT_FIRST_VARG(...) SKIT_FIRST_VARG_(__VA_ARGS__, throwaway)
#define SKIT_FIRST_VARG_(first,...) first

/**
Returns all arguments passed to it, except for the first one.
Example:
#define SNPRINTFLN(buf,sz,...) \
	snprintf(buf,sz, SKIT_FIRST_VARG(__VA_ARGS__) "\n", \
		SKIT_REST_VARGS(__VA_ARGS__))
char buf[64];
SNPRINTFLN(buf,64, "Test%d", 1);
assert( strcmp(buf,"Test1\n") == 0 );
*/
#define SKIT_REST_VARGS(first,...) __VA_ARGS__

#endif