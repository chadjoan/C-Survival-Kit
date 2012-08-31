#include <inttypes.h>
#include <assert.h>

#include "survival_kit/generated_exception_defs.h"

err_code_t __exc_inheritance_table[__EXC_TABLE_SIZE] = {
	0, /* Never use 0, it has a special meaning for setjmp/longjmp. */
	1, /* 1 is pretty dangerous too. */
	GENERIC_EXCEPTION, /* Is it's own parent. */
	FATAL_EXCEPTION,   /* ditto */
	/* From now on, it goes like so: */
	/* PARENT_NAME, */ /* CHILD_NAME */
	GENERIC_EXCEPTION, /* BREAK_IN_TRY_CATCH */
	GENERIC_EXCEPTION, /* CONTINUE_IN_TRY_CATCH */
	GENERIC_EXCEPTION, /* SOCKET_EXCEPTION */
	};

int exception_is_a( err_code_t ecode1, err_code_t ecode2 )
{
	err_code_t child_code = ecode1;
	err_code_t parent_code = ecode2;
	err_code_t last_parent = 0;
	while (1)
	{
		/* Invalid parent_codes. */
		if ( parent_code <= 1 || parent_code >= __EXC_TABLE_SIZE )
			return 0;
		
		/* Match found! */
		if ( child_code == parent_code )
			return 1;

		/* We've hit a root-level exception type without establishing a parent-child relationship. No good. */
		if ( last_parent == parent_code )
			return 0;
		
		last_parent = parent_code;
		parent_code = __exc_inheritance_table[parent_code];
	}
	assert(0);
}