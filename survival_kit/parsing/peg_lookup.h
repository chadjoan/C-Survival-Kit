/**
This header file defines an X-macro that implements a fast N-way unordered
choice between N keywords.  This is potentially much more efficient than
doing N string comparisons worth of ordered choice when many possible
keywords might appear at a given place in a grammar.

Usage is like so:

	SKIT_PEG_DEFINE_RULE(my_lookup_rule, skit_trie *trie, int *result)
		SKIT_USE_FEATURE_EMULATION; // required.
	
		// LOOKUPs may not be nested within other PEG elements, so they must
		// be at the root of their own RULE.
		#define SKIT_PEG_LOOKUP_CHOICE(TRIE_TO_USE, CHOICE, SUFFIX) \
			TRIE_TO_USE(trie) \
			CHOICE(x,       SKIT_PEG_ACTION( (*result) = 1; )) \
			CHOICE(foobar,  SKIT_PEG_ACTION( (*result) = 2; )) \
			CHOICE(foo,     SKIT_PEG_ACTION( (*result) = 3; )) \
			CHOICE(FOO,     SKIT_PEG_ACTION( (*result) = 4; ))
		#include "survival_kit/parsing/peg_lookup.h"
		#undef SKIT_PEG_LOOKUP_CHOICE
	SKIT_PEG_END_RULE
	
	...
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("FOO"));
	skit_trie *trie = skit_trie_new();
	...
	
	int result = 0;
	SKIT_PEG_RULE(my_lookup_rule, trie, &result);
	sASSERT_EQ(result, 4);


Or, with shortand notation (#include "survival_kit/parsing/peg_shorthand.h")

	DEFINE_RULE(my_lookup_rule, skit_trie *trie, int *result)
		SKIT_USE_FEATURE_EMULATION; // required.

		// LOOKUPs may not be nested within other PEG elements, so they must
		// be at the root of their own RULE.
		#define LOOKUP_CHOICE(TRIE_TO_USE, CHOICE, SUFFIX) \
			TRIE_TO_USE(trie) \
			CHOICE(x,       ACTION( (*result) = 1; )) \
			CHOICE(foobar,  ACTION( (*result) = 2; )) \
			CHOICE(foo,     ACTION( (*result) = 3; )) \
			CHOICE(FOO,     ACTION( (*result) = 4; ))
		#include "survival_kit/parsing/peg_lookup.h"
		#undef LOOKUP_CHOICE
	END_RULE
	
	...
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("FOO"));
	skit_trie *trie = skit_trie_new();
	...
	
	int result = 0;
	SKIT_PEG_RULE(my_lookup_rule, trie, &result);
	sASSERT_EQ(result, 4);
*/

#ifdef SKIT_PARSING_PEG_SHORTAND_INCLUDED
#ifndef SKIT_PEG_LOOKUP_CHOICE
#define SKIT__PEG_LOOKUP_CHOICE_STUB 1
#define SKIT_PEG_LOOKUP_CHOICE(...) LOOKUP_CHOICE(__VA_ARGS__)
#endif
#endif

