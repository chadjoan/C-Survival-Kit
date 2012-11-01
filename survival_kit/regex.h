
/** Everything in this module is very NOT implemented. */

#ifndef SKIT_REGEX_INCLUDED
#define SKIT_REGEX_INCLUDED

#include <stdlib.h>
#include "survival_kit/string.h"

typedef struct skit_regex_match skit_regex_match;
struct skit_regex_match
{
	// These members are VERY likely to change.
	size_t lo;
	size_t hi;
};

typedef struct skit_regex_engine skit_regex_engine;
struct skit_regex_engine
{
	// These members are VERY likely to change.
	skit_regex_match match;
	int state;
};

void skit_regex_init(skit_regex_engine *regex);
skit_regex_engine *skit_regex_new();
void skit_regex_compile(skit_regex_engine *regex, skit_slice expr);

/** Returns 1 if it's still hungry.  0 if it's done (for better or worse). */
int skit_regex_feed(skit_regex_engine *regex, skit_utf8c c);

void skit_regex_dtor(skit_regex_engine *regex);
skit_regex_engine *skit_regex_free(skit_regex_engine *regex);

skit_regex_match *skit_regex_get_matches(skit_regex_engine *regex);
skit_slice skit_regex_match_n(skit_slice original_text, skit_regex_match *match, size_t which);

void skit_regex_unittest();

#endif
