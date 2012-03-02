
#include "STR_UTIL.H"
#include "ERR_UTIL.H"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h> /* For ssize_t */

string *str_init(string *str)
{
	str->ptr = NULL;
	str->length = 0;
	return str;
}

string *str_malloc(string *str, size_t length)
{
	if ( str == NULL )
		die("str_malloc(NULL)");
	else if ( str->ptr != NULL ) {
		die(
			"str_malloc: Argument str->ptr is not NULL.\n"
			"str_malloc: This could be an attempt to overwrite a memory-leaked string.\n"
			"str_malloc: Be sure to use str_init() or str_delete() on the string before\n"
			"str_malloc:   passing it into str_malloc().\n");
	}
	
	str->ptr = malloc(length+1);
	str->length = length;
	*(str->ptr + str->length) = '\0';
	return str;
}

string str_literal(const char *lit)
{
	string str;
	str.length = strlen(lit);
	str.ptr = malloc(str.length+1);
	memcpy(str.ptr, lit, str.length);
	*(str.ptr + str.length) = '\0';
	return str;
}

string *str_append(string *str1, string str2)
{
	if ( str1 == NULL )
		die("str_append(NULL,???)");
	else if ( str1->ptr == NULL && str2.ptr == NULL )
		die("str_append(ptr=NULL,ptr=NULL)");
	else if ( str1->ptr == NULL )
		die("str_append(ptr=NULL,'%.60s')",str2.ptr);
	else if ( str2.ptr == NULL )
		die("str_append('%.60s',ptr=NULL)");
	
	/* TODO: slicing safety:
	if ( str_is_slice(str1) )
		die("attempt to append onto a slice.  Call "dup" first.");
	*/
	
	if ( str2.length <= 0 )
		return str1;
	
	char *offset = str1->ptr + str1->length;
	str1->length += str2.length;
	str1->ptr = realloc(str1->ptr, str1->length+1);
	memcpy(offset, str2.ptr, str2.length);
	*(str1->ptr + str1->length) = '\0';
	return str1;
}

string str_dup(string str)
{
	if ( str.ptr == NULL )
		die("str_dup(ptr=NULL)");

	string newstr;
	newstr.ptr = malloc(str.length+1);
	newstr.length = str.length;
	memcpy(newstr.ptr, str.ptr, str.length);
	*(newstr.ptr + newstr.length) = '\0';
	return newstr;
}

string *str_delete(string *str)
{
	free(str->ptr);
	str->ptr = NULL;
	str->length = 0;
	return str;
}


/* ------------------------- string misc functions ------------------------- */

#define IS_WHITESPACE(x) (x == ' '  || x == '\t' || x == '\r' || x == '\n' )

string str_ltrim(string str)
{
	string slice;
	slice.ptr    = str.ptr;
	slice.length = str.length;
	
	while ( slice.length > 0 )
	{
		char c = *slice.ptr;
		if (!IS_WHITESPACE(c))
			break;
		
		slice.ptr++;
		slice.length--;
	}
	
	return slice;
}

string str_rtrim(string str)
{
	string slice;
	slice.ptr    = str.ptr;
	slice.length = str.length;
	
	while ( slice.length > 0 )
	{
		char c = slice.ptr[slice.length-1];
		if (!IS_WHITESPACE(c))
			break;
		
		slice.length--;
	}
	
	return slice;
}

string str_trim(string str)
{
	return str_ltrim( str_rtrim(str) );
}

string str_ltruncate(string str, size_t nchars)
{
	if ( nchars > str.length )
	{
		/* The requested truncation is greater than the string length. */
		/* In this case we return a zero-length slice at the end of the string. */
		str.ptr = str.ptr + str.length;
		str.length = 0;
		return str;
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		str.ptr += nchars;
		str.length -= nchars;
		return str;
	}
}

string str_rtruncate(string str, size_t nchars)
{
	if ( nchars > str.length )
	{
		/* The requested truncation is greater than the string length. */
		/* In this case we return a zero-length slice at the beginning of the string. */
		str.length = 0;
		return str;
	}
	else
	{
		/* Nothing unusual.  Truncate as normal. */
		str.length -= nchars;
		return str;
	}
}

