#ifndef ERR_UTIL_INCLUDED
#define ERR_UTIL_INCLUDED

#include <stdlib.h>

#define ERROR_BUFFER_SIZE 1024
extern char error_text_buffer[ERROR_BUFFER_SIZE];

typedef struct exception exception;
struct exception
{
	/* Implementation note: keep this small, it will be returned by-value from functions a lot. */
	size_t error_code;   /* 0 should always mean "no error" */
	char *error_text;    /* READ ONLY: a description for the error. */
};

void print_exception(exception e);
exception no_exception();
exception new_exception(size_t error_code, char *mess, ...);

void die(char *mess, ...);

#endif