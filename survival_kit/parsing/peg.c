
#if defined(__DECC)
#pragma module skit_parsing_peg
#endif

#include "survival_kit/parsing/peg.h"
#include "survival_kit/parsing/peg_macros.h"
#include "survival_kit/parsing/peg_shorthand.h"

#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <pthread.h>

#include "survival_kit/feature_emulation.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/pfile_stream.h"
#include "survival_kit/string.h"
#include "survival_kit/memory.h"
#include "survival_kit/assert.h"
#include "survival_kit/math.h"
#include "survival_kit/macro.h"
#include "survival_kit/trie.h"

#define NUM_NEXT_CHARS 40


uint8_t *skit_peg_default_word_char_tbl = NULL;
const static ssize_t skit_peg_default_word_char_tlen = 128;

/* ------------------------------------------------------------------------- */

void skit_peg_parser_ctor( skit_peg_parser *parser, skit_slice text_to_parse, skit_stream *debug_out )
{
	parser->input = text_to_parse;
	parser->last_error_msg_buf = skit_loaf_alloc(1024);
	parser->last_error_msg = skit_slice_of(parser->last_error_msg_buf.as_slice, 0, 0);
	parser->debug_out = debug_out;
	parser->caller_context = NULL;
	parser->case_sensitive = 1;
	parser->is_word_char_table = skit_peg_default_word_char_tbl;
	parser->is_word_char_table_len = skit_peg_default_word_char_tlen;
	parser->is_word_char_method = &skit_peg_is_word_char_method;
	parser->parse_whitespace = &skit_peg_parse_whitespace;
	parser->branch_discard = NULL;
}

skit_peg_parser *skit_peg_parser_new( skit_slice text_to_parse, skit_stream *debug_out )
{
	skit_peg_parser *result = skit_malloc(sizeof(skit_peg_parser));
	skit_peg_parser_ctor(result, text_to_parse, debug_out);
	return result;
}

skit_peg_parser *skit_peg_parser_mock_new( skit_slice text_to_parse )
{
	return skit_peg_parser_new(text_to_parse, skit_stream_stdout);
}

void skit_peg_parser_dtor(skit_peg_parser *parser)
{
	parser->input = skit_slice_null();
	parser->last_error_msg_buf = skit_loaf_free(&parser->last_error_msg_buf);
	parser->last_error_msg = skit_slice_null();
}

skit_peg_parser *skit_peg_parser_free(skit_peg_parser *parser)
{
	skit_peg_parser_dtor(parser);
	skit_free(parser);
	return NULL;
}

skit_peg_parser *skit_peg_parser_mock_free(skit_peg_parser *parser)
{
	return skit_peg_parser_free(parser);
}

/* ------------------------------------------------------------------------- */

skit_peg_lookup_index *skit_peg_lookup_index_new()
{
	skit_peg_lookup_index *index = skit_malloc(sizeof(skit_peg_lookup_index));
	skit_peg_lookup_index_ctor(index);
	return index;
}

void skit_peg_lookup_index_ctor( skit_peg_lookup_index *index )
{
	SKIT_USE_FEATURE_EMULATION;
	sCTRACE(pthread_mutexattr_init(&index->mutex_attrs));
	sCTRACE(pthread_mutexattr_settype(&index->mutex_attrs, PTHREAD_MUTEX_ERRORCHECK));
	sCTRACE(pthread_mutex_init(&index->mutex, &index->mutex_attrs));

	index->suffix_table = NULL;
	skit_trie_ctor(&index->trie);
}

skit_peg_lookup_index *skit_peg_lookup_index_free( skit_peg_lookup_index *index )
{
	skit_peg_lookup_index_dtor(index);
	return NULL;
}

void skit_peg_lookup_index_dtor( skit_peg_lookup_index *index )
{
	SKIT_USE_FEATURE_EMULATION;
	sCTRACE(pthread_mutex_destroy(&index->mutex));
	sCTRACE(pthread_mutexattr_destroy(&index->mutex_attrs));

	if ( index->suffix_table != NULL )
	{
		skit_free(index->suffix_table);
		index->suffix_table = NULL;
	}

	skit_trie_dtor(&index->trie);
}

