
#ifndef SKIT_MISC_INCLUDED
#define SKIT_MISC_INCLUDED

#include <stdlib.h>
#include <stdarg.h>

/** Print the given message along with any other error messages that would
normally get printed at this point if "skit_die" was called. */
void skit_death_cry(char *mess, ...);
void skit_death_cry_va(char *mess, va_list vl); /** ditto */

/** Die without printing any error messages. */
void skit_die_only();

/**
Unconditionally exit the program and print the formatted string 'mess' along
with any other diagnostic information available to the survival kit context.
*/
void skit_die(char *mess, ...);

/** 
Prints a chunk of memory in both hexadecimal and text form. 
TODO: there is a current limitation that 'size' must be a multiple of 8.
*/
void skit_print_mem(void *ptr, int size);

/**
Returns an error message that corresponds to the posix errno value.
The string returned may point to some form of static memory or internal
buffer and should not be modified or freed by the caller.
"buf" may or may not be used.
This is very similar to the GNU-specific version of strerror_r, but is
provided as a way to get a consistent interface accross operating systems.
This should be as safe to use in signals as is possible.
This function will try to avoid allocations.  (TODO: does strerror on VMS guarantee this?)
*/
const char *skit_errno_to_cstr( char *buf, size_t buf_size);

/**
Constructs an inheritence table suitable for use with
  skit_is_a(table,table_size,parent,child).

The 'table' argument should be a pointer to an array (a table) that describes
parent-child relationships.  It is OK for (*table == NULL) to be true when
calling: this will cause the table to be allocated.  The table is passed as
a reference to allow the original pointer to be changed during allocations.

The 'child' argument should be a pointer to a variable that will hold the
ID/index assigned to that child in the table.  

The 'parent' argument should be a pointer to a variable that was already
populated as a child somewhere else using skit_register_parent_child_rel.

It is OK to make calls like
  skit_register_parent_child_rel(&table, &table_size, &x, &x);
as a way to establish 'x' as a root in the inheritence hierarchy.
*/
void skit_register_parent_child_rel( ssize_t **table, ssize_t *table_size, ssize_t *child, const ssize_t *parent );

/**
Given a table describing a parent-child inheritence relationship, determine
whether or not two indices into the table satisfy the "is a" relationship.
The table should be constructed such that
  table[child_index] == parent_index
or
  (table[index] == index) when there is no parent (roots).
is always true.
This can be done by repeatedly calling skit_register_parent_child_rel.

Returns 1 when ('index1' is a 'index2') according to 'table'.
Returns 0 otherwise.
*/
int skit_is_a( ssize_t *table, ssize_t table_size, ssize_t index1, ssize_t index2 );

void skit_misc_unittest();

#endif
