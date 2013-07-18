
#ifdef __DECC
#pragma module skit_inheritance_table
#endif

/* Don't include this: it could create circular dependencies. */
/* In principle, anything that relies on feature emulation should be fairly domain-specific. */
/* If this principle is violated, then there will need to be some way to break apart the */
/*   contents of this "misc" module. (Or make the feature_emulation usage optional by macro #ifdef'ing.) */
/* #include "survival_kit/feature_emulation.h" */

#include <assert.h>
#include <sys/types.h> /* ssize_t */
#include <stddef.h>
#include <stdio.h>

#include "survival_kit/inheritance_table.h"
#include "survival_kit/memory.h"


/* TODO: maybe there is a way to make this work with streams? */
/* Beware of circular dependencies, though.  This would probably require */
/*   a "core" inheritance table module usable by exceptions, plus an */
/*   extension that publicly includes the core module and adds stuff */
/*   like a skit_inheritance_table_to_stream method. */
/*
static void skit_inheritance_table_printf( skit_inheritance_table *table )
{
	if ( table->table == NULL )
	{
		printf("table->table == %p\n", table->table);
		printf("table->table_size == %ld\n", table->table_size);
		printf("table->initialized_ptrs = %p\n", table->initialized_ptrs);
	}
	else
	{
		size_t i = 0;
		printf("table->table_size == %ld\n", table->table_size);
		printf("table->table = \n");
		for ( i = 0; i < table->table_size; i++ )
			printf("table->table[%ld] == %ld\n", i, table->table[i]);
		for ( i = 0; i < table->table_size; i++ )
			printf("table->initialized_ptrs[%ld] == %p -> %ld\n", i,
				table->initialized_ptrs[i], *(table->initialized_ptrs[i]) );
	}
}
*/

/* ------------------------------------------------------------------------- */

void skit_inheritance_table_ctor(skit_inheritance_table *table)
{
	table->table = NULL;
	table->table_size = 0;
	table->initialized_ptrs = NULL;
}

void skit_inheritance_table_dtor(skit_inheritance_table *table)
{
	if ( table->table != NULL )
		skit_free(table->table);
	if ( table->initialized_ptrs != NULL )
		skit_free(table->initialized_ptrs);
	
	table->table = NULL;
	table->table_size = 0;
	table->initialized_ptrs = NULL;
}

static int skit_inher_id_is_initialized(const skit_inheritance_table *table, const ssize_t *id)
{
	if ( *id < 0 || *id >= table->table_size )
		return 0;
	return ( table->initialized_ptrs[*id] == id );
}

static void skit_inher_id_mark_initialized(skit_inheritance_table *table, ssize_t *id)
{
	assert( *id < table->table_size );
	table->initialized_ptrs[*id] = id;
}

void skit_register_parent_child_rel( skit_inheritance_table *table, ssize_t *child, ssize_t *parent )
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
	
	/* If the parent isn't initialized, then make it a root. */
	/* If the caller didn't want that, then they can overwrite it with */
	/*   another call to skit_register_parent_child_rel() */
	/* This is necessary to support out-of-order initialization. */
	if ( child != parent && !skit_inher_id_is_initialized(table, parent) )
		skit_register_parent_child_rel(table, parent, parent);
	
	if ( skit_inher_id_is_initialized(table, child) )
	{
		/* Simple re-assignment. */
		table->table[*child] = *parent;
	}
	else
	{
		*child = table->table_size;
	
		if ( table->table == NULL )
		{
			/* If this doesn't already exist, create the table and give it 1 element. */
			table->table = skit_malloc(sizeof(ssize_t));
			table->initialized_ptrs = skit_malloc(sizeof(ssize_t*));
			table->table_size = 1;
		}
		else
		{
			/* If this does exist, it will need to grow by one element. */
			(table->table_size) += 1;
			table->table = skit_realloc( table->table, sizeof(ssize_t) * table->table_size);
			table->initialized_ptrs = 
				skit_realloc( table->initialized_ptrs, sizeof(ssize_t*) * table->table_size);
		}
		
		table->table[*child] = *parent;
		skit_inher_id_mark_initialized(table,child);
	}
}

