#ifndef SKIT_DATETIME_INCLUDED
#define SKIT_DATETIME_INCLUDED

//#include "survival_kit/math.h"
#include "survival_kit/string.h"
#include "survival_kit/streams/stream.h"

#include <time.h>
#include <inttypes.h>

// Internal functions.
int32_t skit__datetime_parse_int( const char *parse_cursor_ptr, size_t nchars_wide );
//int skit__datetime_parse_frac( char *str_ptr, size_t str_len, size_t parse_cursor, uint32_t *int_ptr );

#define SKIT__DATETIME_FAIL_EXIT(...) \
	{ \
		if ( optional_depth == 0 && err_stream != NULL ) \
		{ \
			skit_stream_appendf(err_stream, __VA_ARGS__); \
			skit_stream_appendf(err_stream, "\n" ); \
		} \
		failed = 1; \
		break; \
	}

#define SKIT__DATETIME_PARSE_ELEMENT(ELEMENT) \
	SKIT__DATETIME_ELEM_##ELEMENT

#define SKIT__DATETIME_PARSE_INT( nchars_wide, min_value, max_value, name, field_path ) \
	if ( parse_cursor + nchars_wide > str_len ) \
		SKIT__DATETIME_FAIL_EXIT( \
			"Reached end-of-string before parsing %d character wide %s", \
			nchars_wide, name) \
	int_return_value = skit__datetime_parse_int( str_ptr + parse_cursor, nchars_wide ); \
	if ( int_return_value < 0 ) \
		SKIT__DATETIME_FAIL_EXIT( \
			"Non-digit characters '%.*s' found while parsing the %s", \
			nchars_wide, str_ptr + parse_cursor, name ) \
	if ( int_return_value < min_value ) \
		SKIT__DATETIME_FAIL_EXIT( \
			"%d is a value too low to be the %s", int_return_value, name) \
	if ( int_return_value > max_value ) \
		SKIT__DATETIME_FAIL_EXIT( \
			"%d is a value too large to be the %s", int_return_value, name) \
	(field_path) = int_return_value; \
	\
	/* Parse succeeded.  Continue. */ \
	parse_cursor += nchars_wide;

#define SKIT__DATETIME_ELEM_SPACES \
	while ( parse_cursor < str_len && skit_is_whitespace(str_ptr[parse_cursor]) ) \
		parse_cursor++;

// NOTE: tm_year is supposed to store years since 1900, but these will output
//   years since Gregorian year 0.  This is OK for now, because we will adjust
//   by 1900 at the end of the generated parse function.  (We can also adjust
//   for BC when we do that, too.)
#define SKIT__DATETIME_ELEM_YYYY \
	SKIT__DATETIME_PARSE_INT( 4, 0, 9999, "year", time_breakdown->tm_year ) \
	have_year = 1;

#define SKIT__DATETIME_ELEM_YY \
	SKIT__DATETIME_PARSE_INT( 2, 0, 99, "year", time_breakdown->tm_year ) \
	have_year = 1; \
	\
	/* Infer the century. */ \
	if ( time_breakdown->tm_year >= 69 ) \
		(time_breakdown->tm_year) += 1900; \
	else \
		(time_breakdown->tm_year) += 2000;

#define SKIT__DATETIME_ELEM_MM \
	SKIT__DATETIME_PARSE_INT( 2, 1, 12, "month", time_breakdown->tm_mon ) \
	have_month = 1;

#define SKIT__DATETIME_ELEM_DD        SKIT__DATETIME_PARSE_INT( 2, 1,     31, "day",          time_breakdown->tm_mday )
#define SKIT__DATETIME_ELEM_hh        SKIT__DATETIME_PARSE_INT( 2, 0,     23, "hour",         time_breakdown->tm_hour )
#define SKIT__DATETIME_ELEM_mm        SKIT__DATETIME_PARSE_INT( 2, 0,     59, "minute",       time_breakdown->tm_min )
#define SKIT__DATETIME_ELEM_ss        SKIT__DATETIME_PARSE_INT( 2, 0,     60, "second",       time_breakdown->tm_sec )

#define SKIT__DATETIME_ELEM_milliseconds \
	SKIT__DATETIME_PARSE_INT( 3, 0, 999, "milliseconds", *nanoseconds ) \
	(*nanoseconds) *= 1000000;

#define SKIT__DATETIME_ELEM_microseconds \
	SKIT__DATETIME_PARSE_INT( 6, 0, 999999, "microseconds", *nanoseconds ) \
	(*nanoseconds) *= 1000;

#define SKIT__DATETIME_ELEM_nanoseconds \
	SKIT__DATETIME_PARSE_INT( 6, 0, (uint32_t)999999999, "nanoseconds", *nanoseconds )