do {

	enum
	{
		#define SKIT_X_TRIE_TO_USE(ptr_expr)
		#define SKIT_X_CHOICE(keyword, rule) skit_x__ ## keyword,
		#define SKIT_X_SUFFIX(name, rule)
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
		skit__peg_lookup__kw_count
	};
	
	enum
	{
		skit__peg_lookup_default_suffix,
		#define SKIT_X_TRIE_TO_USE(ptr_expr)
		#define SKIT_X_CHOICE(keyword, rule)
		#define SKIT_X_SUFFIX(name, rule)    skit_sx__ ## name,
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
	};

	// Retrieve the trie we will be using.
	#define SKIT_X_TRIE_TO_USE(ptr_expr) skit_trie *skit__peg_lookup_trie = (ptr_expr);
	#define SKIT_X_CHOICE(keyword, rule)
	#define SKIT_X_SUFFIX(name, rule)
	SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE, SKIT_X_SUFFIX)
	#undef SKIT_X_TRIE_TO_USE
	#undef SKIT_X_CHOICE
	#undef SKIT_X_SUFFIX

	// Lazily initialize the trie used to choose potential strings.
	static int skit__peg_lookup_initialized = 0;
	static int *skit__peg_lookup_suffix_table = NULL;
	if ( !skit__peg_lookup_initialized ) {

		sASSERT(skit__peg_lookup_trie != NULL);
		if ( skit_trie_len(skit__peg_lookup_trie) > 0 )
		{
			skit_trie_dtor(skit__peg_lookup_trie);
			skit_trie_ctor(skit__peg_lookup_trie);
		}
		
		skit__peg_lookup_suffix_table = skit_malloc(sizeof(int)*(skit__peg_lookup__kw_count));
		int skit__peg_lookup_current_suffix = skit__peg_lookup_default_suffix;

		#define SKIT_X_TRIE_TO_USE(ptr_expr) (void)0;
		#define SKIT_X_CHOICE(keyword, rule) \
			skit_trie_set( skit__peg_lookup_trie, sSLICE(#keyword), (void*)(skit_x__ ## keyword), SKIT_FLAG_C ); \
			skit__peg_lookup_suffix_table[skit_x__ ## keyword] = skit__peg_lookup_current_suffix;
		#define SKIT_X_SUFFIX(name, rule) \
			skit__peg_lookup_current_suffix = skit_sx__ ## name;
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
		skit__peg_lookup_initialized = 1;
	}

	// Grab a keyword.  (This is essentially what a tokenizer would do, if we had one.)
	char *word_buf;
	skit_slice word;
	SKIT_PEG_RULE(any_word, "keyword", &word, &word_buf);
	if ( !match.successful )
		break;
	new_cursor = match.end;
	
	if ( auto_consume_whitespace )
		new_cursor = parser->parse_whitespace(parser, new_cursor, ubound);

	// Verify that the identifier/keyword parsed is one of
	//   the keywords that we can accept.
	// Additionally, find out WHICH key word it is.
	int skit__peg_lookup_which_keyword = -1;
	skit_slice skit__peg_lookup_result = 
		skit_trie_get(skit__peg_lookup_trie, word, (void**)&skit__peg_lookup_which_keyword,
			parser->case_sensitive ? SKIT_FLAGS_NONE : SKIT_FLAG_I);
	if ( skit_slice_is_null(skit__peg_lookup_result) )
	{
		skit_slice next_chars =
			skit_peg_next_chars_in_parse(parser,new_cursor,NUM_NEXT_CHARS);
		match = skit_peg_match_failure(parser, new_cursor, "Expected keyword, instead got '%.*s'",
			sSLENGTH(next_chars), sSPTR(next_chars) );
		break;
	}
	
	// Parse the SUFFIX associated with the keyword.
	int skit__peg_lookup_which_suffix = 
		skit__peg_lookup_suffix_table[skit__peg_lookup_which_keyword];
	
	// The default suffix would be RULE(success).
	// We can move a little faster by just doing nothing in such cases,
	//   which is what this if-statement enforces.
	if ( skit__peg_lookup_which_suffix != skit__peg_lookup_default_suffix )
	{
		// Dispatch using a switch-case statement.
		switch(skit__peg_lookup_which_suffix)
		{
			#define SKIT_X_TRIE_TO_USE(ptr_expr) default: sTHROW(SKIT_EXCEPTION, "This should not happen.");
			#define SKIT_X_CHOICE(keyword, rule)
			#define SKIT_X_SUFFIX(name, rule) \
				case skit_sx__ ## name: \
					rule; \
					break;

			SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE, SKIT_X_SUFFIX)
			#undef SKIT_X_TRIE_TO_USE
			#undef SKIT_X_CHOICE
			#undef SKIT_X_SUFFIX
		}
		if ( !match.successful )
			break;
		new_cursor = match.end;
		
		if ( auto_consume_whitespace )
			new_cursor = parser->parse_whitespace(parser, new_cursor, ubound);
	}

	// Dispatch using a switch-case statement.
	switch (skit__peg_lookup_which_keyword)
	{
		#define SKIT_X_TRIE_TO_USE(ptr_expr) default: sTHROW(SKIT_EXCEPTION, "This should not happen.");

		#define SKIT_X_CHOICE(keyword, rule) \
			case skit_x__ ## keyword: \
				rule; \
				break;
		
		#define SKIT_X_SUFFIX(name, rule)

		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
	}
	
	if ( !match.successful )
		break;
	new_cursor = match.end;

} while(0);

#ifdef SKIT__PEG_LOOKUP_CHOICE_STUB
#undef SKIT__PEG_LOOKUP_CHOICE_STUB
#undef SKIT_PEG_LOOKUP_CHOICE
#endif
