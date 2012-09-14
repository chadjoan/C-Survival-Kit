#ifndef SKIT_MACRO_INCLUDED
#define SKIT_MACRO_INCLUDED

/* TODO: examples in this module need to end up in unittests. */
/**
Counts the number of arguments passed into a variadic macro.

Limitation: For now this only supports up to 15 arguments.

Examples:
assert( VA_NARG(1,2,3,4) == 4 );
*/
#define VA_NARG(...)  VA_NARG_(__VA_ARGS__,VA_RSEQ_N())
#define VA_NARG_(...) VA_ARG_N(__VA_ARGS__)
#define VA_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define VA_RSEQ_N() 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

/**
Returns 0 if there are 0 arguments passed.
Returns 1 if there are any other number of arguments passed.
This is mostly useful in macro generation or other meta-programming tasks
  where the result of the macro must be a numeric /literal/ rather than a
  parenthesized expression yielding a number.  The latter could be obtained
  by using MIN(VA_NARG(args),1).  However, func ## MIN(VA_NARG(1,2),1)
  yields 'func(((2)<(1)?(2):(1)))' instead of the desired 'func1'.
*/
#define VA_NARG1(...)  VA_NARG1_(__VA_ARGS__,VA_RSEQ_1())
#define VA_NARG1_(...) VA_ARG_1(__VA_ARGS__)
#define VA_ARG_1(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define VA_RSEQ_1() 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0

/**
Returns 0 if there are 0 arguments passed.
Returns 1 if there is 1 argument passed.
Returns 2 if there are any other number of arguments passed.
See VA_NARG1 for an explanation of purpose.
*/
#define VA_NARG2(...)  VA_NARG2_(__VA_ARGS__,VA_RSEQ_2())
#define VA_NARG2_(...) VA_ARG_2(__VA_ARGS__)
#define VA_ARG_2(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define VA_RSEQ_2() 2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0

/**
Returns 0 if there are 0 arguments passed.
Returns 1 if there is 1 argument passed.
Returns 2 if there are 2 arguments passed.
Returns 3 if there are any other number of arguments passed.
See VA_NARG1 for an explanation of purpose.
*/
#define VA_NARG3(...)  VA_NARG3_(__VA_ARGS__,VA_RSEQ_3())
#define VA_NARG3_(...) VA_ARG_3(__VA_ARGS__)
#define VA_ARG_3(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,N,...) N
#define VA_RSEQ_3() 3,3,3,3,3,3,3,3,3,3,3,3,3,2,1,0

/**
MACRO_DISPATCHER allows a variadic macro to select other macros to call based
on the number of variadic arguments provided.

Examples:
#define MAX(...) MACRO_DISPATCHER(MAX, __VA_ARGS__)(__VA_ARGS__)
#define MAX1(a) a
#define MAX2(a, b) ((a)>(b)?(a):(b))
#define MAX3(a, b, c) MAX2(MAX2(a, b), c)
assert( MAX3(1,2,3) == 3 );
*/
#define MACRO_DISPATCHER(func, ...) \
	MACRO_DISPATCHER_(func, VA_NARG(__VA_ARGS__))
#define MACRO_DISPATCHER_(func, nargs) \
	MACRO_DISPATCHER__(func, nargs)
#define MACRO_DISPATCHER__(func, nargs) \
	func ## nargs

/**
Specialization of MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if any other number of arguments are supplied.
*/
#define MACRO_DISPATCHER1(func, ...) \
	MACRO_DISPATCHER_(func, VA_NARG1(__VA_ARGS__))

/**
Specialization of MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if 1 argument is supplied.  
It will call func2 if any other number of arguments are supplied.
*/
#define MACRO_DISPATCHER2(func, ...) \
	MACRO_DISPATCHER_(func, VA_NARG2(__VA_ARGS__))

/**
Specialization of MACRO_DISPATCHER.
It will call func0 if 0 arguments are supplied.
It will call func1 if 1 argument is supplied.  
It will call func2 if 2 argument is supplied.  
It will call func3 if any other number of arguments are supplied.
*/
#define MACRO_DISPATCHER3(func, ...) \
	MACRO_DISPATCHER_(func, VA_NARG3(__VA_ARGS__))

/** 
Use this to silence warnings about unused variables when the variable is
intentionally declared but not used.
Example:
	void foo(...)
	{
		int a; // Normally the compiler might print "warning: unused variable 'a'"
		UNUSED(a); // Now the compiler will be silenced.
	}
*/
#define UNUSED(expr) do { (void)(expr); } while (0)

#endif