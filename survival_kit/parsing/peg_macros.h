#ifndef SKIT_PARSING_PEG_MACROS_INCLUDED
#define SKIT_PARSING_PEG_MACROS_INCLUDED

#include "survival_kit/macro.h"

// Default parser debugging status.
#ifndef SKIT_DEBUG_PEG_PARSING
#define SKIT_DEBUG_PEG_PARSING 0
#endif

#if SKIT_DEBUG_PEG_PARSING == 1

#  define SKIT__PEG_PRINT_ENTRY() \
	do { \
		skit__peg_on_entry(parser, cursor, ubound, __func__); \
	} while(0)

#  define SKIT__PEG_PRINT_EXIT()  \
	do { \
		skit__peg_on_exit(parser, match, __func__); \
	} while(0)

#else
#  define SKIT__PEG_PRINT_ENTRY() ((void)0)
#  define SKIT__PEG_PRINT_EXIT()  ((void)0)
#endif

#define SKIT_PEG_PARSING_INITIAL_VARS(parser) \
		int auto_consume_whitespace = 1; \
		ssize_t cursor = 0; \
		ssize_t new_cursor = 0; \
		ssize_t ubound = skit_slice_is_null((parser)->input) ? 0 : sSLENGTH((parser)->input); \
		skit_peg_parse_match match; \
		(void)auto_consume_whitespace; \
		(void)cursor; \
		(void)new_cursor; \
		(void)ubound; \
		(void)match;