/* ------------------------------------------------------------------------- */

void skit_peg_parser_set_text(skit_peg_parser *parser, skit_slice text_to_parse)
{
	sASSERT(parser != NULL);
	parser->input = text_to_parse;
}

skit_peg_parse_match skit_peg_match_success(skit_peg_parser *parser, size_t begin, size_t end)
{
	skit_peg_parse_match m;
	m.successful = 1;
	m.input = parser->input;
	m.begin = begin;
	m.end = end;
	return m;
}

skit_peg_parse_match skit_peg_match_failure(skit_peg_parser *parser, ssize_t position, const char *fail_msg, ...)
{
	skit_peg_parse_match m;
	m.successful = 0;
	m.input = parser->input;
	m.begin = position;
	m.end = -1;
	
	va_list vl;
	va_start(vl, fail_msg);
	ssize_t n_bytes_written = vsnprintf(
		(char*)sLPTR(parser->last_error_msg_buf),
		sLLENGTH(parser->last_error_msg_buf),
		fail_msg, vl);
	va_end(vl);
	
	n_bytes_written = SKIT_MIN(sLLENGTH(parser->last_error_msg_buf), n_bytes_written);
	parser->last_error_msg = skit_slice_of(parser->last_error_msg_buf.as_slice, 0, n_bytes_written);
	return m;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_peg_next_chars_in_parse(skit_peg_parser *parser, ssize_t cursor, ssize_t nchars)
{
	ssize_t end = cursor + nchars;
	if ( end > sSLENGTH(parser->input) )
		end = sSLENGTH(parser->input);
	
	return skit_slice_of(parser->input, cursor, end);
}

/* ------------------------------------------------------------------------- */

ssize_t skit_peg_parse_one_newline(skit_utf8c *txt, ssize_t cursor, ssize_t ubound)
{
	ssize_t remaining = (ubound - cursor);
	if      ( remaining >= 2 && (txt[cursor] == '\r' && txt[cursor+1] == '\n') )
		return cursor+2;
	else if ( remaining >= 1 && (txt[cursor] == '\n' || txt[cursor] == '\r') )
		return cursor+1;
	return cursor; /* No match. */
}

static void skit_peg_parse_one_newline_test()
{
	SKIT_USE_FEATURE_EMULATION;

	skit_utf8c mline[] = "x\n;y\r\n;z";
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  0, sizeof(mline)),  0);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  1, sizeof(mline)),  2);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  2, sizeof(mline)),  2);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  3, sizeof(mline)),  3);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  4, sizeof(mline)),  6);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  5, sizeof(mline)),  6);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  6, sizeof(mline)),  6);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  7, sizeof(mline)),  7);
	sASSERT_EQ(skit_peg_parse_one_newline(mline,  8, sizeof(mline)),  8);
	
	printf("  skit_peg_parse_one_newline_test passed.\n");
}

int skit_parsing_is_whitespace( skit_utf8c c )
{
	if ( c == ' ' || c == '\t' || c == '\n' || c == '\r' )
		return 1;
	else
		return 0;
}

ssize_t skit_peg_parse_whitespace(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound)
{
	skit_utf8c *ptr = sSPTR(parser->input);
	ssize_t new_cursor = cursor;
	
	while ( new_cursor < ubound )
	{
		if ( !skit_parsing_is_whitespace(ptr[new_cursor]) )
			break;
		
		new_cursor++;
	}
	
	return new_cursor;
}

