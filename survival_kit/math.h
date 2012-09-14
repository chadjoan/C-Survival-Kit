
#ifndef SKIT_MATH_INCLUDED
#define SKIT_MATH_INCLUDED

#include "survival_kit/macro.h"

/**
SKIT_MAX finds the maximum among its arguments.
Warning: Given that this is a macro implementation, do not place expressions
  with side-effects into the arguments of this function.  
  Do not do SKIT_MAX(i++,j--).  They /will/ be evaluated more than once.
Limitation: currently only supports up to 8 arguments.
Example:
	int foo = 5;
	double bar = SKIT_MAX(foo, 6, foo+3.5, 1.2);
	sASSERT_EQ(bar, 8.5);
*/
#define SKIT_MAX(...) MACRO_DISPATCHER(SKIT_MAX, __VA_ARGS__)(__VA_ARGS__)
#define SKIT_MAX1(a) a
#define SKIT_MAX2(a, b) ((a)>(b)?(a):(b))
#define SKIT_MAX3(a, b, c) SKIT_MAX2(SKIT_MAX2(a, b), c)
#define SKIT_MAX4(a, b, c, d) SKIT_MAX2(SKIT_MAX2(a, b), SKIT_MAX2(c, d))
#define SKIT_MAX5(a, b, c, d, e) SKIT_MAX2(SKIT_MAX4(a,b,c,d),e)
#define SKIT_MAX6(a, b, c, d, e, f) SKIT_MAX2(SKIT_MAX4(a,b,c,d),SKIT_MAX2(e,f))
#define SKIT_MAX7(a, b, c, d, e, f, g) SKIT_MAX2(SKIT_MAX4(a,b,c,d),SKIT_MAX3(e,f,g))
#define SKIT_MAX8(a, b, c, d, e, f, g, h) SKIT_MAX2(SKIT_MAX4(a,b,c,d),SKIT_MAX4(e,f,g,h))

/**
SKIT_MIN finds the minimum among its arguments.
Warning: Given that this is a macro implementation, do not place expressions
  with side-effects into the arguments of this function.  
  Do not do SKIT_MIN(i++,j--).  They /will/ be evaluated more than once.
Limitation: currently only supports up to 8 arguments.
Example:
	int foo = 5;
	double bar = SKIT_MIN(foo, 6, foo+3.5, 1.2);
	sASSERT_EQ(bar, 1.2);
*/
#define SKIT_MIN(...) MACRO_DISPATCHER(SKIT_MIN, __VA_ARGS__)(__VA_ARGS__)
#define SKIT_MIN1(a) a
#define SKIT_MIN2(a, b) ((a)<(b)?(a):(b))
#define SKIT_MIN3(a, b, c) SKIT_MIN2(SKIT_MIN2(a, b), c)
#define SKIT_MIN4(a, b, c, d) SKIT_MIN2(SKIT_MIN2(a, b), SKIT_MIN2(c, d))
#define SKIT_MIN5(a, b, c, d, e) SKIT_MIN2(SKIT_MIN4(a,b,c,d),e)
#define SKIT_MIN6(a, b, c, d, e, f) SKIT_MIN2(SKIT_MIN4(a,b,c,d),SKIT_MIN2(e,f))
#define SKIT_MIN7(a, b, c, d, e, f, g) SKIT_MIN2(SKIT_MIN4(a,b,c,d),SKIT_MIN3(e,f,g))
#define SKIT_MIN8(a, b, c, d, e, f, g, h) SKIT_MIN2(SKIT_MIN4(a,b,c,d),SKIT_MIN4(e,f,g,h))

void skit_math_unittest();

#endif
