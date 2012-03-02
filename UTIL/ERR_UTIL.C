
#include "ERR_UTIL.H"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <lib$routines.h> /* lib$signal */

void die(char *mess, ...)
{
	char buf[1024];
	va_list vl;
	va_start(vl, mess);
	vsnprintf(buf, 1024, mess, vl);
	va_end(vl);
	
	perror(buf);
	/*exit(1);*/
	lib$signal(EXIT_FAILURE);
}