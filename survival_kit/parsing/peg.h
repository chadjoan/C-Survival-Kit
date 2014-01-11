
#ifndef SKIT_PARSING_PEG_INCLUDED
#define SKIT_PARSING_PEG_INCLUDED

#include "survival_kit/streams/stream.h"
#include "survival_kit/string.h"

typedef struct skit_peg_parser skit_peg_parser;
struct skit_peg_parser
{
	skit_slice          input;
	skit_loaf           last_error_msg_buf;
	skit_slice          last_error_msg;
	skit_stream         *debug_out;
	
	/// Allows the caller to embed caller-specific state into the parser.
	/// This is important, because the DEFINE_RULE/END_RULE and RULE()
	/// macros do not provide an easy way to swap parser implementations.
	/// This is NULL by default.
	void                *caller_ctx;
	
	/// Determines if string matching is case sensitive by default.
	/// The caller can always create elements in their grammar that are
	/// exceptions to this setting by writing their own RULEs that
	/// implement string comparisons differently.
	/// (Defaults to 1)
	int                 case_sensitive; 
	
	/// This table is consulted first when the parser needs to decide
	/// whether or not a character counts as a "word" character.
	/// A default (read-only) table will be used if the caller never
	/// assigns to this variable.
	/// If a character has a value too high to be in the table, then it will
	/// be passed into the parser->is_word_char_method function instead.
	uint8_t             *is_word_char_table;
	ssize_t             is_word_char_table_len;
	
	int (*is_word_char_method)( skit_peg_parser *parser, skit_utf32c c );
	
	/// This is what the parser will use by default to consume whitespace.
	/// By default, it is set to &skit_peg_parse_whitespace.
	/// The caller may replace this with a different whitespace parser as a
	///   way to deal with more complicated whitespace requirements, ex:
	///   - When comments are considered part of whitespace.
	///   - Disabling automatic whitespace parsing globally in the grammar.
	///
	ssize_t (*parse_whitespace)(
			skit_peg_parser *parser,
			ssize_t cursor,
			ssize_t ubound ); 
};

typedef struct skit_peg_parse_match skit_peg_parse_match;
struct skit_peg_parse_match
{
	int          successful;
	skit_slice   input;
	ssize_t      begin, end;
};

void skit_peg_parser_ctor( skit_peg_parser *parser, skit_slice text_to_parse, skit_stream *debug_out );
void skit_peg_parser_dtor( skit_peg_parser *parser );

skit_peg_parser *skit_peg_parser_new( skit_slice text_to_parse, skit_stream *debug_out );
skit_peg_parser *skit_peg_parser_free(skit_peg_parser *parser);

skit_peg_parser *skit_peg_parser_mock_new( skit_slice text_to_parse );
skit_peg_parser *skit_peg_parser_mock_free( skit_peg_parser *parser );

/// Set's the text that the parser should parse, and updates any related
/// internal state that the parser needs.
/// This should never be called during parsing; call it BEFORE parsing.
void skit_peg_parser_set_text(skit_peg_parser *parser, skit_slice text_to_parse);

skit_peg_parse_match skit_peg_match_success(skit_peg_parser *parser, size_t begin, size_t end);
skit_peg_parse_match skit_peg_match_failure(skit_peg_parser *parser, ssize_t position, const char *fail_msg, ...);

/// Rules that either always succeed or always fail, and do nothing else.
/// These are useful for debugging primarily, or sometimes for inserting the
/// analogue of a "no-op" into a parser.
/// Typical invocation:
///    SKIT_PEG_RULE(success) // Long notation.
///    RULE(success)          // Shorthand notation.
skit_peg_parse_match SKIT_PEG_success( skit_peg_parser *parser, ssize_t cursor, ssize_t ubound );
skit_peg_parse_match SKIT_PEG_failure( skit_peg_parser *parser, ssize_t cursor, ssize_t ubound ); ///

/// Returns the next 'nchars' characters in the text being parsed.
/// This is useful for generating error messages of the typical
/// "Expected X, instead got Y" nature.
skit_slice skit_peg_next_chars_in_parse(skit_peg_parser *parser, ssize_t cursor, ssize_t nchars);

/// This function is the default function placed into the 
///   parser->is_word_char_method callback.
int skit_peg_is_word_char_method( skit_peg_parser *parser, skit_utf32c c );

/// Parses one newline at the given cursor position in the given text.
/// Returns the new cursor position, or the original cursor position if there
/// was no match.
/// This function is useful for implement customized whitespace logic or
/// rules that need to handle newlines efficiently.
/// This is not callable as a SKIT_PEG_RULE (or RULE).  The function signature
/// has been kept simpler to make it more likely to be inlined or called in
/// a less expensive manner.
ssize_t skit_peg_parse_one_newline(skit_utf8c *txt, ssize_t cursor, ssize_t ubound);

int skit_parsing_is_whitespace( skit_utf8c c );

/// Returns the index (byte-wise) of the  character the parser would have to
/// position to in order to skip any whitespace at the current cursor position.
/// If there is no whitespace to skip, this will return the current cursor
/// position.
ssize_t skit_peg_parse_whitespace(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound);

/// Invoke as "RULE(word_boundary)" to determine if there is a word boundary
///   at the parser's current cursor position.
skit_peg_parse_match SKIT_PEG_word_boundary(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound);

// Internal use.
skit_peg_parse_match skit__peg_parse_token(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound, skit_slice token);
skit_peg_parse_match skit__peg_parse_keyword(skit_peg_parser *parser, ssize_t cursor, ssize_t ubound, skit_slice keyword);

/// RULE for matching the text given by 'token' at the parser's current cursor
///   position.
/// Example:
///   SEQ(
///       RULE(token,"*="),
///       ACTION( printf("Multiply-equal found.\n"); )
///   );
#define SKIT_PEG_token(input, cursor, ubound, token) \
	(skit__peg_parse_token((input), (cursor), (ubound), sSLICE((token))))

/// RULE for matching the text given by 'keyword' at the parser's current
///   cursor position, but only if it begins and ends with word boundaries.
/// Case sensitivity is determined by the parser->case_sensitive field.
/// Example:
///   SEQ(
///       RULE(keyword,"while"),
///       ACTION( printf("while keyword found.\n"); )
///   );
#define SKIT_PEG_keyword(input, cursor, ubound, keyword) \
	(skit__peg_parse_keyword((input), (cursor), (ubound), sSLICE((keyword))))

/// RULE for matching a single word at the parser's current cursor position.
/// A word is a sequence of word characters that is both preceded and followed
/// by non-word characters.
///
/// Example:
///   skit_slice  word      = skit_slice_null();
///   char        *word_buf = NULL;
///   SEQ(
///       RULE(any_word, "Attribute", &word, &word_buf),
///       ACTION( printf("Attribute matched: %.*s\n", sSLENGTH(word), sSPTR(word)); )
///   );
///   // If it errors, it will say something like:
///   // "Expected Attribute, instead got 'xyz...'"
skit_peg_parse_match SKIT_PEG_any_word(
	skit_peg_parser *parser,
	ssize_t cursor,
	ssize_t ubound,
	const char *description,
	skit_slice *result,
	char **buffer_ref );

void skit_peg_module_init();

void skit_peg_unittests();

#include "survival_kit/parsing/peg_macros.h"

#endif