static void skit_peg_whitespace_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("A B\tC  D\r\nE\n\rF\rH"));
	ssize_t ubound = sSLENGTH(parser->input);
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  0, ubound),  0); /* "A"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  1, ubound),  2); /* " "      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  2, ubound),  2); /* "B"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  3, ubound),  4); /* "\t"     */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  4, ubound),  4); /* "C"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  5, ubound),  7); /* "  "     */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  7, ubound),  7); /* "D"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser,  8, ubound), 10); /* "\r\n"   */
	sASSERT_EQ(skit_peg_parse_whitespace(parser, 10, ubound), 10); /* "E"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser, 11, ubound), 13); /* "\n\r"   */
	sASSERT_EQ(skit_peg_parse_whitespace(parser, 13, ubound), 13); /* "F"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser, 14, ubound), 15); /* "\r"     */
	sASSERT_EQ(skit_peg_parse_whitespace(parser, 15, ubound), 15); /* "H"      */
	sASSERT_EQ(skit_peg_parse_whitespace(parser, 16, ubound), 16); /* ""       */
	skit_peg_parser_mock_free(parser);
	
	printf("  skit_peg_whitespace_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_peg_parse_match SKIT_PEG_success( skit_peg_parser *parser, ssize_t cursor, ssize_t ubound )
{
	return skit_peg_match_success(parser, cursor, cursor);
}

skit_peg_parse_match SKIT_PEG_failure( skit_peg_parser *parser, ssize_t cursor, ssize_t ubound )
{
	return skit_peg_match_failure(parser, cursor, "Could not parse.");
}

static void skit_peg_macros_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("a"));
	SKIT_PEG_PARSING_INITIAL_VARS(parser);
	
	RULE(success);
	sASSERT_EQ(match.successful, 1);
	sASSERT_EQ(match.begin, 0);
	sASSERT_EQ(match.end, 0);

	RULE(failure);
	sASSERT_EQ(match.successful, 0);
	sASSERT_EQ(match.begin, 0);

	sASSERT_EQ(cursor, 0);
	sASSERT_EQ(new_cursor, 0);

	SEQ( RULE(success), RULE(success) );
	sASSERT_EQ(match.successful, 1);
	SEQ( RULE(success), RULE(failure) );
	sASSERT_EQ(match.successful, 0);
	SEQ( RULE(failure), RULE(success) );
	sASSERT_EQ(match.successful, 0);
	SEQ( RULE(failure), RULE(failure) );
	sASSERT_EQ(match.successful, 0);
	
	skit_peg_parser_mock_free(parser);
	
	printf("  skit_peg_macros_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_peg_parse_match SKIT_PEG_non_whitespace(
	skit_peg_parser *parser,
	ssize_t cursor,
	ssize_t ubound)
{
	while ( cursor < ubound && parser->parse_whitespace != NULL 
	&&      parser->parse_whitespace(parser, cursor, ubound) == 0 )
		cursor++;
	
	return skit_peg_match_success(parser, cursor, ubound);
}

/* ------------------------------------------------------------------------- */

int skit_peg_is_word_char_method( skit_peg_parser *parser, skit_utf32c c )
{
	if ( ('0' <= c && c <= '9') ||
	     ('A' <= c && c <= 'Z') || 
	     ('a' <= c && c <= 'z') ||
	     (c == '_') )
	{
		return 1;
	}
	
	return 0;
}

#define skit_peg_is_word_char(parser, c) \
	(((c) < (parser)->is_word_char_table_len) ? \
		((parser)->is_word_char_table[(c)]) : \
		((parser)->is_word_char_method((parser),(c))))

skit_peg_parse_match SKIT_PEG_word_boundary(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound)
{
	int word_before;
	int word_after;
	skit_utf8c *ptr = sSPTR(parser->input);
	
	if ( cursor >= ubound || !skit_peg_is_word_char(parser, ptr[cursor]) )
		word_after = 0;
	else
		word_after = 1;
	
	if ( cursor <= 0 || !skit_peg_is_word_char(parser, ptr[cursor-1]) )
		word_before = 0;
	else
		word_before = 1;
	
	if( (word_before && !word_after) || (!word_before && word_after) || (!word_before && !word_after) )
		return skit_peg_match_success(parser, cursor, cursor);
	else
		return skit_peg_match_failure(parser, cursor, "Expected word boundary.");
}