/* ------------------------------------------------------------------------- */

int skit_is_a( skit_inheritance_table *table, ssize_t index1, ssize_t index2 )
{
	ssize_t child_index = index2;
	ssize_t parent_index = index1;
	ssize_t last_parent = 0;
	while (1)
	{
		/* Invalid parent indices. */
		if ( parent_index >= table->table_size || parent_index < 0 )
			assert(0);
		
		/* Match found! */
		if ( child_index == parent_index )
			return 1;

		/* We've hit the root-level without establishing a parent-child relationship. No good. */
		if ( last_parent == parent_index )
			return 0;
		
		last_parent = parent_index;
		parent_index = table->table[parent_index];
	}
	assert(0);
}

static void skit_inheritance_table_test()
{
	skit_inheritance_table table;
	ssize_t a, b, aa, ab, ac, ba, aaa, aab, abb;
	
	skit_inheritance_table_ctor(&table);
	
	/* Construct this graph:
	*
	*             a                        b
	*       ,-----+-----.--------.         |
	*      aa           ab       ac        ba
	*   ,--+----.        |
	*  aaa     aab      abb
	*
	*/
	skit_register_parent_child_rel(&table, &a, &a);
	skit_register_parent_child_rel(&table, &aa, &a);
	skit_register_parent_child_rel(&table, &aaa, &aa);
	skit_register_parent_child_rel(&table, &aab, &aa);
	skit_register_parent_child_rel(&table, &ab, &a);
	skit_register_parent_child_rel(&table, &abb, &ab);
	skit_register_parent_child_rel(&table, &ac, &a);
	
	/* test out-of-order initialization. */
	skit_register_parent_child_rel(&table, &ba, &b);
	skit_register_parent_child_rel(&table, &b, &b);
	
	/* Positives. */
	assert(skit_is_a(&table, a, a));
	assert(skit_is_a(&table, aa, a));
	assert(skit_is_a(&table, aa, aa));
	assert(skit_is_a(&table, aaa, a));
	assert(skit_is_a(&table, aaa, aa));
	assert(skit_is_a(&table, aaa, aaa));
	assert(skit_is_a(&table, aab, a));
	assert(skit_is_a(&table, aab, aa));
	assert(skit_is_a(&table, aab, aab));
	assert(skit_is_a(&table, ab, a));
	assert(skit_is_a(&table, ab, ab));
	assert(skit_is_a(&table, abb, a));
	assert(skit_is_a(&table, abb, ab));
	assert(skit_is_a(&table, abb, abb));
	assert(skit_is_a(&table, ac, a));
	assert(skit_is_a(&table, ac, ac));
	assert(skit_is_a(&table, b, b));
	assert(skit_is_a(&table, ba, b));
	assert(skit_is_a(&table, ba, ba));
	
	/* Negatives. (Not exhaustive.) */
	assert(!skit_is_a(&table, a, b));
	assert(!skit_is_a(&table, ab, aa));
	assert(!skit_is_a(&table, ac, aa));
	assert(!skit_is_a(&table, ab, ac));
	assert(!skit_is_a(&table, aa, b));
	assert(!skit_is_a(&table, ab, b));
	assert(!skit_is_a(&table, ac, b));
	assert(!skit_is_a(&table, ba, a));
	assert(!skit_is_a(&table, ba, ac));
	assert(!skit_is_a(&table, aaa, aab));
	assert(!skit_is_a(&table, aaa, abb));
	assert(!skit_is_a(&table, aab, abb));
	assert(!skit_is_a(&table, aaa, ab));
	assert(!skit_is_a(&table, abb, aa));
	
	printf("  skit_inheritence_table_test passed.\n");
}

/* ------------------------------------------------------------------------- */

void skit_inheritance_table_unittest()
{
	printf("skit_inheritance_table_unittest\n");
	skit_inheritance_table_test();
	printf("  skit_inheritance_table_unittest passed!\n");
	printf("\n");
}