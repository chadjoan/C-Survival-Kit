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

	#ifndef INT8_MAX
	#define INT8_MAX            ((int8_t)0x7F)
	#endif

	#ifndef INT8_MIN
	#define INT8_MIN            (-INT8_MIN-(int8_t)1)
	#endif

	#ifndef UINT8_MAX
	#define UINT8_MAX           ((uint8_t)0xFF)
	#endif

	#ifndef INT16_MAX
	#define INT16_MAX           ((int16_t)0x7FFF)
	#endif

	#ifndef INT16_MIN
	#define INT16_MIN           (-INT16_MAX-(int16_t)1)
	#endif

	#ifndef UINT16_MAX
	#define UINT16_MAX          ((uint16_t)0xFFFF)
	#endif

	#ifndef INT32_MAX
	#define INT32_MAX           ((int32_t)0x7FFFffffL)
	#endif

	#ifndef INT32_MIN
	#define INT32_MIN           (-INT32_MAX-(int32_t)1)
	#endif

	#ifndef UINT32_MAX
	#define UINT32_MAX          ((uint32_t)0xFFFFffffUL)
	#endif

	#ifndef INT64_MAX
	#define INT64_MAX           ((int64_t)0x7FFFffffFFFFffffLL)
	#endif

	#ifndef INT64_MIN
	#define INT64_MIN           (-INT64_MAX-(int64_t)1)
	#endif

	#ifndef UINT64_MAX
	#define UINT64_MAX          ((uint64_t)0xFFFFffffFFFFffffULL)
	#endif
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