static void skit_peg_word_boundary_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("This is a word."));
	ssize_t ubound = sSLENGTH(parser->input);
	
	/* This */
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  0, ubound).successful,  1);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  1, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  2, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  3, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  4, ubound).successful,  1);
	
	/* is */
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  5, ubound).successful,  1);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  6, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  7, ubound).successful,  1);
	
	/* a */
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  8, ubound).successful,  1);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser,  9, ubound).successful,  1);
	
	/* word */
	sASSERT_EQ(SKIT_PEG_word_boundary(parser, 10, ubound).successful,  1);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser, 11, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser, 12, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser, 13, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_word_boundary(parser, 14, ubound).successful,  1);
	
	/* . */
	sASSERT_EQ(SKIT_PEG_word_boundary(parser, 15, ubound).successful,  1);
	
	skit_peg_parser_mock_free(parser);
	
	printf("  skit_peg_word_boundary_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_peg_parse_match SKIT_PEG_end_of_text(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound)
{
	skit_slice next_chars;
	if ( cursor < ubound )
	{
		next_chars = skit_peg_next_chars_in_parse(parser,cursor,NUM_NEXT_CHARS);
		return skit_peg_match_failure(parser, cursor, "Expected token end-of-text, instead got '%.*s'",
			sSLENGTH(next_chars), sSPTR(next_chars) );
	}

	return skit_peg_match_success(parser, cursor, cursor);
}

static void skit_peg_end_of_text_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = NULL;
	ssize_t ubound = 0;

	//
	parser = skit_peg_parser_mock_new(sSLICE("."));
	ubound = sSLENGTH(parser->input);
	sASSERT_EQ(SKIT_PEG_end_of_text(parser, 0, ubound).successful,  0);
	sASSERT_EQ(SKIT_PEG_end_of_text(parser, 1, ubound).successful,  1);
	skit_peg_parser_mock_free(parser);
	
	//
	parser = skit_peg_parser_mock_new(sSLICE(""));
	ubound = sSLENGTH(parser->input);
	sASSERT_EQ(SKIT_PEG_end_of_text(parser, 0, ubound).successful,  1);
	skit_peg_parser_mock_free(parser);
	
	printf("  skit_peg_end_of_text_test passed.\n");
}

/* ------------------------------------------------------------------------- */

skit_peg_parse_match skit__peg_parse_token(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound, skit_slice token)
{
	skit_slice next_chars;
	
	/* Make sure the token would even fit within the remaining text. */
	ssize_t token_length = sSLENGTH(token);
	if ( (ubound - cursor) < token_length )
	{
		next_chars = skit_slice_of(parser->input, cursor, ubound);
		return skit_peg_match_failure(parser, cursor, "Expected token %.*s, instead got '%.*s'",
			sSLENGTH(token),      sSPTR(token), 
			sSLENGTH(next_chars), sSPTR(next_chars) );
	}

	/* The text-to-text comparison. */
	skit_slice candidate = skit_slice_of(parser->input, cursor, cursor + token_length);
	if ( skit_slice_ascii_ccmp(candidate, token, parser->case_sensitive ) != 0 )
	{
		next_chars = skit_peg_next_chars_in_parse(parser,cursor,NUM_NEXT_CHARS);
		return skit_peg_match_failure(parser, cursor, "Expected token %.*s, instead got '%.*s'",
			sSLENGTH(token),      sSPTR(token),
			sSLENGTH(next_chars), sSPTR(next_chars) );
	}
	
	return skit_peg_match_success(parser, cursor, cursor + token_length);
}

skit_peg_parse_match skit__peg_parse_keyword(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound, skit_slice keyword)
{
	skit_peg_parse_match m;
	ssize_t new_cursor = cursor;
	
	/* Assert a word boundary */
	m = SKIT_PEG_word_boundary(parser, new_cursor, ubound);
	if ( !m.successful )
		return m;
	
	/* Parse the keyword itself. */
	m = skit__peg_parse_token(parser, new_cursor, ubound, keyword);
	if ( !m.successful )
		return m;
	
	new_cursor = m.end;
	/* Assert a word boundary */
	m = SKIT_PEG_word_boundary(parser, new_cursor, ubound);
	if ( !m.successful )
		return m;
	
	return skit_peg_match_success(parser, cursor, new_cursor);
}

/* ------------------------------------------------------------------------- */