#if 0
// not right now...
#define SKIT__DATETIME_ELEM_frac_of_second \
	if ( parse_cursor == str_len ) \
		SKIT__DATETIME_FAIL_EXIT( \
			"Reached end-of-string before parsing at least one digit of fractions of seconds.") \
	if ( !skit_is_digit(str_ptr[parse_cursor]) ) \
		SKIT__DATETIME_FAIL_EXIT( \
			"Encountered non-digit while attempting to parse fractions of seconds (at least one digit is required).") \
	*nanoseconds = str_ptr[parse_cursor] - '0'; \
	parse_cursor++; \
	i = 1; /* For counting digits */ \
	while ( parse_cursor < str_len ) \
	{ \
		char digit = str_ptr[parse_cursor]; \
		if ( !skit_is_digit(digit) ) \
			break; \
		digit -= '0'; \
		\
		if ( i <= 9 ) \
		{ \
			(*nanoseconds) *= 10; \
			(*nanoseconds) += digit; \
		} \
		else if ( i == 10 ) \
		{ \
			if ( digit >= 5 ) \
				(*nanoseconds) += 1; \
		} \
		parse_cursor++; \
		i++; \
	} \
	(*nanoseconds) = *nanoseconds % skit_ipow(10, 9); \
	if ( i < 9 ) \
		(*nanoseconds) *= skit_ipow(10, 9 - i);
#endif

#define SKIT__DATETIME_AM_PM_COMMON \
	do { \
		char c0 = str_ptr[parse_cursor+0]; \
		char c1 = str_ptr[parse_cursor+1]; \
		if ( (c0 == 'A' || c0 == 'a') && (c1 == 'M' || c1 == 'm' ) ) \
			parse_cursor += 2; \
		else \
		if ( (c0 == 'P' || c0 == 'p') && (c1 == 'M' || c1 == 'm' ) ) \
		{ \
			parse_cursor += 2; \
			have_pm = 1; \
		} \
	} \
	while (0)

#define SKIT__DATETIME_ELEM_AM_PM \
	if ( parse_cursor + 2 > str_len ) \
		SKIT__DATETIME_FAIL_EXIT( "Reached end of string before parsing AM/PM." ) \
	i = parse_cursor; \
	SKIT__DATETIME_AM_PM_COMMON; \
	if ( parse_cursor != i+2 ) \
		SKIT__DATETIME_FAIL_EXIT( "Expected AM/PM, instead got '%.*s'.", 2, str_ptr + parse_cursor)

#define SKIT__DATETIME_AD_BC_COMMON \
	do { \
		char c0 = str_ptr[parse_cursor+0]; \
		char c1 = str_ptr[parse_cursor+1]; \
		if ( (c0 == 'A' || c0 == 'a') && (c1 == 'D' || c1 == 'd' ) ) \
			parse_cursor += 2; \
		else \
		if ( (c0 == 'B' || c0 == 'b') && (c1 == 'C' || c1 == 'c' ) ) \
		{ \
			parse_cursor += 2; \
			have_bc = 1; \
		} \
	} \
	while (0)

#define SKIT__DATETIME_ELEM_AD_BC \
	if ( parse_cursor + 2 > str_len ) \
		SKIT__DATETIME_FAIL_EXIT( "Reached end of string before parsing BC/AD." ) \
	i = parse_cursor; \
	SKIT__DATETIME_AD_BC_COMMON; \
	if ( parse_cursor != i+2 ) \
		SKIT__DATETIME_FAIL_EXIT( "Expected BC/AD, instead got '%.*s'.", 2, str_ptr + parse_cursor)

#define SKIT__DATETIME_ELEM_OPT_BEGIN \
	{ \
		ssize_t parse_cursor_save = parse_cursor; \
		optional_depth++; \
		do {

#define SKIT__DATETIME_ELEM_OPT_END \
		} while (0); \
		if ( failed ) \
			parse_cursor = parse_cursor_save; \
		optional_depth--; \
		failed = 0; /* Optional parses never fail, they just match nothing ;) */ \
	}

#define SKIT__DATETIME_PARSE_CSTRING(cstr) \
	len = SKIT_MIN(sizeof(cstr)-1, str_len - parse_cursor ); \
	i = 0; \
	tmp_ptr = str_ptr + parse_cursor; \
	while( i < len && tmp_ptr[i] == cstr[i] ) \
		i++; \
	if ( i < len ) \
		SKIT__DATETIME_FAIL_EXIT( "Expected '%s', instead got '%.*s'", cstr, (int)i, str_ptr + parse_cursor) \
	\
	/* Parse succeeded.  Continue. */ \
	parse_cursor += len;

