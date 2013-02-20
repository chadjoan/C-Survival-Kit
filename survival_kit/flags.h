
/**

Overview:

This module defines the sFLAGS and skit_flags_to_str functions.

The sFLAGS operator is motivated by the need for a concise way to pass 
bit-flags to functions in C without using offensively long symbols and without
polluting C's global namespace.  

Examples of undesirable bit-flags:
skit_trie_setc(trie, "abcde", value, SKIT_TRIE_CREATE | SKIT_TRIE_OVERWRITE |
	SKIT_TRIE_CASE_INSENTIVE);
skit_trie_setc(trie, "abcde", value, CREATE | OVERWRITE | CASE_INSENSITIVE);
skit_trie_setc(trie, "abcde", value, "co" | caller_passed_case);

In the first case, there are 39 characters dedicated to the function call and
it's non-flag arguments, and about 64 (not include newline/tab) dedicated to
handling the hypothetical flags.  The flags occupy more screen real-estate than
the rest of the function call.  This tends to cause dense expressions and 
line-wrapping that harm code readability.  It's not even too bad in isolation,
but with five or ten function calls like this next to each other, it begins to
scale very poorly.  

The second case is more readable.  It suffers from a different problem: it
introduces CREATE, OVERWRITE, and CASE_INSENSITIVE into the global namespace.
Given that these symbols are not prefixed with a library or module designation,
they are incredibly likely to collide with symbols from other libraries or
caller code.

The third case is concise and, while not quite as readable as the second, does
at least offer the reader a mnemonic to work with.  The problem is that it does
not compile.  Purely string-based flags do not compose easily in C.

The C Survival Kit solution to this problem is to provide a very concise way
to convert string-based flags into integral flags that can be easily composed:
skit_trie_setc(trie, "abcde", value, sFLAGS("co") | caller_passed_case);

To work with flags in integer form without unnecessary string conversion, there
is a set of macro definitions for which bits in an integer represent which
letter in a flags string:

SKIT_FLAG_A
SKIT_FLAG_B
SKIT_FLAG_C
...
SKIT_FLAG_X
SKIT_FLAG_Y
SKIT_FLAG_Z

There is also a macro definition for passing a value with no flags set:
SKIT_FLAGS_NONE

These can be composed with ordinary C bitwise operations:
skit_flags flags = sFLAGS("agz") | SKIT_FLAG_C;
skit_flags moreflags = sFLAGS("l") | flags;
if ( moreflags & SKIT_FLAG_A )
	moreflags |= SKIT_FLAG_Q;
sASSERT_EQ(moreflags & SKIT_FLAG_Q, SKIT_FLAG_Q, "%x");

*/

#ifndef SKIT_FLAGS_INCLUDED
#define SKIT_FLAGS_INCLUDED

#include <inttypes.h>

#include "survival_kit/feature_emulation/exception.h"

extern skit_err_code SKIT_BAD_FLAGS;

/* Do not call directly.  skit_init() should handle this. */
void skit_flags_module_init();

/** 
An integer representation for flags.

If a function needs to accept bitwise flags as one of its arguments, then
it should use this for the argument type.
*/
typedef uint32_t skit_flags;

/* Internal use for sFLAGS.  
It is, however, stable enough to link against in other-language bindings. */
skit_flags skit_str_to_flags( const char *flags_as_cstr );

/**
Converts the given C-string representation of flags into an integer
representation of type skit_flags.

Only lowercase alphabetic characters are accepted in this string, with the
only exception being the null-terminating '\0' character at the end of the
string.

If any other characters are passed, a SKIT_BAD_FLAGS exception will be thrown.

Duplicate flags are not allowed and will result in a SKIT_BAD_FLAGS exception
being thrown.  This would probably happen if code was used to generate a flag
string by concatenating strings.  Composition of flags should instead be done
by converting to integer representations, doing bitwise operations to compose
those, and then converting back to string flags (if needed) by using 
skit_flags_to_str.

Returns: the integer representation of the flags given by 'flags_as_cstr'

Example:
sASSERT_EQ(sFLAGS("agz"), SKIT_FLAG_A | SKIT_FLAG_G | SKIT_FLAG_Z, "%x");
sASSERT_EQ(sFLAGS("zga"), SKIT_FLAG_A | SKIT_FLAG_G | SKIT_FLAG_Z, "%x");
sASSERT_EQ(sFLAGS(""), SKIT_FLAGS_NONE, "%x");

*/
#define sFLAGS(flags_as_cstr) (skit_str_to_flags((flags_as_cstr)))

/**
Converts the given flags into their string representation.  The result is a
traditional C-style null-terminated string that is placed into 'buffer'.

The characters in the result string will be sorted in alphabetical order, with
the lowest character (if any exist) coming first.

buffer must be passed a string that is at least as large as
SKIT_FLAGS_BUF_SIZE.

Returns: the number of characters written into 'buffer', not including the
null-terminating zero character at the end.

Example:
char buf[SKIT_FLAGS_BUF_SIZE];
skit_flags_to_str( SKIT_FLAG_A | SKIT_FLAG_G | SKIT_FLAG_Z, buf );
sASSERT_EQ( strcmp( buf, "agz" ), 0, "%d" );
skit_flags_to_str( SKIT_FLAGS_NONE, buf );
sASSERT_EQ( strcmp( buf, "" ), 0, "%d" );
*/
int skit_flags_to_str( skit_flags flags, char *buffer );

/**
Used with skit_flags_to_str.
*/
#define SKIT_FLAGS_BUF_SIZE 33

/** */
#define SKIT_FLAGS_NONE (0)
#define SKIT_FLAG_A (0x00000001UL)
#define SKIT_FLAG_B (0x00000002UL)
#define SKIT_FLAG_C (0x00000004UL)
#define SKIT_FLAG_D (0x00000008UL)
#define SKIT_FLAG_E (0x00000010UL)
#define SKIT_FLAG_F (0x00000020UL)
#define SKIT_FLAG_G (0x00000040UL)
#define SKIT_FLAG_H (0x00000080UL)
#define SKIT_FLAG_I (0x00000100UL)
#define SKIT_FLAG_J (0x00000200UL)
#define SKIT_FLAG_K (0x00000400UL)
#define SKIT_FLAG_L (0x00000800UL)
#define SKIT_FLAG_M (0x00001000UL)
#define SKIT_FLAG_N (0x00002000UL)
#define SKIT_FLAG_O (0x00004000UL)
#define SKIT_FLAG_P (0x00008000UL)
#define SKIT_FLAG_Q (0x00010000UL)
#define SKIT_FLAG_R (0x00020000UL)
#define SKIT_FLAG_S (0x00040000UL)
#define SKIT_FLAG_T (0x00080000UL)
#define SKIT_FLAG_U (0x00100000UL)
#define SKIT_FLAG_V (0x00200000UL)
#define SKIT_FLAG_W (0x00400000UL)
#define SKIT_FLAG_X (0x00800000UL)
#define SKIT_FLAG_Y (0x01000000UL)
#define SKIT_FLAG_Z (0x02000000UL)

void skit_flags_unittest();

#endif