#ifndef SKIT_PORTABLE_INTS_INCLUDED
#define SKIT_PORTABLE_INTS_INCLUDED

#ifdef __VMS
	// More portable on VMS.
	#define skit_uintmax        uint64_t
	#define skit_intmax         int64_t
	#define skit_uintmax_fmt    "%llu"
	#define skit_intmax_fmt     "%lld"
	#define skit_uintmax_hexfmt "%llx"
	#define skit_intmax_hexfmt  "%llx"
	#define skit_uintptr_t      uint64_t
	#define skit_intptr_t       int64_t
#else
	// More portable everywhere else.
	#define skit_uintmax        uintmax_t
	#define skit_intmax         intmax_t
	#define skit_uintmax_fmt    "%ju"
	#define skit_intmax_fmt     "%jd"
	#define skit_uintmax_hexfmt "%jx"
	#define skit_intmax_hexfmt  "%jx"
	#define skit_uintptr_t      uintptr_t
	#define skit_intptr_t       intptr_t
#endif

#endif