/// Generates a C function that parses the date format defined by
/// DATE_FMT_XMACRO and whose name is given by the 'func_name' parameter.
///
/// The DATE_FMT_XMACRO is a macro defined by calling/invoking code that
/// accepts two macro arguments: one for specifying date elements, and another
/// for specifying fixed-length strings as C string literals.
/// As an example, the DATE_FMT_XMACRO works like this:
///   #define MY_DATE_FMT(E, S) \                                            //
///     E(YYYY) S("-") E(MM) S("-") E(DD)                                    //
/// The above MY_DATE_FMT macro defines a date format that parses any date of
/// the form YYYY-MM-DD, such as 1999-08-23.
///
/// This should create code that can parse dates and times faster than
/// traditional format strings, since the parsing code does not have to
/// parse a format string while parsing the date.  The downside is that the
/// date format must be known at compile time.  Since date formats do tend to
/// be known at compile time, this is a reasonable trade-off.
/// (TODO: Efficiency claims are, as of yet, unsubstantiated.)
///
/// The function generated by the SKIT_GEN_DATETIME_PARSING_FUNC macro will
/// have this signature:
/// skit_slice func_name(
///     const skit_slice date_as_str,
///     struct tm *time_breakdown,
///     uint32_t *nanoseconds,
///     skit_stream *err_stream );
///
/// This function will parse 'date_as_str' and populate the 'time_breakdown'
/// and 'nanoseconds' arguments with the data parsed from 'date_as_str'.
///
/// Either of the 'time_breakdown' and 'nanoseconds' arguments may be passed a
/// value of NULL.  In that case the argument will be ignored, even if there
/// are format specifications that would populate it.
///
/// As a general rule, parse failures may leave the arguments in a partially
/// populated state.  This includes elements wrapped in OPT_BEGIN and OPT_END:
/// it is possible for an optional branch to fail AFTER populating one of its
/// elements, in which case that element will remain populated, regardless
/// of the match/fail status of the entire parse.  This is done to avoid
/// the copying of large structures within the parser function, since such
/// copying would be required to guarantee the undoing of side-effects caused
/// during parse failures.
///
/// Components of the 'time_breakdown' or 'nanoseconds' arguments that are not
/// referenced in the format macro will not be altered in any way by calls to
/// the generated parsing function.
///
/// Upon successful completion, the function will return the portion of the
/// input date_as_str that matches the date format.
///
/// If the parsing function encounters an error, it will return
/// skit_slice_null() and write a descriptive error message to 'err_stream'.
/// If 'err_stream' is NULL, then it will be ignored, and no error message
/// will be given.
///
/// The following element options are currently supported:
/// YYYY           The 4-digit year.
/// YY             The 2-digit year, with century being inferred.  Values in
///                  the range [69,99] shall refer to years 1969 to 1999
///                  inclusive, and values in the range [00,68] shall refer to
///                  years 2000 to 2068 inclusive.  The default century
///                  inferred may change in the future.
/// MM             The month number [01,12].
/// DD             The day of the month [01,31].
/// hh             The hour (24-hour clock) [00,23].
/// mm             The minute [00,59].
/// ss             The second [00,60].
/// milliseconds   The milliseconds [000,999].
///                  This will populate the nanoseconds parameter.
/// microseconds   The microseconds [000000,999999].
///                  This will populate the nanoseconds parameter.
/// nanoseconds    The nanoseconds [000000000,999999999].
/// AM_PM          Matches either AM or PM.  This is case insensitive.  If PM
///                  is given, the tm_hour field will be adjusted accordingly.
/// AD_BC          Matches either AD or BC.  This is case insensitive.  If BC
///                  is given, the tm_year field will be adjusted accordingly.
/// OPT_BEGIN      Begins a sequence of optional date formatting.
///                  This must be matched with an OPT_END element.
/// OPT_END        Ends a sequence of optional date formatting.
///                  This must follow an OPT_BEGIN element.
///
/// In all cases, leading zeroes are required unless otherwise specified.
///
#define SKIT_GEN_DATETIME_PARSING_FUNC( DATE_FMT_XMACRO, func_name ) \
skit_slice func_name( \
	const skit_slice date_as_str, \
	struct tm *time_breakdown, \
	uint32_t *nanoseconds, \
	skit_stream *err_stream \
) \
{ \
	struct tm surrogate_tm; \
	uint32_t nanosecs_tmp; \
	const char *str_ptr = (const char*)sSPTR(date_as_str); \
	ssize_t str_len = sSLENGTH(date_as_str); \
	ssize_t parse_cursor = 0; \
	ssize_t i = 0; \
	ssize_t len = 0; \
	const char *tmp_ptr = NULL; \
	int32_t int_return_value = -1; \
	int have_pm = 0; \
	int have_bc = 0; \
	int have_year = 0; \
	int have_month = 0; \
	int optional_depth = 0; \
	int failed = 0; \
	\
	/* Silence compiler warnings from unused variables. */ \
	/* The caller may or may not cause to be generated code that uses these. */ \
	(void)i; (void)len; (void)tmp_ptr; (void)optional_depth; \
	(void)have_pm; (void)have_bc; (void)have_year; (void)have_month; \
	\
	if ( time_breakdown == NULL ) \
		time_breakdown = &surrogate_tm; \
	if ( nanoseconds == NULL ) \
		nanoseconds = &nanosecs_tmp; \
	\
	/* <insert parsing mechinations> */ \
	/* Wrap them in a do{...}while(0) to catch any break; statements from */ \
	/* parse failures that need to exit. */ \
	do { \
		DATE_FMT_XMACRO( \
			SKIT__DATETIME_PARSE_ELEMENT, \
			SKIT__DATETIME_PARSE_CSTRING) \
	} while(0); \
	\
	if ( failed ) \
		return skit_slice_null(); \
	\
	if ( have_year ) \
	{ \
		/* This must be adjusted BEFORE subtracting 1900 from tm_year. */ \
		if ( have_bc ) \
			time_breakdown->tm_year = -time_breakdown->tm_year; \
		\
		/* This is supposed to be stored as "years since 1900". */ \
		/* Ours starts off in Gregorian "years since 0", so we need to adjust it. */ \
		(time_breakdown->tm_year) -= 1900; \
	} \
	\
	if ( have_pm && time_breakdown->tm_hour < 12 ) \
		(time_breakdown->tm_hour) += 12; \
	\
	/* time.h specifies tm_mon as having a [0,11] inclusive range. */ \
	/* Ours is currently [01,12], so we need to adjust it. */ \
	if ( have_month ) \
		(time_breakdown->tm_mon)--; \
	\
	return skit_slice_of_cstrn( str_ptr, parse_cursor); \
}