skit_peg_parse_match SKIT_PEG_any_word(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound,
	const char *description,
	skit_slice *result,
	char **buffer_ref )
{
	ssize_t new_cursor = cursor;
	
	*result = skit_slice_null();
	*buffer_ref = NULL;
	
	if ( description == NULL )
		description = "word";
	
	// Error detection.
	skit_slice next_chars;
	if ( new_cursor >= ubound )
		return skit_peg_match_failure(parser, cursor, "Expected %s, instead got end-of-text.", description);
	
	skit_utf8c *text = sSPTR(parser->input);
	if ( !skit_peg_is_word_char(parser, text[new_cursor]) )
	{
		next_chars = skit_peg_next_chars_in_parse(parser,cursor,NUM_NEXT_CHARS);
		return skit_peg_match_failure(parser, cursor, "Expected %s, instead got %.*s.",
			description, sSLENGTH(next_chars), sSPTR(next_chars));
	}
	
	// Now we iterate over the word.
	new_cursor++;

	while ( new_cursor < ubound && skit_peg_is_word_char(parser, text[new_cursor]) )
		new_cursor++;
	
	// The word we just matched, by definition, must be /preceded/ by a word
	// boundary.  Note that we are checking this word boundary at 'cursor'
	// and not at 'new_cursor'.  We do it later in the parse so that we
	// have more information for error printing if this happens.
	if ( cursor > 0 && skit_peg_is_word_char(parser, text[cursor-1]) )
	{
		skit_slice text = skit_slice_of(parser->input, cursor, new_cursor);
		return skit_peg_match_failure(parser, cursor, 
			"'%.*s' should not be preceded by word characters, but it is.",
			sSLENGTH(text), sSPTR(text));
	}
	
	*result = skit_slice_of(parser->input, cursor, new_cursor);
	
	return skit_peg_match_success(parser, cursor, new_cursor);
}

static void skit_peg_any_word_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("This is a word."));
	ssize_t ubound = sSLENGTH(parser->input);
	skit_slice word = skit_slice_null();
	char *word_buf = NULL;
#define any_word_test(pos, expected_str, expected_success) \
	do { \
		sASSERT_EQ(SKIT_PEG_any_word(parser, pos, ubound, "any word", &word, &word_buf).successful, expected_success); \
		\
		if ( expected_success && expected_str != NULL ) \
		{ \
			sASSERT( !skit_slice_is_null(word) ); \
			sASSERT_EQS(word, sSLICE(expected_str)); \
		} \
		if ( word_buf != NULL ) \
		{ \
			skit_free(word_buf); \
			word_buf = NULL; \
		} \
	} while(0)
	
	/* This */
	any_word_test( 0, "This", 1);
	any_word_test( 1, NULL,   0);
	any_word_test( 2, NULL,   0);
	any_word_test( 3, NULL,   0);
	any_word_test( 4, NULL,   0);
	
	/* is */
	any_word_test( 5, "is",   1);
	any_word_test( 6, NULL,   0);
	any_word_test( 7, NULL,   0);
	
	/* a */
	any_word_test( 8, "a",    1);
	any_word_test( 9, NULL,   0);
	
	/* word */
	any_word_test(10, "word", 1);
	any_word_test(11, NULL,   0);
	any_word_test(12, NULL,   0);
	any_word_test(13, NULL,   0);
	any_word_test(14, NULL,   0);
	
	/* . */
	any_word_test(15, NULL,   0);
	
	
	/* Simple case: a single word with end-of-text on both ends. */
	parser->input = sSLICE("word");
	any_word_test( 0, "word", 1);
	any_word_test( 1, NULL,   0);
	any_word_test( 2, NULL,   0);
	any_word_test( 3, NULL,   0);
	
	/* A single character. */
	parser->input = sSLICE("x");
	any_word_test( 0, "x", 1);
	
	skit_peg_parser_mock_free(parser);
	
	printf("  skit_peg_any_word_test passed.\n");
}

/* ------------------------------------------------------------------------- */