#define SKIT_PEG_DEFINE_RULE1(rule_name) \
	static skit_peg_parse_match SKIT_PEG_ ## rule_name ( \
		skit_peg_parser *parser, ssize_t cursor, ssize_t ubound ) \
	{ \
		SKIT_PEG_RULE_HEADER

#define SKIT_PEG_DEFINE_RULE2(rule_name, ...) \
	static skit_peg_parse_match SKIT_PEG_ ## rule_name ( \
		skit_peg_parser *parser, ssize_t cursor, ssize_t ubound, __VA_ARGS__) \
	{ \
		SKIT_PEG_RULE_HEADER

#define SKIT_PEG_DEFINE_RULE(...) \
	SKIT_MACRO_DISPATCHER2(SKIT_PEG_DEFINE_RULE, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_RULE_HEADER \
		int auto_consume_whitespace = 1; \
		ssize_t new_cursor = cursor; \
		skit_peg_parse_match match; \
		(void)new_cursor; \
		(void)match; \
		(void)auto_consume_whitespace; \
		SKIT__PEG_PRINT_ENTRY();

#define SKIT_PEG_END_RULE \
		SKIT__PEG_PRINT_EXIT(); \
		return match; \
	}

#define SKIT_PEG__BRANCH_DISCARD(cursor_reset_pos) \
	do { \
		if ( parser->branch_discard != NULL ) \
			parser->branch_discard(parser, cursor_reset_pos); \
	} while(0)

#define SKIT_PEG_SEQ1(a) \
	do { \
		a; \
	} while(0)

#define SKIT_PEG_SEQ2(a, b) \
	do { \
		a; \
		if (!match.successful) \
			break; \
		\
		new_cursor = match.end; \
		if ( auto_consume_whitespace && parser->parse_whitespace != NULL ) \
			new_cursor = parser->parse_whitespace(parser, new_cursor, ubound); \
		\
		b; \
		if (!match.successful) \
			break; \
		\
		new_cursor = match.end; \
		match = skit_peg_match_success(parser, cursor, new_cursor); \
	} while(0)

#define SKIT_PEG_SEQ3(a,b,c)                 SKIT_PEG_SEQ2(a,SKIT_PEG_SEQ2(b,c))
#define SKIT_PEG_SEQ4(a,b,c,d)               SKIT_PEG_SEQ2(SKIT_PEG_SEQ2(a,b),       SKIT_PEG_SEQ2(c,d))
#define SKIT_PEG_SEQ5(a,b,c,d,e)             SKIT_PEG_SEQ2(SKIT_PEG_SEQ3(a,b,c),     SKIT_PEG_SEQ2(d,e))
#define SKIT_PEG_SEQ6(a,b,c,d,e,f)           SKIT_PEG_SEQ2(SKIT_PEG_SEQ3(a,b,c),     SKIT_PEG_SEQ3(d,e,f))
#define SKIT_PEG_SEQ7(a,b,c,d,e,f,g)         SKIT_PEG_SEQ2(SKIT_PEG_SEQ4(a,b,c,d),   SKIT_PEG_SEQ3(e,f,g))
#define SKIT_PEG_SEQ8(a,b,c,d,e,f,g,h)       SKIT_PEG_SEQ2(SKIT_PEG_SEQ4(a,b,c,d),   SKIT_PEG_SEQ4(e,f,g,h))
#define SKIT_PEG_SEQ9(a,b,c,d,e,f,g,h,i)     SKIT_PEG_SEQ2(SKIT_PEG_SEQ5(a,b,c,d,e), SKIT_PEG_SEQ4(f,g,h,i))
#define SKIT_PEG_SEQ10(a,b,c,d,e,f,g,h,i,j)  SKIT_PEG_SEQ2(SKIT_PEG_SEQ5(a,b,c,d,e), SKIT_PEG_SEQ5(f,g,h,i,j))

#define SKIT_PEG_SEQ(...) SKIT_MACRO_DISPATCHER(SKIT_PEG_SEQ, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_ZERO_OR_MORE1(a) \
	do { \
		ssize_t stored_cursor = new_cursor; \
		while(1) { \
			a; \
			if (!match.successful) \
				break; \
			\
			new_cursor = match.end; \
			if ( auto_consume_whitespace && parser->parse_whitespace != NULL ) \
				new_cursor = parser->parse_whitespace(parser, new_cursor, ubound); \
			stored_cursor = new_cursor; \
		} \
		new_cursor = stored_cursor; \
		/* We /know/ a branch failure will have occured: */ \
		/* it's the only way to stop iteration. */ \
		SKIT_PEG__BRANCH_DISCARD(new_cursor); \
		match = skit_peg_match_success(parser, cursor, new_cursor); \
	} while(0)

#define SKIT_PEG_ZERO_OR_MORE2(a,b)                  SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ2(a,b))
#define SKIT_PEG_ZERO_OR_MORE3(a,b,c)                SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ3(a,b,c))
#define SKIT_PEG_ZERO_OR_MORE4(a,b,c,d)              SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ4(a,b,c,d))
#define SKIT_PEG_ZERO_OR_MORE5(a,b,c,d,e)            SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ5(a,b,c,d,e))
#define SKIT_PEG_ZERO_OR_MORE6(a,b,c,d,e,f)          SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ6(a,b,c,d,e,f))
#define SKIT_PEG_ZERO_OR_MORE7(a,b,c,d,e,f,g)        SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ7(a,b,c,d,e,f,g))
#define SKIT_PEG_ZERO_OR_MORE8(a,b,c,d,e,f,g,h)      SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ8(a,b,c,d,e,f,g,h))
#define SKIT_PEG_ZERO_OR_MORE9(a,b,c,d,e,f,g,h,i)    SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ9(a,b,c,d,e,f,g,h,i))
#define SKIT_PEG_ZERO_OR_MORE10(a,b,c,d,e,f,g,h,i,j) SKIT_PEG_ZERO_OR_MORE1(SKIT_PEG_SEQ10(a,b,c,d,e,f,g,h,i,j))

#define SKIT_PEG_ZERO_OR_MORE(...) SKIT_MACRO_DISPATCHER(SKIT_PEG_ZERO_OR_MORE, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_ONE_OR_MORE1(a)                     SKIT_PEG_SEQ2(a,                                   SKIT_PEG_ZERO_OR_MORE1(a))
#define SKIT_PEG_ONE_OR_MORE2(a,b)                   SKIT_PEG_SEQ2(SKIT_PEG_SEQ2(a,b),                  SKIT_PEG_ZERO_OR_MORE2(a,b))
#define SKIT_PEG_ONE_OR_MORE3(a,b,c)                 SKIT_PEG_SEQ2(SKIT_PEG_SEQ3(a,b,c),                SKIT_PEG_ZERO_OR_MORE3(a,b,c))
#define SKIT_PEG_ONE_OR_MORE4(a,b,c,d)               SKIT_PEG_SEQ2(SKIT_PEG_SEQ4(a,b,c,d),              SKIT_PEG_ZERO_OR_MORE4(a,b,c,d))
#define SKIT_PEG_ONE_OR_MORE5(a,b,c,d,e)             SKIT_PEG_SEQ2(SKIT_PEG_SEQ5(a,b,c,d,e),            SKIT_PEG_ZERO_OR_MORE5(a,b,c,d,e))
#define SKIT_PEG_ONE_OR_MORE6(a,b,c,d,e,f)           SKIT_PEG_SEQ2(SKIT_PEG_SEQ6(a,b,c,d,e,f),          SKIT_PEG_ZERO_OR_MORE6(a,b,c,d,e,f))
#define SKIT_PEG_ONE_OR_MORE7(a,b,c,d,e,f,g)         SKIT_PEG_SEQ2(SKIT_PEG_SEQ7(a,b,c,d,e,f,g),        SKIT_PEG_ZERO_OR_MORE7(a,b,c,d,e,f,g))
#define SKIT_PEG_ONE_OR_MORE8(a,b,c,d,e,f,g,h)       SKIT_PEG_SEQ2(SKIT_PEG_SEQ8(a,b,c,d,e,f,g,h),      SKIT_PEG_ZERO_OR_MORE8(a,b,c,d,e,f,g,h))
#define SKIT_PEG_ONE_OR_MORE9(a,b,c,d,e,f,g,h,i)     SKIT_PEG_SEQ2(SKIT_PEG_SEQ9(a,b,c,d,e,f,g,h,i),    SKIT_PEG_ZERO_OR_MORE9(a,b,c,d,e,f,g,h,i))
#define SKIT_PEG_ONE_OR_MORE10(a,b,c,d,e,f,g,h,i,j)  SKIT_PEG_SEQ2(SKIT_PEG_SEQ10(a,b,c,d,e,f,g,h,i,j), SKIT_PEG_ZERO_OR_MORE10(a,b,c,d,e,f,g,h,i,j))

#define SKIT_PEG_ONE_OR_MORE(...) SKIT_MACRO_DISPATCHER(SKIT_PEG_ONE_OR_MORE, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_CHOOSE1(a) SKIT_PEG_SEQ1(a)

#define SKIT_PEG_CHOOSE2(a,b) \
	do { \
		ssize_t stored_cursor = new_cursor; \
		/* Given how PEGs work, the error from the branch that made it */ \
		/*   farthest into the text is more likely to be the most /specific/ */ \
		/*   error message and therefore more likely to be the most /useful/ */ \
		/*   error message.  It requires some copying/allocation to */ \
		/*   implement this naively, but hopefully it is worth it. */ \
		/* TODO: HACK: lots of manual copying of error information.  It will miss stuff. */ \
		SKIT_LOAF_ON_STACK(prev_err_msg_buf, 128); \
		skit_slice prev_err_msg = skit_slice_null(); \
		skit_peg_parse_match prev_match;\
		\
		a; \
		if ( match.successful ) \
		{ \
			new_cursor = match.end; \
			break; \
		} \
		\
		prev_match = match; \
		prev_err_msg = skit_loaf_store_slice(&prev_err_msg_buf, parser->last_error_msg); \
		\
		new_cursor = stored_cursor; \
		SKIT_PEG__BRANCH_DISCARD(new_cursor); \
		\
		b; \
		if ( match.successful ) \
		{ \
			new_cursor = match.end; \
			skit_loaf_free(&prev_err_msg_buf); \
			break; \
		} \
		\
		/* Reaching this point means that an error occured. */ \
		/* As outlined earlier on, we will prefer the error that reached the farthest. */ \
		if ( prev_match.begin > match.begin ) \
		{ \
			parser->last_error_msg = skit_loaf_store_slice(&parser->last_error_msg_buf, prev_err_msg); \
			match = prev_match; \
		} \
		skit_loaf_free(&prev_err_msg_buf); \
		\
		new_cursor = stored_cursor; \
		/* NOTE: do not stick SKIT_PEG__BRANCH_DISCARD here. */ \
		/* At this point, it is simply a failing rule, like how SEQ would fail. */ \
		/* If there are more branches, then those will immediately call */ \
		/* SKIT_PEG__BRANCH_DISCARD once this rule returns. */ \
		/* If there aren't any more branches, then that means the whole */ \
		/* grammar failed and the entry point can issue a branch failure. */ \
		break; \
	} while(0)

#define SKIT_PEG_CHOOSE3(a,b,c)                 SKIT_PEG_CHOOSE2(a,                           SKIT_PEG_CHOOSE2(b,c))
#define SKIT_PEG_CHOOSE4(a,b,c,d)               SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE2(a,b),       SKIT_PEG_CHOOSE2(c,d))
#define SKIT_PEG_CHOOSE5(a,b,c,d,e)             SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE3(a,b,c),     SKIT_PEG_CHOOSE2(d,e))
#define SKIT_PEG_CHOOSE6(a,b,c,d,e,f)           SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE3(a,b,c),     SKIT_PEG_CHOOSE3(d,e,f))
#define SKIT_PEG_CHOOSE7(a,b,c,d,e,f,g)         SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE4(a,b,c,d),   SKIT_PEG_CHOOSE3(e,f,g))
#define SKIT_PEG_CHOOSE8(a,b,c,d,e,f,g,h)       SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE4(a,b,c,d),   SKIT_PEG_CHOOSE4(e,f,g,h))
#define SKIT_PEG_CHOOSE9(a,b,c,d,e,f,g,h,i)     SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE5(a,b,c,d,e), SKIT_PEG_CHOOSE4(f,g,h,i))
#define SKIT_PEG_CHOOSE10(a,b,c,d,e,f,g,h,i,j)  SKIT_PEG_CHOOSE2(SKIT_PEG_CHOOSE5(a,b,c,d,e), SKIT_PEG_CHOOSE5(f,g,h,i,j))

#define SKIT_PEG_CHOOSE(...) SKIT_MACRO_DISPATCHER(SKIT_PEG_CHOOSE, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_OPTIONAL1(a) \
	do { \
		ssize_t stored_cursor = new_cursor; \
		a; \
		if ( match.successful ) \
			new_cursor = match.end; \
		else \
		{ \
			new_cursor = stored_cursor; \
			/* Notify the caller of the failure of the optional branch. */ \
			SKIT_PEG__BRANCH_DISCARD(new_cursor); \
			match = skit_peg_match_success(parser, new_cursor, new_cursor); \
		} \
	} while(0)

#define SKIT_PEG_OPTIONAL2(a,b)                   SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ2(a,b))
#define SKIT_PEG_OPTIONAL3(a,b,c)                 SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ3(a,b,c))
#define SKIT_PEG_OPTIONAL4(a,b,c,d)               SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ4(a,b,c,d))
#define SKIT_PEG_OPTIONAL5(a,b,c,d,e)             SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ5(a,b,c,d,e))
#define SKIT_PEG_OPTIONAL6(a,b,c,d,e,f)           SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ6(a,b,c,d,e,f))
#define SKIT_PEG_OPTIONAL7(a,b,c,d,e,f,g)         SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ7(a,b,c,d,e,f,g))
#define SKIT_PEG_OPTIONAL8(a,b,c,d,e,f,g,h)       SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ8(a,b,c,d,e,f,g,h))
#define SKIT_PEG_OPTIONAL9(a,b,c,d,e,f,g,h,i)     SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ9(a,b,c,d,e,f,g,h,i))
#define SKIT_PEG_OPTIONAL10(a,b,c,d,e,f,g,h,i,j)  SKIT_PEG_OPTIONAL1(SKIT_PEG_SEQ10(a,b,c,d,e,f,g,h,i,j))

#define SKIT_PEG_OPTIONAL(...) SKIT_MACRO_DISPATCHER(SKIT_PEG_OPTIONAL, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_ACTION(a) \
	do { \
		match = skit_peg_match_success(parser, cursor, new_cursor); \
		a; \
	} while(0)

#define SKIT_PEG_RULE_N1(rule_name) \
	do { \
		match = SKIT_PEG_ ## rule_name ( parser, new_cursor, ubound); \
	} while(0)

#define SKIT_PEG_RULE_N2(rule_name, ...) \
	do { \
		match = SKIT_PEG_ ## rule_name ( parser, new_cursor, ubound, __VA_ARGS__); \
	} while(0)

#define SKIT_PEG_RULE(...) \
	SKIT_MACRO_DISPATCHER2(SKIT_PEG_RULE_N, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_PEG_NEG_LOOKAHEAD(a) \
	do { \
		ssize_t stored_cursor = new_cursor; \
		\
		a; \
		\
		/* This is a lookahead, so we unconditionally discard any parsing action: */ \
		new_cursor = stored_cursor; \
		SKIT_PEG__BRANCH_DISCARD(new_cursor); \
		\
		if ( match.successful ) \
		{ \
			skit_slice invalid_elem = skit_slice_of(match.input, match.begin, match.end); \
			match = skit_peg_match_failure( parser, new_cursor, "Invalid syntax element: %.*s", sSLENGTH(invalid_elem), sSPTR(invalid_elem) ); \
			break; \
		} \
		\
		match = skit_peg_match_success( parser, new_cursor, new_cursor ); \
	} while(0)

#define SKIT_PEG_ASSERT_PARSE_PASS(parser_arg, str, ...) \
	do { \
		/* If we try to use (parser_arg) everywhere in the macro and the caller */ \
		/* of this macro passes anything besides a (skit_peg_parser*) then */ \
		/* they will get this vague error message (from GCC): */ \
		/* "error: request for member 'input' in something not a structure or union" */ \
		/* This can easily happen if they use the wrong level of indirection, */ \
		/* ex: SKIT_PEG_ASSERT_PARSE_PASS(&my_structure->parser, ...) */ \
		/*     instead of */ \
		/*     SKIT_PEG_ASSERT_PARSE_PASS(my_structure->parser, ...) */ \
		/* To make this mistake easier to find, we expand the argument */ \
		/* only once and pass it into a variable.  This also avoids */ \
		/* side-effectful expressions being evaluated more than once. */ \
		/* The error message then becomes something like this: */ \
		/* "warning: initialization makes pointer from integer without a cast" */ \
		/* Further design note: DO NOT name this variable 'parser'. */ \
		/* If the caller invokes the macro with a variable named "parser" */ \
		/* passed into 'parser_arg', then the below statement becomes */ \
		/* 'skit_peg_parser *parser = parser;' which has an undefined value */ \
		/* after the assignment, and will almost certainly segfault. */ \
		skit_peg_parser *skit__assert_parser = (parser_arg); \
		/* Another note: Assign this back into a variable named */ \
		/* 'parser', because the later SKIT_PEG_RULE expects that to exist. */ \
		/* This is safe, as long as we don't write */ \
		/* 'skit_peg_parser *parser = parser;'.  The temporary assignment */ \
		/* to another variable prevents macro expansions from creating */ \
		/* nonsensicle statements with undefined behavior, because the */ \
		/* temporary variable is (nearly) guaranteed to be in-scope. */ \
		/* If we don't do this, then test functions that don't define a */ \
		/* 'parser' variable anywhere will get this error message: */ \
		/* "error: 'parser' undeclared (first use in this function)" */ \
		skit_peg_parser *parser = skit__assert_parser; \
		\
		skit_peg_parser_set_text(parser,sSLICE((str))); \
		\
		SKIT_PEG_PARSING_INITIAL_VARS(parser); \
		\
		/* __VA_ARGS__ contains rule_name. */ \
		/* This is contrived, but it allows zero-arg rules to be invoked. */ \
		SKIT_PEG_RULE(__VA_ARGS__); \
		\
		sASSERT_MSGF( match.successful, "Parser failed to match the input \"%.*s\", error as follows: %.*s", \
			sSLENGTH(parser->input),          sSPTR(parser->input), \
			sSLENGTH(parser->last_error_msg), sSPTR(parser->last_error_msg)); \
		sASSERT_EQ( match.end, sSLENGTH(parser->input) ); \
		\
	} while(0)

#define SKIT_PEG_ASSERT_PARSE_FAIL(parser_arg, str, ...) \
	do { \
		/* NOTE: we intentionally assign to a variable, rather than expanding (parser_arg) everywhere. */ \
		/* See SKIT_PEG_ASSERT_PASS for a more thorough explanation. */ \
		skit_peg_parser *skit__assert_parser = (parser_arg); \
		skit_peg_parser *parser = skit__assert_parser; \
		\
		sTRACE(skit_peg_parser_set_text(parser, sSLICE((str)))); \
		\
		SKIT_PEG_PARSING_INITIAL_VARS(parser); \
		\
		/* __VA_ARGS__ contains rule_name. */ \
		/* This is contrived, but it allows zero-arg rules to be invoked. */ \
		SKIT_PEG_RULE(__VA_ARGS__); \
		\
		sASSERT( !match.successful ); \
		\
	} while(0)

/* Use this to ensure that rules aren't over-aggressive when calculating how long a match is. */
#define SKIT_PEG_ASSERT_PARSE_PART(parser_arg, str_to_parse, str_after, ...) \
	do { \
		/* NOTE: we intentionally assign to a variable, rather than expanding (parser_arg) everywhere. */ \
		/* See SKIT_PEG_ASSERT_PASS for a more thorough explanation. */ \
		skit_peg_parser *skit__assert_parser = (parser_arg); \
		skit_peg_parser *parser = skit__assert_parser; \
		\
		char parser_input[sizeof((str_to_parse)) + sizeof((str_after)) + 1]; \
		memcpy(parser_input, (str_to_parse), (sizeof((str_to_parse))-1)); \
		memcpy(parser_input + (sizeof((str_to_parse))-1), str_after, sizeof((str_after))); \
		\
		sTRACE(skit_peg_parser_set_text(parser, skit_slice_of_cstrn(parser_input, sizeof(parser_input)-1))); \
		\
		SKIT_PEG_PARSING_INITIAL_VARS(parser); \
		\
		/* __VA_ARGS__ contains rule_name. */ \
		/* This is contrived, but it allows zero-arg rules to be invoked. */ \
		SKIT_PEG_RULE(__VA_ARGS__); \
		\
		sASSERT( match.successful ); \
		sASSERT_EQ( match.end, sizeof((str_to_parse))-1 ); \
		\
	} while(0)

#endif
