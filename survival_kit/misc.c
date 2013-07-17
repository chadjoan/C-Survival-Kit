
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
#include "survival_kit/memory.h"

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


/* ------------------------------------------------------------------------- */

void skit_register_parent_child_rel( ssize_t **table, ssize_t *table_size, ssize_t *child, const ssize_t *parent )
{
	/* 
	Note that parent is passed by reference and not value.
	This is to ensure that cases like 
	  skit_register_parent_child_rel(table, table_sz, &x, &x)
	will actually do what is expected: make the 'x' inherit from itself.
	If parent were passed by value, then the above expression might fill the
	'parent' argument with whatever random garbage the compiler left in the
	initial value of pointed to by 'x'.  *child = ...; will assign the original
	variable a sane value, but the parent value in the table will still end
	up being the undefined garbage value.  By passing parent as a pointer we
	ensure that *child = ...; will also populate the parent value in cases
	where an index is defined as its own parent (root-level).
	
	As an example:
	ssize_t *table = NULL; 
	ssize_t table_sz = 0;
	ssize_t x;
	skit_register_parent_child_rel(&table, &table_sz, &x, x);
	
	Would eventually cause a situation where the equivalent of these statements execute:
	old_x = x;
	*x = 0;
	table[*x] = old_x;
	Since the original x was uninitialized it could be anything, like 1076.
	Now table[0] == 1076, even though there is no 1076'th element in the table.
	
	So instead we write 
	skit_register_parent_child_rel(&table, &table_sz, &x, &x);
	
	Which plays out like this:
	*x = 0;
	table[*x] = *x;
	Thus table[0] = 0; and the desired result is obtained.
	*/
	
	assert(child != NULL);
	assert(parent != NULL);
	assert(table != NULL);
	assert(table_size != NULL);
	
	*child = *table_size;
	
	if ( *table == NULL )
	{
		/* If this doesn't already exist, create the table and give it 1 element. */
		*table = skit_malloc(sizeof(ssize_t));
		(*table_size) = 1;
	}
	else
	{
		/* If this does exist, it will need to grow by one element. */
		(*table_size) += 1;
		*table = skit_realloc( *table, sizeof(ssize_t) * *table_size);
	}
	
	(*table)[*child] = *parent;
}

/* ------------------------------------------------------------------------- */

int skit_is_a( ssize_t *table, ssize_t table_size, ssize_t index1, ssize_t index2 )
{
	ssize_t child_index = index2;
	ssize_t parent_index = index1;
	ssize_t last_parent = 0;
	while (1)
	{
		/* Invalid parent indices. */
		if ( parent_index >= table_size || parent_index < 0 )
			assert(0);
		
		/* Match found! */
		if ( child_index == parent_index )
			return 1;

		/* We've hit the root-level without establishing a parent-child relationship. No good. */
		if ( last_parent == parent_index )
			return 0;
		
		last_parent = parent_index;
		parent_index = table[parent_index];
	}
	assert(0);
}

static void skit_inheritence_table_test()
{
	ssize_t *table = NULL;
	ssize_t table_sz = 0;
	ssize_t a, b, aa, ab, ac, ba, aaa, aab, abb;
	
	/* Construct this graph:
	*
	*             a                        b
	*       ,-----+-----.--------.         |
	*      aa           ab       ac        ba
	*   ,--+----.        |
	*  aaa     aab      abb
	*
	*/
	skit_register_parent_child_rel(&table, &table_sz, &a, &a);
	skit_register_parent_child_rel(&table, &table_sz, &aa, &a);
	skit_register_parent_child_rel(&table, &table_sz, &aaa, &aa);
	skit_register_parent_child_rel(&table, &table_sz, &aab, &aa);
	skit_register_parent_child_rel(&table, &table_sz, &ab, &a);
	skit_register_parent_child_rel(&table, &table_sz, &abb, &ab);
	skit_register_parent_child_rel(&table, &table_sz, &ac, &a);
	skit_register_parent_child_rel(&table, &table_sz, &b, &b);
	skit_register_parent_child_rel(&table, &table_sz, &ba, &b);
	
	/* Positives. */
	assert(skit_is_a(table, table_sz, a, a));
	assert(skit_is_a(table, table_sz, aa, a));
	assert(skit_is_a(table, table_sz, aa, aa));
	assert(skit_is_a(table, table_sz, aaa, a));
	assert(skit_is_a(table, table_sz, aaa, aa));
	assert(skit_is_a(table, table_sz, aaa, aaa));
	assert(skit_is_a(table, table_sz, aab, a));
	assert(skit_is_a(table, table_sz, aab, aa));
	assert(skit_is_a(table, table_sz, aab, aab));
	assert(skit_is_a(table, table_sz, ab, a));
	assert(skit_is_a(table, table_sz, ab, ab));
	assert(skit_is_a(table, table_sz, abb, a));
	assert(skit_is_a(table, table_sz, abb, ab));
	assert(skit_is_a(table, table_sz, abb, abb));
	assert(skit_is_a(table, table_sz, ac, a));
	assert(skit_is_a(table, table_sz, ac, ac));
	assert(skit_is_a(table, table_sz, b, b));
	assert(skit_is_a(table, table_sz, ba, b));
	assert(skit_is_a(table, table_sz, ba, ba));
	
	/* Negatives. (Not exhaustive.) */
	assert(!skit_is_a(table, table_sz, a, b));
	assert(!skit_is_a(table, table_sz, ab, aa));
	assert(!skit_is_a(table, table_sz, ac, aa));
	assert(!skit_is_a(table, table_sz, ab, ac));
	assert(!skit_is_a(table, table_sz, aa, b));
	assert(!skit_is_a(table, table_sz, ab, b));
	assert(!skit_is_a(table, table_sz, ac, b));
	assert(!skit_is_a(table, table_sz, ba, a));
	assert(!skit_is_a(table, table_sz, ba, ac));
	assert(!skit_is_a(table, table_sz, aaa, aab));
	assert(!skit_is_a(table, table_sz, aaa, abb));
	assert(!skit_is_a(table, table_sz, aab, abb));
	assert(!skit_is_a(table, table_sz, aaa, ab));
	assert(!skit_is_a(table, table_sz, abb, aa));
	
	printf("  skit_inheritence_table_test passed.\n");
}

void skit_misc_unittest()
{
	printf("skit_misc_unittest\n");
	skit_inheritence_table_test();
	printf("  skit_misc_unittest passed!\n");
	printf("\n");
}
