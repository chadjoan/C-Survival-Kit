
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

#include "survival_kit/feature_emulation/types.h"
#include "survival_kit/feature_emulation/funcs.h"
#include "survival_kit/misc.h"

void skit_die(char *mess, ...)
{
	skit_thread_context *skit_thread_ctx = skit_thread_context_get();
	
	va_list vl;
	va_start(vl, mess);
	vsnprintf(skit_thread_ctx->error_text_buffer, skit_thread_ctx->error_text_buffer_size, mess, vl);
	va_end(vl);
	
	fprintf(stderr,"\n");
	fprintf(stderr,"ERROR: skit_die was called.\n");
	fprintf(stderr,"Message:\n");
	
	fprintf(stderr,"%s\n",skit_thread_ctx->error_text_buffer);
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

void skit_free(void *mem)
{
	free(mem);
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
