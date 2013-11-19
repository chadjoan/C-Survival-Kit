
#if defined(__DECC)
#pragma module skit_flags
#endif

#include <inttypes.h>
#include <string.h>

#include "survival_kit/assert.h"
#include "survival_kit/feature_emulation.h"
#include "survival_kit/flags.h"

skit_err_code SKIT_BAD_FLAGS;

void skit_flags_module_init()
{
	//                      Exception,        Derived from,     Default message
	SKIT_REGISTER_EXCEPTION(SKIT_BAD_FLAGS,   SKIT_EXCEPTION,   "Invalid character found in sFLAGS(...) expression.");
}

skit_flags skit_str_to_flags( const char *flags_as_cstr )
{
	SKIT_USE_FEATURE_EMULATION;
	skit_flags result = 0;
	
	while (1)
	{
		char c = *flags_as_cstr;
		if ( c == '\0' )
			break;
		
		if ( 'a' <= c && c <= 'z' )
		{
			skit_flags new_flag = 1UL << (skit_flags)(c - 'a');
			if ( result & new_flag )
				sTHROW(SKIT_BAD_FLAGS, "Duplicate character found in sFLAGS(\"%s\"): %c", flags_as_cstr, c);
			result |= new_flag;
		}
		else
			sTHROW(SKIT_BAD_FLAGS, "Invalid character found in sFLAGS(\"%s\"): %c", flags_as_cstr, c);
		
		flags_as_cstr++;
	}
	
	return result;
}

static void skit_str_to_flags_unittest()
{
	SKIT_USE_FEATURE_EMULATION;
	
	/* sFLAGS example. */
	sASSERT_EQ_HEX(sFLAGS("agz"), SKIT_FLAG_A | SKIT_FLAG_G | SKIT_FLAG_Z);
	sASSERT_EQ_HEX(sFLAGS("zga"), SKIT_FLAG_A | SKIT_FLAG_G | SKIT_FLAG_Z);
	sASSERT_EQ_HEX(sFLAGS(""), SKIT_FLAGS_NONE);
	
	/* Overview example. */
	skit_flags flags = sFLAGS("agz") | SKIT_FLAG_C;
	skit_flags moreflags = sFLAGS("l") | flags;
	if ( moreflags & SKIT_FLAG_A )
		moreflags |= SKIT_FLAG_Q;
	sASSERT_EQ_HEX(moreflags & SKIT_FLAG_Q, SKIT_FLAG_Q);
	
	/* Exception cases. */
	int exception_triggered;
	
	exception_triggered = 0;
	sTRY
		skit_flags exc_flags = sFLAGS("Q");
		(void)exc_flags;
	sCATCH(SKIT_BAD_FLAGS, e)
		exception_triggered = 1;
	sEND_TRY
	sASSERT(exception_triggered);
	
	exception_triggered = 0;
	sTRY
		skit_flags exc_flags = sFLAGS(" ");
		(void)exc_flags;
	sCATCH(SKIT_BAD_FLAGS, e)
		exception_triggered = 1;
	sEND_TRY
	sASSERT(exception_triggered);
	
	exception_triggered = 0;
	sTRY
		skit_flags exc_flags = sFLAGS("accz");
		(void)exc_flags;
	sCATCH(SKIT_BAD_FLAGS, e)
		exception_triggered = 1;
	sEND_TRY
	sASSERT(exception_triggered);
	
	printf("  skit_str_to_flags_unittest passed.\n");
}

int skit_flags_to_str( skit_flags flags, char *buffer )
{
	size_t i = 0;
	int j = 0;
	
	for (; i < 26; i++)
	{
		if ( flags & (1UL << i) )
			buffer[j++] = 'a' + i;
	}
	buffer[j] = '\0';
	return j;
}

static void skit_flags_to_str_unittest()
{
	char buf[SKIT_FLAGS_BUF_SIZE];
	skit_flags_to_str( SKIT_FLAG_A | SKIT_FLAG_G | SKIT_FLAG_Z, buf );
	sASSERT_EQ( strcmp( buf, "agz" ), 0 );
	skit_flags_to_str( SKIT_FLAGS_NONE, buf );
	sASSERT_EQ( strcmp( buf, "" ), 0 );
	
	printf("  skit_flags_to_str_unittest passed.\n");
}

void skit_flags_unittest()
{
	printf("skit_flags_unittest()\n");
	skit_str_to_flags_unittest();
	skit_flags_to_str_unittest();
	printf("  skit_flags_unittest passed!\n");
	printf("\n");
}
