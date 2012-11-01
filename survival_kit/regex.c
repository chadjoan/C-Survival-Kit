
#include "survival_kit/misc.h"
#include "survival_kit/feature_emulation.h"
#include "survival_kit/regex.h"
#include "survival_kit/string.h"

void skit_regex_init(skit_regex_engine *regex)
{
	regex->match.lo = 0;
	regex->match.hi = 0;
	regex->state = 0;
}

skit_regex_engine *skit_regex_new()
{
	skit_regex_engine *regex = skit_malloc(sizeof(skit_regex_engine));
	skit_regex_init(regex);
	return regex;
}

void skit_regex_compile(skit_regex_engine *regex, skit_slice expr)
{
	SKIT_USE_FEATURE_EMULATION;
	if ( skit_slice_nes(expr, sSLICE(".*\0")) )
		sTHROW(SKIT_EXCEPTION, "skit_regex_compile only accepts the expression \".*\\0\" right now.");
}

/** Returns 1 if it's still hungry.  0 if it's done (for better or worse). */
int skit_regex_feed(skit_regex_engine *regex, skit_utf8c c)
{
	int state = regex->state;
	if ( state == 0 )
	{
		if ( c != '\0' )
		{
			regex->match.hi += 1;
			return 1;
		}
		else
		{
			regex->match.hi += 1;
			regex->state = 1;
			return 0;
		}
	}
	else
		return 0;
}

skit_regex_match *skit_regex_get_matches(skit_regex_engine *regex)
{
	return &regex->match;
}

skit_slice skit_regex_match_n(skit_slice original_text, skit_regex_match *match, size_t which)
{
	return skit_slice_of(original_text, match->lo, match->hi);
}

void skit_regex_dtor(skit_regex_engine *regex)
{
	/* Doesn't do anything right now. */
}

skit_regex_engine *skit_regex_free(skit_regex_engine *regex)
{
	skit_regex_dtor(regex);
	if ( regex != NULL )
		skit_free(regex);
	return NULL;
}

void skit_regex_unittest()
{
	printf("skit_regex_unittest()\n");
	
	skit_slice original_text = sSLICE("aa\0a");
	skit_regex_engine e;
	skit_regex_init(&e);
	skit_regex_compile(&e, sSLICE(".*\0"));
	sASSERT_EQ(skit_regex_feed(&e, 'a'), 1, "%d");
	sASSERT_EQ(skit_regex_feed(&e, 'a'), 1, "%d");
	sASSERT_EQ(skit_regex_feed(&e, '\0'), 0, "%d");
	sASSERT_EQ(skit_regex_feed(&e, 'a'), 0, "%d");
	skit_regex_match *match = skit_regex_get_matches(&e);
	sASSERT_EQS(skit_regex_match_n(original_text, match, 0), sSLICE("aa\0"));
	skit_regex_dtor(&e);
	
	printf("  skit_regex_unittest passed!\n");
	printf("\n");
}
