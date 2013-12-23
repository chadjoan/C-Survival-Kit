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
		#define SKIT_PEG_LOOKUP_CHOICE(TRIE_TO_USE, CHOICE) \
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
		#define LOOKUP_CHOICE(TRIE_TO_USE, CHOICE) \
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
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
	};

	// Retrieve the trie we will be using.
	#define SKIT_X_TRIE_TO_USE(ptr_expr) skit_trie *skit__peg_lookup_trie = (ptr_expr);
	#define SKIT_X_CHOICE(keyword, rule)
	SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE)
	#undef SKIT_X_TRIE_TO_USE
	#undef SKIT_X_CHOICE

	// Lazily initialize the trie used to choose potential strings.
	static int skit__peg_lookup_initialized = 0;
	if ( !skit__peg_lookup_initialized ) {

		sASSERT(skit__peg_lookup_trie != NULL);
		if ( skit_trie_len(skit__peg_lookup_trie) > 0 )
		{
			skit_trie_dtor(skit__peg_lookup_trie);
			skit_trie_ctor(skit__peg_lookup_trie);
		}

		#define SKIT_X_TRIE_TO_USE(ptr_expr) (void)0;
		#define SKIT_X_CHOICE(keyword, rule) \
			skit_trie_set( skit__peg_lookup_trie, sSLICE(#keyword), (void*)(skit_x__ ## keyword), SKIT_FLAG_C );
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
		skit__peg_lookup_initialized = 1;
	}

	// Grab a keyword.  (This is essentially what a tokenizer would do, if we had one.)
	char *word_buf;
	skit_slice word;
	SKIT_PEG_RULE(any_word, "keyword", &word, &word_buf);
	if ( !match.successful )
		break;

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

	// Dispatch using a switch-case statement.
	switch (skit__peg_lookup_which_keyword)
	{
		#define SKIT_X_TRIE_TO_USE(ptr_expr) default: sTHROW(SKIT_EXCEPTION, "This should not happen.");
		#define SKIT_X_CHOICE(keyword, rule) case skit_x__ ## keyword: rule; break;
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_TRIE_TO_USE, SKIT_X_CHOICE)
		#undef SKIT_X_TRIE_TO_USE
		#undef SKIT_X_CHOICE
	}

} while(0);

#ifdef SKIT__PEG_LOOKUP_CHOICE_STUB
#undef SKIT__PEG_LOOKUP_CHOICE_STUB
#undef SKIT_PEG_LOOKUP_CHOICE
#endif