DEFINE_RULE(lookup_test, skit_peg_lookup_index *index, int *result)
	SKIT_USE_FEATURE_EMULATION;

	#define LOOKUP_CHOICE(INDEX, CHOICE, SUFFIX) \
		INDEX(index) \
		CHOICE(x,       ACTION( (*result) = 1; )) \
		CHOICE(foobar,  ACTION( (*result) = 2; )) \
		CHOICE(foo,     ACTION( (*result) = 3; )) \
		CHOICE(FOO,     ACTION( (*result) = 4; ))
	#include "survival_kit/parsing/peg_lookup.h"
	#undef LOOKUP_CHOICE

END_RULE

DEFINE_RULE(suffix_test, skit_peg_lookup_index *index, int *result)
	SKIT_USE_FEATURE_EMULATION;
	
	#define LOOKUP_CHOICE(INDEX, CHOICE, SUFFIX) \
		SUFFIX(bar,     RULE(keyword, "bar")) \
		CHOICE(x,       ACTION( (*result) = 1; )) \
		CHOICE(foobar,  ACTION( (*result) = 2; )) \
		SUFFIX(baz,     RULE(keyword, "baz")) \
		CHOICE(foo,     ACTION( (*result) = 3; )) \
		CHOICE(FOO,     ACTION( (*result) = 4; ))
	#include "survival_kit/parsing/peg_lookup.h"
	#undef LOOKUP_CHOICE	
END_RULE

static void skit_peg_lookup_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = skit_peg_parser_mock_new(skit_slice_null());
	SKIT_PEG_PARSING_INITIAL_VARS(parser);
	
	int result;
	
	// --------- Normal, non-SUFFIX test. ---------
	skit_peg_lookup_index *index = sETRACE(skit_peg_lookup_index_new());
	
	result = 0;
	sASSERT_PARSE_PASS(parser, "x", lookup_test, index, &result);
	sASSERT_EQ(result, 1);

	result = 0;
	sASSERT_PARSE_PASS(parser, "foobar", lookup_test, index, &result);
	sASSERT_EQ(result, 2);

	result = 0;
	sASSERT_PARSE_PASS(parser, "foo", lookup_test, index, &result);
	sASSERT_EQ(result, 3);

	result = 0;
	sASSERT_PARSE_PASS(parser, "FOO", lookup_test, index, &result);
	sASSERT_EQ(result, 4);
	
	sTRACE(skit_peg_lookup_index_free(index));
	
	// --------- SUFFIX test. ---------
	index = sETRACE(skit_peg_lookup_index_new());
	
	result = 0;
	sASSERT_PARSE_PASS(parser, "x bar", suffix_test, index, &result);
	sASSERT_EQ(result, 1);

	result = 0;
	sASSERT_PARSE_PASS(parser, "foobar bar", suffix_test, index, &result);
	sASSERT_EQ(result, 2);

	result = 0;
	sASSERT_PARSE_PASS(parser, "foo baz", suffix_test, index, &result);
	sASSERT_EQ(result, 3);

	result = 0;
	sASSERT_PARSE_PASS(parser, "FOO baz", suffix_test, index, &result);
	sASSERT_EQ(result, 4);
	
	sTRACE(skit_peg_lookup_index_free(index));
	
	sTRACE(skit_peg_parser_mock_free(parser));
	
	printf("  skit_peg_lookup_test passed.\n");
}

/* ------------------------------------------------------------------------- */

static void skit_peg_bd_test_callback( skit_peg_parser *parser, ssize_t cursor_reset_pos )
{
	int *which_discard = (int*)parser->caller_context;
	
	switch( *which_discard )
	{
		// branch_discard_1
		case 0: sASSERT_EQ( cursor_reset_pos, 1 ); break;
		case 1: sASSERT_EQ( cursor_reset_pos, 1 ); break;
		case 2: sASSERT_EQ( cursor_reset_pos, 1 ); break;
		case 3: sASSERT_EQ( cursor_reset_pos, 2 ); break;
		case 4: sASSERT_EQ( cursor_reset_pos, 3 ); break;
		
		// branch_discard_2
		case 5: sASSERT_EQ( cursor_reset_pos, 4 ); break;
		case 6: sASSERT_EQ( cursor_reset_pos, 9 ); break;
		
		// branch_discard_3
		case 7: sASSERT_EQ( cursor_reset_pos, 1 ); break;
		case 8: sASSERT_EQ( cursor_reset_pos, 0 ); break;
		
		// There should be no other discards.
		default: sASSERT(0);
	}
	
	(*which_discard)++;
}

