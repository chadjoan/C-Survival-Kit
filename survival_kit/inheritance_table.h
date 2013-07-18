
#ifndef SKIT_INHERITANCE_TABLE_INCLUDED
#define SKIT_INHERITANCE_TABLE_INCLUDED

#include <sys/types.h> /* ssize_t */

typedef struct skit_inheritance_table skit_inheritance_table;
struct skit_inheritance_table
{
	ssize_t *table;
	ssize_t table_size;
	
	/*
	This is another table parallel with 'table'.
	It is used to ensure that things stay sane when the inheritence table
	is constructed out-of-order (ex: children are defined before parents).
	It is an array of pointers to IDs.
	See skit_inher_id_is_initialized for usage.
	*/
	ssize_t **initialized_ptrs;
};

void skit_inheritance_table_ctor(skit_inheritance_table *table);

void skit_inheritance_table_dtor(skit_inheritance_table *table);

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
void skit_register_parent_child_rel( skit_inheritance_table *table, ssize_t *child, ssize_t *parent );

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
int skit_is_a( skit_inheritance_table *table, ssize_t index1, ssize_t index2 );

void skit_inheritance_table_unittest();

#endif