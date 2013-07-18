
#ifdef __DECC
#pragma module skit_misc
#endif

#include <unistd.h> /* For STDERR_FILENO */
#include <stdio.h>
#include <string.h> /* strerror_r */
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#if defined(__DECC) && defined(__VMS)
#  include <lib$routines.h> /* lib$signal */
#else
#  include <execinfo.h> /* backtrace_symbols_fd */
#  define HAVE_LINUX_BACKTRACE
#endif

/* Don't include this: it could create circular dependencies. */
/* In principle, anything that relies on feature emulation should be fairly domain-specific. */
/* If this principle is violated, then there will need to be some way to break apart the */
/*   contents of this "misc" module. (Or make the feature_emulation usage optional by macro #ifdef'ing.) */
/* #include "survival_kit/feature_emulation.h" */

#include "survival_kit/misc.h"

void skit_death_cry_va(char *mess, va_list vl)
{
	fprintf(stderr,"\n");
	fprintf(stderr,"ERROR: skit_die was called.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Message:\n");
	vfprintf(stderr,mess,vl);
	fprintf(stderr,"\n\n");
	
	if ( errno != 0 )
	{
		fprintf(stderr,"perror value:\n");
		perror("");
		fprintf(stderr,"\n");
	}
	
#ifdef HAVE_LINUX_BACKTRACE
	fprintf(stderr,"backtrace:\n");
	void *backtrace_buf[256];
	int n_addresses = backtrace(backtrace_buf,256);
	backtrace_symbols_fd(backtrace_buf, n_addresses, STDERR_FILENO);
	fprintf(stderr,"\n");
#endif
}

void skit_death_cry(char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	skit_death_cry_va(mess, vl);
	va_end(vl);
}

void skit_die_only()
{
	/* TODO: this should probably not crash the program if this is not the main
	 * thread.  If the thread is non-main, then it should just kill the thread. */
	exit(1);
	/*lib$signal(EXIT_FAILURE);*/ /* This produced too much spam when exiting after a bunch of setjmp/longjmps. */
}

void skit_die(char *mess, ...)
{
	va_list vl;
	va_start(vl, mess);
	skit_death_cry_va(mess, vl);
	va_end(vl);
	
	skit_die_only();
}

void skit_print_mem(void *ptr, int size)
{
	#define LINE_SIZE 8
	char xline[LINE_SIZE][3];
	char line[LINE_SIZE+1];
	/* xline[LINE_SIZE] = '\0'; */
	line[LINE_SIZE] = '\0';
	int pos = 0;
		printf("ADDR: 00 01 02 03 . 04 05 06 07 : text\n");
	while ( pos < size )
	{
		int line_width = LINE_SIZE;
		int left = size-pos;
		if ( left < LINE_SIZE )
			line_width = left;
	
		int i = 0;
		for ( i = 0; i < LINE_SIZE; i++ )
		{
			unsigned char c;
			if ( i < line_width )
			{
				c = ((char*)ptr)[pos+i];
				sprintf(xline[i],"%02x",c);
			}
			else
			{
				c = ' ';
				xline[i][0] = ' ';
				xline[i][1] = ' ';
				xline[i][2] = '\0';
			}
			
			/* printable ASCII chars */
			if ( 32 <= c && c <= 126 )
				line[i] = c;
			else
				line[i] = '.';
		}
		
		printf("%04x: %s %s %s %s . %s %s %s %s : %s\n",
			pos,
			xline[0], xline[1], xline[2], xline[3],
			xline[4], xline[5], xline[6], xline[7],
			line);
		pos += LINE_SIZE;
	}
	printf("\n");
}

const char *skit_errno_to_cstr( char *buf, size_t buf_size)
{
		#ifdef __VMS
			/* strerror_r is not provided on OpenVMS, but supposedly strerror is thread-safe on VMS.
			source: http://www.mail-archive.com/boost-cvs@lists.sourceforge.net/msg08433.html
			(Search for "VMS doesn't provide strerror_r, but on this platform")
			*/
			const char *result = strerror(errno);
		#elif defined(__linux__)
			/* Linux: do it the normal way. */
			char *result = buf;
			
			/* Reference: http://linux.die.net/man/3/strerror_r */
			#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
				int errval = strerror_r(errno, result, buf_size);
				(void)errval;
				/* TODO: what to do if (errval != 0) ? */
			#else
				result = strerror_r(errno, buf, buf_size);
			#endif
		#else
		#	error "This requires porting."
		#endif
		
		return buf;
}