DEFINE_RULE(branch_discard_1)
	auto_consume_whitespace = 0;
	SEQ(
		RULE(token, "a"),
		CHOOSE(
			RULE(token, "x"), // which_discard == 0
			RULE(token, "y"), // which_discard == 1
			RULE(token, "z"), // which_discard == 2
			RULE(token, "b")
		),
		OPTIONAL(RULE(token,"q")), // which_discard == 3
		RULE(token,"c"),
		NEG_LOOKAHEAD(RULE(token,"1")), // which_discard == 4
		RULE(token,"d")
	);
END_RULE

DEFINE_RULE(branch_discard_2)
	auto_consume_whitespace = 0;
	SEQ(
		ZERO_OR_MORE(RULE(token, "a")), // which_discard == 5
		RULE(token,"b"),
		ONE_OR_MORE(RULE(token, "a"))   // which_discard == 6
	);
END_RULE

DEFINE_RULE(branch_discard_3)
	auto_consume_whitespace = 0;
	CHOOSE(
		SEQ(
			RULE(token, "a"),
			CHOOSE(
				RULE(token, "x"), // which_discard == 7
				RULE(token, "y")
			)
		), // which_discard == 8
		RULE(token, "abc")
	);
END_RULE

static void skit_peg_branch_discard_test()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_peg_parser *parser = skit_peg_parser_mock_new(skit_slice_null());
	SKIT_PEG_PARSING_INITIAL_VARS(parser);
	int which_discard = 0;
	parser->caller_context = &which_discard;
	parser->branch_discard = &skit_peg_bd_test_callback;
	
	sASSERT_PARSE_PASS(parser, "abcd",      branch_discard_1);
	sASSERT_PARSE_PASS(parser, "aaaabaaaa", branch_discard_2);
	sASSERT_PARSE_PASS(parser, "abc",       branch_discard_3);
	
	sTRACE(skit_peg_parser_mock_free(parser));
	
	printf("  skit_peg_branch_discard_test passed.\n");
}

/* ------------------------------------------------------------------------- */

static int skit_peg_module_initialized = 0;
pthread_mutex_t      skit_peg__lookup_mutex;
pthread_mutexattr_t  skit_peg__lookup_mutex_attrs;

void skit_peg_module_init()
{
	SKIT_USE_FEATURE_EMULATION;
	
	if ( skit_peg_module_initialized )
		return;

	skit_peg_default_word_char_tbl = skit_malloc(skit_peg_default_word_char_tlen);
	ssize_t i = 0;
	for ( i = 0; i < skit_peg_default_word_char_tlen; i++ )
		skit_peg_default_word_char_tbl[i] = skit_peg_is_word_char_method(NULL, i);
	
	sCTRACE(pthread_mutexattr_init(&skit_peg__lookup_mutex_attrs));
	sCTRACE(pthread_mutexattr_settype(&skit_peg__lookup_mutex_attrs, PTHREAD_MUTEX_ERRORCHECK));
	sCTRACE(pthread_mutex_init(&skit_peg__lookup_mutex, &skit_peg__lookup_mutex_attrs));
	
	skit_peg_module_initialized = 1;
}

/* ------------------------------------------------------------------------- */

void skit_peg_unittests()
{
	SKIT_USE_FEATURE_EMULATION;
	printf("skit_peg_parser_unittests()\n");
	sTRACE(skit_peg_macros_test());
	sTRACE(skit_peg_parse_one_newline_test());
	sTRACE(skit_peg_whitespace_test());
	sTRACE(skit_peg_word_boundary_test());
	sTRACE(skit_peg_end_of_text_test());
	// TODO: token, keyword tests.
	sTRACE(skit_peg_any_word_test());
	sTRACE(skit_peg_lookup_test());
	sTRACE(skit_peg_branch_discard_test());
	printf("  skit_peg_parser_unittests all passed!\n");
	printf("\n");
}

/* ------------------------------------------------------------------------- */