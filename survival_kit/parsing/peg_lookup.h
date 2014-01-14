/**
This header file defines an X-macro that implements a fast N-way unordered
choice between N keywords.  This is potentially much more efficient than
doing N string comparisons worth of ordered choice when many possible
keywords might appear at a given place in a grammar.

If is is necessary to give different threads different instances of the
lookup's state, or if it is necessary for the caller to free the memory used
by the lookup at some point, then it will be necessary to use the INDEX
parameter.  The INDEX parameter is a macro that accepts an initialized
skit_peg_lookup_index pointer that is owned by the caller.  Use
skit_peg_lookup_index_new() or skit_peg_lookup_index_ctor(index) to create
such an object and pass it into the lookup.  The index will be populated on
the first entry into the peg_lookup.h inclusion.  The inclusion macro uses
a pthread mutex to ensure that any attempts to access a lookup index in the
process of being populated will block until the population is complete.

If INDEX is passed a NULL argument or never specified, then the lookup
inclusion will initialize it's own (static / process-global) instance that
will only be created once and will live for the remainder of the process's
execution.  This process-global instance will be initialized on the first
entry into the peg_lookup.h inclusion.

(in other words: call
skit_peg_lookup_index_ctor on it first).

Usage is like so:

	SKIT_PEG_DEFINE_RULE(my_lookup_rule, skit_peg_lookup_index *index, int *result)
		SKIT_USE_FEATURE_EMULATION; // required.
	
		// LOOKUPs may not be nested within other PEG elements, so they must
		// be at the root of their own RULE.
		#define SKIT_PEG_LOOKUP_CHOICE(INDEX, CHOICE, SUFFIX) \
			INDEX(index) \
			CHOICE(x,       SKIT_PEG_ACTION( (*result) = 1; )) \
			CHOICE(foobar,  SKIT_PEG_ACTION( (*result) = 2; )) \
			CHOICE(foo,     SKIT_PEG_ACTION( (*result) = 3; )) \
			CHOICE(FOO,     SKIT_PEG_ACTION( (*result) = 4; ))
		#include "survival_kit/parsing/peg_lookup.h"
		#undef SKIT_PEG_LOOKUP_CHOICE
	SKIT_PEG_END_RULE
	
	...
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("FOO"));
	skit_peg_lookup_index *index = skit_peg_lookup_index_new();
	...
	
	int result = 0;
	SKIT_PEG_RULE(my_lookup_rule, index, &result);
	sASSERT_EQ(result, 4);


Or, with shortand notation (#include "survival_kit/parsing/peg_shorthand.h")

	DEFINE_RULE(my_lookup_rule, int *result)
		SKIT_USE_FEATURE_EMULATION; // required.

		// LOOKUPs may not be nested within other PEG elements, so they must
		// be at the root of their own RULE.
		
		// In this case, in addition to the usual shorthand notation, we also
		// let the lookup allocate it's own static index that is shared between
		// all threads.
		#define LOOKUP_CHOICE(INDEX, CHOICE, SUFFIX) \
			CHOICE(x,       ACTION( (*result) = 1; )) \
			CHOICE(foobar,  ACTION( (*result) = 2; )) \
			CHOICE(foo,     ACTION( (*result) = 3; )) \
			CHOICE(FOO,     ACTION( (*result) = 4; ))
		#include "survival_kit/parsing/peg_lookup.h"
		#undef LOOKUP_CHOICE
	END_RULE
	
	...
	skit_peg_parser *parser = skit_peg_parser_mock_new(sSLICE("FOO"));
	...
	
	int result = 0;
	SKIT_PEG_RULE(my_lookup_rule, &result);
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
		#define SKIT_X_INDEX(ptr_expr)
		#define SKIT_X_CHOICE(keyword, rule) skit_x__ ## keyword,
		#define SKIT_X_SUFFIX(name, rule)
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_INDEX, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_INDEX
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
		skit__peg_lookup__kw_count
	};
	
	enum
	{
		skit__peg_lookup_default_suffix,
		#define SKIT_X_INDEX(ptr_expr)
		#define SKIT_X_CHOICE(keyword, rule)
		#define SKIT_X_SUFFIX(name, rule)    skit_sx__ ## name,
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_INDEX, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_INDEX
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
	};
	
	skit_peg_lookup_index *skit__peg_lookup_index = NULL;

	// Retrieve the index we will be using (if available).
	#define SKIT_X_INDEX(ptr_expr) skit__peg_lookup_index = (ptr_expr);
	#define SKIT_X_CHOICE(keyword, rule)
	#define SKIT_X_SUFFIX(name, rule)
	SKIT_PEG_LOOKUP_CHOICE(SKIT_X_INDEX, SKIT_X_CHOICE, SKIT_X_SUFFIX)
	#undef SKIT_X_INDEX
	#undef SKIT_X_CHOICE
	#undef SKIT_X_SUFFIX

	// Fill in a static/process-global index if the caller doesn't want to
	// control this aspect of the lookup.
	static skit_peg_lookup_index *skit_peg__global_index = NULL;
	
	if ( skit__peg_lookup_index == NULL )
	{
		sCTRACE(pthread_mutex_lock(&skit_peg__lookup_mutex));

		if ( skit_peg__global_index == NULL )
			skit_peg__global_index = skit_peg_lookup_index_new();
		
		skit__peg_lookup_index = skit_peg__global_index;
		
		sCTRACE(pthread_mutex_unlock(&skit_peg__lookup_mutex));
	}
	
	// Note: Avoid the perils of Double-Checked Locking.
	// Also, uncontested locks should be very fast to obtain.
	// (TODO: This should be true for modern OSes, but are they still fast to obtain on VMS?)
	
	sCTRACE(pthread_mutex_lock(&skit__peg_lookup_index->mutex));

	// Lazily initialize the index used to choose potential strings.	
	// if ( !initialized )
	if ( skit__peg_lookup_index->suffix_table == NULL )
	{
		// Note: The ->trie member should already be constructed.
		//       See skit_peg_lookup_index_ctor(index) to see where that occurs.
		skit__peg_lookup_index->suffix_table = skit_malloc(sizeof(int)*(skit__peg_lookup__kw_count));
		int skit__peg_lookup_current_suffix = skit__peg_lookup_default_suffix;

		#define SKIT_X_INDEX(ptr_expr) (void)0;
		#define SKIT_X_CHOICE(keyword, rule) \
			skit_trie_set( &skit__peg_lookup_index->trie, sSLICE(#keyword), (void*)(skit_x__ ## keyword), SKIT_FLAG_C ); \
			skit__peg_lookup_index->suffix_table[skit_x__ ## keyword] = skit__peg_lookup_current_suffix;
		#define SKIT_X_SUFFIX(name, rule) \
			skit__peg_lookup_current_suffix = skit_sx__ ## name;
		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_INDEX, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_INDEX
		#undef SKIT_X_CHOICE
		#undef SKIT_X_SUFFIX
	}
	sCTRACE(pthread_mutex_unlock(&skit__peg_lookup_index->mutex));

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
		skit_trie_get(&skit__peg_lookup_index->trie, word, (void**)&skit__peg_lookup_which_keyword,
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
		skit__peg_lookup_index->suffix_table[skit__peg_lookup_which_keyword];
	
	// The default suffix would be RULE(success).
	// We can move a little faster by just doing nothing in such cases,
	//   which is what this if-statement enforces.
	if ( skit__peg_lookup_which_suffix != skit__peg_lookup_default_suffix )
	{
		// Dispatch using a switch-case statement.
		switch(skit__peg_lookup_which_suffix)
		{
			#define SKIT_X_INDEX(ptr_expr) default: sTHROW(SKIT_EXCEPTION, "This should not happen.");
			#define SKIT_X_CHOICE(keyword, rule)
			#define SKIT_X_SUFFIX(name, rule) \
				case skit_sx__ ## name: \
					rule; \
					break;

			SKIT_PEG_LOOKUP_CHOICE(SKIT_X_INDEX, SKIT_X_CHOICE, SKIT_X_SUFFIX)
			#undef SKIT_X_INDEX
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
		#define SKIT_X_INDEX(ptr_expr) default: sTHROW(SKIT_EXCEPTION, "This should not happen.");

		#define SKIT_X_CHOICE(keyword, rule) \
			case skit_x__ ## keyword: \
				rule; \
				break;
		
		#define SKIT_X_SUFFIX(name, rule)

		SKIT_PEG_LOOKUP_CHOICE(SKIT_X_INDEX, SKIT_X_CHOICE, SKIT_X_SUFFIX)
		#undef SKIT_X_INDEX
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