/// The type of a function pointer that points to a datetime parsing function
/// created by SKIT_GEN_DATETIME_PARSING_FUNC.
typedef skit_slice (*skit_datetime_parsing_fn)(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(YYYY) S("-") E(MM) S("-") E(DD) \                                  //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AD_BC) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_YYYY_MM_DD);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "YYYY-MM-DD" and "YYYY-MM-DD BC".
///
skit_slice skit_parse_YYYY_MM_DD(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(YYYY) E(MM) E(DD) \                                                //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AD_BC) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_YYYYMMDD);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "YYYYMMDD" and "YYYYMMDD BC".
///
skit_slice skit_parse_YYYYMMDD(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(YY) E(MM) E(DD) \                                                  //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AD_BC) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_YYMMDD);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "YYMMDD" and "YYYYMMDD BC".
///
skit_slice skit_parse_YYMMDD(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) S(":") E(mm) S(":") E(ss) S(".") E(microseconds) \             //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm_ss_nnnnnn);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hh:mm:ss.nnnnnn" and "hh:mm:ss.nnnnnn PM".
///
skit_slice skit_parse_hh_mm_ss_nnnnnn(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) S(":") E(mm) S(":") E(ss) S(".") E(milliseconds) \             //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm_ss_nnn);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hh:mm:ss.nnn" and "hh:mm:ss.nnn PM".
///
skit_slice skit_parse_hh_mm_ss_nnn(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) S(":") E(mm) S(":") E(ss) \                                    //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm_ss);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hh:mm:ss" and "hh:mm:ss PM".
///
skit_slice skit_parse_hh_mm_ss(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) S(":") E(mm) \                                                 //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hh_mm);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hh:mm" and "hh:mm PM".
///
skit_slice skit_parse_hh_mm(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) E(mm) E(ss) E(microseconds) \                                  //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmmssnnnnnn);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hhmmssnnnnnn" and "hhmmssnnnnnn PM".
///
skit_slice skit_parse_hhmmssnnnnnn(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) E(mm) E(ss) E(milliseconds) \                                  //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmmssnnn);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hhmmssnnn" and "hhmmssnnn PM".
///
skit_slice skit_parse_hhmmssnnn(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) E(mm) E(ss) \                                                  //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmmss);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hhmmss" and "hhmmss PM".
///
skit_slice skit_parse_hhmmss(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

/// Parsing function with the following definition:
/// 
/// #define SKIT_DATE_PARSE(E, S) \                                          //
///     E(hh) E(mm) \                                                        //
///     E(OPT_BEGIN) S(" ") E(SPACES) E(AM_PM) E(OPT_END)                    //
/// SKIT_GEN_DATETIME_PARSING_FUNC(SKIT_DATE_PARSE, skit_parse_hhmm);
/// #undef SKIT_DATE_PARSE
///
/// It will match strings of the form "hhmm" and "hhmm PM".
///
skit_slice skit_parse_hhmm(
	const skit_slice date_as_str,
	struct tm *time_breakdown,
	uint32_t *nanoseconds,
	skit_stream *err_stream
);

void skit_datetime_unittests();

#endif

