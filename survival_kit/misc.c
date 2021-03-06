
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
#include <netdb.h> // EAI_* error codes.

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
	return skit_error_code_to_cstr(errno, buf, buf_size);
}

const char *skit_error_code_to_cstr( int error_code, char *buf, size_t buf_size)
{
	#ifdef __VMS
		/* strerror_r is not provided on OpenVMS, but supposedly strerror is thread-safe on VMS.
		source: http://www.mail-archive.com/boost-cvs@lists.sourceforge.net/msg08433.html
		(Search for "VMS doesn't provide strerror_r, but on this platform")
		*/
		const char *result = strerror(error_code);
	#elif defined(__linux__)
		/* Linux: do it the normal way. */
		char *result = buf;
		
		/* Reference: http://linux.die.net/man/3/strerror_r */
		#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
			int errval = strerror_r(error_code, result, buf_size);
			(void)errval;
			/* TODO: what to do if (errval != 0) ? */
		#else
			result = strerror_r(error_code, buf, buf_size);
		#endif
	#else
	#	error "This requires porting."
	#endif
	
	return buf;
}

// Implementation ripped from PostgreSQL source code. (Hopefully no one minds.)
// They seem to know what's going on.
const char *skit_gai_strerror(int errcode)
{
#ifdef HAVE_HSTRERROR
	int			hcode;

	switch (errcode)
	{
		case EAI_NONAME:
			hcode = HOST_NOT_FOUND;
			break;
		case EAI_AGAIN:
			hcode = TRY_AGAIN;
			break;
		case EAI_FAIL:
		default:
			hcode = NO_RECOVERY;
			break;
	}

	return hstrerror(hcode);
#else							/* !HAVE_HSTRERROR */

	switch (errcode)
	{
		case EAI_NONAME:
			return "Unknown host";
		case EAI_AGAIN:
			return "Host name lookup failure";
			/* Errors below are probably WIN32 only */
#ifdef EAI_BADFLAGS
		case EAI_BADFLAGS:
			return "Invalid argument";
#endif
#ifdef EAI_FAMILY
		case EAI_FAMILY:
			return "Address family not supported";
#endif
#ifdef EAI_MEMORY
		case EAI_MEMORY:
			return "Not enough memory";
#endif
#if defined(EAI_NODATA) && EAI_NODATA != EAI_NONAME		/* MSVC/WIN64 duplicate */
		case EAI_NODATA:
			return "No host data of that type was found";
#endif
#ifdef EAI_SERVICE
		case EAI_SERVICE:
			return "Class type not found";
#endif
#ifdef EAI_SOCKTYPE
		case EAI_SOCKTYPE:
			return "Socket type not supported";
#endif
		default:
			return "Unknown server error";
	}
#endif   /* HAVE_HSTRERROR */
}

