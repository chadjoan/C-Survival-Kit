#if defined(__DECC)
#pragma module skit_datetime
#endif

#include "survival_kit/datetime.h"

#include "survival_kit/assert.h"
#include "survival_kit/feature_emulation.h"

int32_t skit__datetime_parse_int( const char *parse_cursor_ptr, size_t nchars_wide )
{
	int32_t result = 0;
	size_t i;
	for ( i = 0; i < nchars_wide; i++ )
	{
		char digit = parse_cursor_ptr[i];
		if ( !skit_is_digit(digit) )
			return -1;

		result *= 10;
		result += (digit - '0');
	}

	return result;
}

static void skit_datetime_parse_int_test()
{
	SKIT_USE_FEATURE_EMULATION;

	const char *test_str = "1234567";

	sASSERT_EQ(skit__datetime_parse_int( test_str, 2 ),     12 );
	sASSERT_EQ(skit__datetime_parse_int( test_str, 4 ),   1234 );
	sASSERT_EQ(skit__datetime_parse_int( test_str, 6 ), 123456 );
	sASSERT_EQ(skit__datetime_parse_int( "abc",    2 ),     -1 );

	printf("  skit_datetime_parse_int_test passed.\n");
}

#define SKIT_DATE_PARSE(E, S) \
    E(YYYY) S("-") E(MM) S("-") E(DD) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AD_BC) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_YYYY_MM_DD)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(YYYY) E(MM) E(DD) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AD_BC) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_YYYYMMDD)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(YY) E(MM) E(DD) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AD_BC) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_YYMMDD)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) S(":") E(mm) S(":") E(ss) S(".") E(microseconds) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm_ss_nnnnnn)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) S(":") E(mm) S(":") E(ss) S(".") E(milliseconds) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm_ss_nnn)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) S(":") E(mm) S(":") E(ss) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm_ss)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) S(":") E(mm) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) E(mm) E(ss) E(microseconds) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmmssnnnnnn)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) E(mm) E(ss) E(milliseconds) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmmssnnn)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) E(mm) E(ss) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmmss)
#undef SKIT_DATE_PARSE

#define SKIT_DATE_PARSE(E, S) \
    E(hh) E(mm) \
    E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)
SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmm)
#undef SKIT_DATE_PARSE

static void skit_datetime_parse_test()
{
	SKIT_USE_FEATURE_EMULATION;

	struct tm time_breakdown;
	uint32_t nanoseconds = 0;
	memset(&time_breakdown, 0, sizeof(struct tm));
	struct tm *tb = &time_breakdown;
	uint32_t *ns = &nanoseconds;

	sASSERT_EQS(skit_parse_YYYY_MM_DD(sSLICE("1985-02-17"), tb, ns, NULL),
		sSLICE("1985-02-17"));
	sASSERT_EQ(tb->tm_year, 85);
	sASSERT_EQ(tb->tm_mon,   1);
	sASSERT_EQ(tb->tm_mday, 17);
	sASSERT_EQ(tb->tm_hour,  0);
	sASSERT_EQ(tb->tm_min,   0);
	sASSERT_EQ(tb->tm_sec,   0);
	sASSERT_EQ(nanoseconds, 0);
	memset(tb, 0, sizeof(*tb));

	sASSERT_EQS(skit_parse_YYYY_MM_DD(sSLICE("19AA-05-17"), tb, ns, NULL),
		skit_slice_null());
	memset(tb, 0, sizeof(*tb));

	sASSERT_EQS(skit_parse_hh_mm_ss_nnn(sSLICE("17:32:60.123"), tb, ns, NULL),
		sSLICE("17:32:60.123"));
	sASSERT_EQ(tb->tm_year,  0);
	sASSERT_EQ(tb->tm_mon,   0);
	sASSERT_EQ(tb->tm_mday,  0);
	sASSERT_EQ(tb->tm_hour, 17);
	sASSERT_EQ(tb->tm_min,  32);
	sASSERT_EQ(tb->tm_sec,  60); // Leap second!
	sASSERT_EQ(nanoseconds, 123000000);
	memset(tb, 0, sizeof(*tb));
	nanoseconds = 0;

	sASSERT_EQS(skit_parse_YYMMDD(sSLICE("061231"), tb, ns, NULL),
		sSLICE("061231"));
	sASSERT_EQ(tb->tm_year, 106);
	sASSERT_EQ(tb->tm_mon,   11);
	sASSERT_EQ(tb->tm_mday,  31);
	memset(tb, 0, sizeof(*tb));

	sASSERT_EQS(skit_parse_YYMMDD(sSLICE("961231"), tb, ns, NULL),
		sSLICE("961231"));
	sASSERT_EQ(tb->tm_year,  96);
	sASSERT_EQ(tb->tm_mon,   11);
	sASSERT_EQ(tb->tm_mday,  31);
	memset(tb, 0, sizeof(*tb));

	sASSERT_EQS(skit_parse_YYYYMMDD(sSLICE("00170101 BC"), tb, ns, NULL),
		sSLICE("00170101 BC"));
	sASSERT_EQ(tb->tm_year, -1917);
	sASSERT_EQ(tb->tm_mon,      0);
	sASSERT_EQ(tb->tm_mday,     1);
	memset(tb, 0, sizeof(*tb));

	sASSERT_EQS(skit_parse_hhmmss(sSLICE("075900 pM"), tb, ns, NULL),
		sSLICE("075900 pM"));
	sASSERT_EQ(tb->tm_hour, 19);
	sASSERT_EQ(tb->tm_min,  59);
	sASSERT_EQ(tb->tm_sec,   0);
	memset(tb, 0, sizeof(*tb));

	printf("  skit_datetime_parse_test passed.\n");
}

void skit_datetime_unittests()
{
	SKIT_USE_FEATURE_EMULATION;
	printf("skit_datetime_unittests()\n");
	
	sTRACE(skit_datetime_parse_int_test());
	sTRACE(skit_datetime_parse_test());

	printf("  skit_datetime_unittests passed!\n");
	printf("\n");
}
