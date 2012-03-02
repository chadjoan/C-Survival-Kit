
#include "ERR_UTIL.H"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <lib$routines.h> /* lib$signal */

char error_text_buffer[ERROR_BUFFER_SIZE];

void print_exception(exception e)
{
	printf("%s\n",e.error_text);
}

exception no_exception()
{
	exception result;
	result.error_code = 0;
	result.error_text = NULL;
	return result;
}

exception new_exception(size_t error_code, char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	vsnprintf(error_text_buffer, ERROR_BUFFER_SIZE, mess, vl);
	va_end(vl);
	
	exception result;
	result.error_code = error_code;
	result.error_text = error_text_buffer;
	return result;
}

void die(char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	vsnprintf(error_text_buffer, ERROR_BUFFER_SIZE, mess, vl);
	va_end(vl);
	
	perror(error_text_buffer);
	/*exit(1);*/
	lib$signal(EXIT_FAILURE);
}