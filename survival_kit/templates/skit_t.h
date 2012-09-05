

/* Clean this up if other templates were using it. */
#ifdef SKIT_T
#undef SKIT_T
#undef SKIT_T_PRE
#undef SKIT_T_PRE2
#undef SKIT_T_STR
#undef SKIT_T_STR2
#undef SKIT_T_STR3
#endif

#ifndef SKIT_T_PREFIX
#error "SKIT_T_PREFIX is needed but was not defined."
#endif

#define SKIT_T_STR3(str) #str
#define SKIT_T_STR2(str) SKIT_T_STR3(str)
#define SKIT_T_STR(ident) SKIT_T_STR2(SKIT_T(slist))

#define SKIT_T_PRE2(prefix,ident) \
	skit_ ## prefix ## _ ## ident

/* This extra level of indirection forces prefix and ident to be expanded 
   before the ## operator pastes things together. 
   Source: http://gcc.gnu.org/onlinedocs/cpp/Argument-Prescan.html#Argument-Prescan */
#define SKIT_T_PRE(prefix,ident) \
	SKIT_T_PRE2(prefix,ident)

#define SKIT_T(ident) \
	SKIT_T_PRE(SKIT_T_PREFIX,ident)
