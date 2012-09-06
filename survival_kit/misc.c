
#include <unistd.h> /* For STDERR_FILENO */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(__DECC) && defined(__VMS)
#  include <lib$routines.h> /* lib$signal */
#else
#  include <execinfo.h> /* backtrace_symbols_fd */
#  define HAVE_LINUX_BACKTRACE
#endif

#include "survival_kit/misc.h"

char skit_error_text_buffer[SKIT_ERROR_BUFFER_SIZE];

void skit_die(char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	vsnprintf(skit_error_text_buffer, SKIT_ERROR_BUFFER_SIZE, mess, vl);
	va_end(vl);
	
	fprintf(stderr,"\n");
	fprintf(stderr,"ERROR: skit_die was called.\n");
	fprintf(stderr,"Message:\n");
	
	fprintf(stderr,"%s\n",skit_error_text_buffer);
	fprintf(stderr,"\n");
	
	fprintf(stderr,"perror value:\n");
	perror("");
	fprintf(stderr,"\n");
	
#ifdef HAVE_LINUX_BACKTRACE
	fprintf(stderr,"backtrace:\n");
	void *backtrace_buf[256];
	int n_addresses = backtrace(backtrace_buf,256);
	backtrace_symbols_fd(backtrace_buf, n_addresses, STDERR_FILENO);
	fprintf(stderr,"\n");
#endif
	
	/* TODO: this should probably not crash the program if this is not the main
	 * thread.  If the thread is non-main, then it should just kill the thread. */
	exit(1);
	/*lib$signal(EXIT_FAILURE);*/ /* This produced too much spam when exiting after a bunch of setjmp/longjmps. */
}

void *skit_malloc(size_t size)
{
	return malloc(size);
}

void skit_print_mem(void *ptr, int size)
{
	#define line_width 8
	if ( (size & (line_width-1)) != 0 ) /* HACK */
		skit_die("print_mem(%p,%d): size argument must be a multiple of %d\n",ptr,size,line_width);

	unsigned char xline[line_width+1];
	unsigned char line[line_width+1];
	xline[line_width] = '\0';
	line[line_width] = '\0';
	int pos = 0;
		printf("ADDR: 00 01 02 03 . 04 05 06 07 : text\n");
	while ( pos < size )
	{
		int i = 0;
		for ( i = 0; i < line_width; i++ )
		{
			unsigned char c = ((char*)ptr)[pos+i];
			
			xline[i] = c;
			
			/* printable ASCII chars */
			if ( 32 <= c && c <= 126 )
				line[i] = c;
			else
				line[i] = '.';
		}
		
		printf("%04x: %02x %02x %02x %02x . %02x %02x %02x %02x : %s\n",
			pos,
			xline[0], xline[1], xline[2], xline[3],
			xline[4], xline[5], xline[6], xline[7],
			line);
		pos += line_width;
	}
	printf("\n");
}
