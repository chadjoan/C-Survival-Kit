#ifndef STR_UTIL_INCLUDED
#define STR_UTIL_INCLUDED

#include <stdlib.h>
#include <unistd.h> /* For ssize_t */

typedef struct string string;
struct string
{
	char *ptr;
	ssize_t length;
};


/* 
Zeroes/Nulls out the string. 
This MUST be called on uninitialized strings before they are fed to 
  str_malloc or any function that expects a NULL string (str->ptr). 
*/
string *str_init(string *str);

/* 
Allocate 'length' bytes for the string 'str'.
str->ptr should be NULL before this is called. 
*/
string *str_malloc(string *str, size_t length);

/*
Similar to str_malloc, except that non-null str->ptr's will be reallocated.
*/
string *str_realloc(string *str, size_t length);

/* Allocates a string and fills it with the given nul-terminated */
/* c-string literal. */
string str_literal(const char *lit);

/* Appends 'str2' onto the end of 'str1'. */
/* The additional memory needed is created using realloc. */
string *str_append(string *str1, string str2);

/* Duplicates the given string 'str'. */
/* This is an allocating function, so be sure to clean up as needed. */
string str_dup(string str);

/* Use this to free memory held by 'str'. */
/* str will be reinitialized to have a NULL ptr and zero length. */
string *str_delete(string *str);


/* ------------------------- string misc functions ------------------------- */

/* Trim whitespace from 'str'. */
string str_ltrim(string str);
string str_rtrim(string str);
string str_trim(string str);

/* Truncate 'nchars' from the left or right side of 'str'. */
string str_ltruncate(string str, size_t nchars);
string str_rtruncate(string str, size_t nchars);

#endif