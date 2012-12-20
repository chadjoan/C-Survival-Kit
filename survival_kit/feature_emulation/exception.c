
#ifdef __DECC
#pragma module skit_feature_emu_exception
#endif

#include <inttypes.h>
#include <assert.h>
#include <stdio.h> /* printf */

#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation/exception.h"

#define SKIT_T_DIE_ON_ERROR 1
#define SKIT_T_ELEM_TYPE skit_exception
#define SKIT_T_NAME exc
#include "survival_kit/templates/stack.c"
#include "survival_kit/templates/fstack.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_DIE_ON_ERROR

skit_err_code SKIT_EXCEPTION       = SKIT_EXCEPTION_INITIAL_VAL;
skit_err_code SKIT_FATAL_EXCEPTION = SKIT_EXCEPTION_INITIAL_VAL;
skit_err_code SKIT_BREAK_IN_TRY_CATCH;
skit_err_code SKIT_CONTINUE_IN_TRY_CATCH;
skit_err_code SKIT_SOCKET_EXCEPTION;
skit_err_code SKIT_OUT_OF_BOUNDS;
skit_err_code SKIT_NOT_IMPLEMENTED;
skit_err_code SKIT_IO_EXCEPTION;
skit_err_code SKIT_FILE_IO_EXCEPTION;
skit_err_code SKIT_TCP_IO_EXCEPTION;
skit_err_code SKIT_FILE_NOT_FOUND;

static int skit_exceptions_initialized = 0;

void skit_init_exceptions()
{
	if ( skit_exceptions_initialized )
		return;
	
	/* Make sure the first exception ever is the unrecoverable one. */
	/* That way, if 0 or NULL is thrown, it will cause a fatal exception and halt the program. */	
	/* This can happen if someone forgets to register an exception but creates a variable for it */
	/*   and then throws that variable.  This could wreak havoc, so crashing immediately is a */
	/*   relatively forgiving outcome. */
	SKIT_REGISTER_EXCEPTION(SKIT_FATAL_EXCEPTION,       SKIT_FATAL_EXCEPTION,   "An unrecoverable exception was thrown.");
	SKIT_REGISTER_EXCEPTION(SKIT_EXCEPTION,             SKIT_EXCEPTION,         "An exception was thrown.");
	SKIT_REGISTER_EXCEPTION(SKIT_BREAK_IN_TRY_CATCH,    SKIT_EXCEPTION,         "Attempt to use the built-in 'break' statement while in an sTRY-sCATCH block.");
	SKIT_REGISTER_EXCEPTION(SKIT_CONTINUE_IN_TRY_CATCH, SKIT_EXCEPTION,         "Attempt to use the built-in 'continue' statement while in an sTRY-sCATCH block.");
	SKIT_REGISTER_EXCEPTION(SKIT_SOCKET_EXCEPTION,      SKIT_EXCEPTION,         "socket exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_OUT_OF_BOUNDS,         SKIT_EXCEPTION,         "Out of bounds exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_NOT_IMPLEMENTED,       SKIT_EXCEPTION,         "The requested feature is not implemented.");
	SKIT_REGISTER_EXCEPTION(SKIT_IO_EXCEPTION,          SKIT_EXCEPTION,         "Generic I/O exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_FILE_IO_EXCEPTION,     SKIT_IO_EXCEPTION,      "File I/O exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_TCP_IO_EXCEPTION,      SKIT_IO_EXCEPTION,      "TCP Network I/O exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_FILE_NOT_FOUND,        SKIT_FILE_IO_EXCEPTION, "File not found.");

	skit_exceptions_initialized = 1;
}

static skit_err_code *_skit_exc_inheritance_table = NULL;
static size_t _skit_exc_table_size = 0;

void _skit__register_exception( skit_err_code *ecode, const skit_err_code *parent, const char *ecode_name, const char *default_msg )
{
	/* 
	Note that parent is passed by reference and not value.
	This is to ensure that cases like 
	  SKIT_REGISTER_EXCEPTION(SKIT_EXCEPTION, SKIT_EXCEPTION, "...")
	will actually do what is expected: make the exception inherit from itself.
	If parent were passed by value, then the above expression might fill the
	'parent' argument with whatever random garbage the compiler left in the
	initial value of SKIT_EXCEPTION.  *ecode = ...; will assign the original
	variable a sane value, but the parent value in the table will still end
	up being the undefined garbage value.  By passing parent as a pointer we
	ensure that *ecode = ...; will also populate the parent value in cases
	where an exception is defined as its own parent (root-level exceptions). */
	
	assert(ecode != NULL);
	assert(parent != NULL);
	assert(ecode_name != NULL);
	assert(default_msg != NULL);
	
	*ecode = _skit_exc_table_size;
	
	if ( _skit_exc_inheritance_table == NULL )
	{
		/* If this doesn't already exist, create the table and give it 1 element. */
		_skit_exc_inheritance_table = skit_malloc(sizeof(skit_err_code));
		_skit_exc_table_size = 1;
	}
	else
	{
		/* If this does exist, it will need to grow by one element. */
		_skit_exc_table_size += 1;
		_skit_exc_inheritance_table = skit_realloc(
			_skit_exc_inheritance_table,
			sizeof(skit_err_code) * _skit_exc_table_size);
	}
	
	_skit_exc_inheritance_table[*ecode] = *parent;
	/* printf("Defined %s as %ld\n", ecode_name, *ecode); */
	/* TODO: do something with the ecode_name and default_msg. */
}

int skit_exception_is_a( skit_err_code ecode1, skit_err_code ecode2 )
{
	skit_err_code child_code = ecode2;
	skit_err_code parent_code = ecode1;
	skit_err_code last_parent = 0;
	while (1)
	{
		/* Invalid parent_codes. */
		if ( parent_code >= _skit_exc_table_size )
			assert(0);
		
		/* Match found! */
		if ( child_code == parent_code )
			return 1;

		/* We've hit a root-level exception type without establishing a parent-child relationship. No good. */
		if ( last_parent == parent_code )
			return 0;
		
		last_parent = parent_code;
		parent_code = _skit_exc_inheritance_table[parent_code];
	}
	assert(0);
}