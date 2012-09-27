
#ifdef __DECC
#pragma module skit_feature_emu_gen_exc_defs
#endif

#include <inttypes.h>
#include <assert.h>
#include <stdio.h> /* printf */

#include "survival_kit/feature_emulation/generated_exception_defs.h"

#define SKIT_DEFINE_EXC_TABLE_ENTRY(EXC_NAME, PARENT_NAME, UID, ERR_TXT) \
	PARENT_NAME,
static skit_err_code __skit_exc_inheritance_table[__SKIT_EXC_TABLE_SIZE] = {
	0,
	1,
SKIT_EXCEPTION_LIST(SKIT_DEFINE_EXC_TABLE_ENTRY)
	};
#undef SKIT_DEFINE_EXC_TABLE_ENTRY

int skit_exception_is_a( skit_err_code ecode1, skit_err_code ecode2 )
{
	skit_err_code child_code = ecode2;
	skit_err_code parent_code = ecode1;
	skit_err_code last_parent = 0;
	while (1)
	{
		/* Invalid parent_codes. */
		if ( parent_code <= 1 || parent_code >= __SKIT_EXC_TABLE_SIZE )
			return 0;
		
		/* Match found! */
		if ( child_code == parent_code )
			return 1;

		/* We've hit a root-level exception type without establishing a parent-child relationship. No good. */
		if ( last_parent == parent_code )
			return 0;
		
		last_parent = parent_code;
		parent_code = __skit_exc_inheritance_table[parent_code];
	}
	assert(0);
}