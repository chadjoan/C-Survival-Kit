
#if defined(__DECC)
#pragma module skit_trie
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h> /* For ssize_t */

#include "survival_kit/feature_emulation.h"
#include "survival_kit/string.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/trie.h"

/* ------------------------------------------------------------------------- */

skit_err_code SKIT_TRIE_EXCEPTION;
skit_err_code SKIT_TRIE_KEY_ALREADY_EXISTS;
skit_err_code SKIT_TRIE_KEY_NOT_FOUND;
skit_err_code SKIT_TRIE_BAD_FLAGS;
skit_err_code SKIT_TRIE_WRITE_IN_ITERATION;

void skit_trie_module_init()
{
	//                      Exception,                    Derived from,        Default message
	SKIT_REGISTER_EXCEPTION(SKIT_TRIE_EXCEPTION,          SKIT_EXCEPTION,      "Generic trie exception.");
	SKIT_REGISTER_EXCEPTION(SKIT_TRIE_KEY_ALREADY_EXISTS, SKIT_TRIE_EXCEPTION, "Wrote a value to a key that already exists. 'o' (overwrite) not passed in flags.");
	SKIT_REGISTER_EXCEPTION(SKIT_TRIE_KEY_NOT_FOUND,      SKIT_TRIE_EXCEPTION, "Wrote a value to a non-existant key. 'c' (create) not passed in flags.");
	SKIT_REGISTER_EXCEPTION(SKIT_TRIE_BAD_FLAGS,          SKIT_TRIE_EXCEPTION, "The flags parameter contained invalid characters or an invalid combination.");
	SKIT_REGISTER_EXCEPTION(SKIT_TRIE_WRITE_IN_ITERATION, SKIT_TRIE_EXCEPTION, "Attempt to do set or remove on a skit_trie with an iterator open.");
}

/* ------------------------------------------------------------------------- */

typedef enum
{
	ICASE     = 0x01,
	CREATE    = 0x02,
	OVERWRITE = 0x04

} skit_trie_flags;

static skit_trie_flags skit_parse_flags( const char *trie_flags )
{
	SKIT_USE_FEATURE_EMULATION;
	skit_trie_flags result = 0;
	if ( trie_flags == NULL )
		return result;

	while(1)
	{
		switch( *(trie_flags++) )
		{
			case 'i': result |= ICASE; break;
			case 'c': result |= CREATE; break;
			case 'o': result |= OVERWRITE; break;
			case '\0': return result;
			default: sTHROW(SKIT_TRIE_BAD_FLAGS, "Unexpected character %c in trie flags.\n", *trie_flags);
		}
	}

	sASSERT(0);
	return 0;
}

/* ------------------------------------------------------------------------- */

/*
4 possible states for these:
- exact match
- prefix match
- incomplete match (continue calling skit_trie_find to reach a conclusion)
- no match
*/

typedef struct skit_trie_coords skit_trie_coords;
struct skit_trie_coords
{
	skit_trie_node  *node;
	uint32_t        pos; /* Location of the first character that disagrees with the key.  If equal to key length, there is an exact match. */
	uint8_t         n_chars_into_node;
	uint8_t         lookup_stopped;
};

/* ------------------------------------------------------------------------- */

static void skit_trie_coords_ctor( skit_trie_coords *coords )
{
	coords->node = NULL;
	coords->pos = 0;
	coords->n_chars_into_node = 0;
	coords->lookup_stopped = 0;
}

/* ------------------------------------------------------------------------- */

static skit_trie_coords skit_trie_stop_lookup(skit_trie_node *node, uint32_t pos, uint8_t n_chars_into_node)
{
	skit_trie_coords result;
	result.node = node;
	result.pos = pos;
	result.n_chars_into_node = n_chars_into_node;
	result.lookup_stopped = 1;
	return result;
}

/* ------------------------------------------------------------------------- */

static skit_trie_coords skit_trie_continue_lookup(skit_trie_node *node, uint32_t pos)
{
	skit_trie_coords result;
	result.node = node;
	result.pos = pos;
	result.n_chars_into_node = 0;
	result.lookup_stopped = 0;
	return result;
}

/* ------------------------------------------------------------------------- */

static int skit_exact_match( skit_trie_coords coords, size_t key_len )
{
	return ( coords.pos == key_len && coords.node->have_value );
}

static int skit_prefix_match( skit_trie_coords coords, size_t key_len )
{
	return ( coords.lookup_stopped && key_len == coords.pos && !coords.node->have_value );
}

static int skit_no_match( skit_trie_coords coords, size_t key_len )
{
	return ( coords.node == NULL || (coords.lookup_stopped && coords.pos < key_len) );
}

/* ------------------------------------------------------------------------- */

static skit_trie_coords skit_trie_next_node(
	skit_trie_node *current,
	const char *key,
	size_t key_len,
	size_t pos)
{

	sASSERT(current->nodes != NULL);
	sASSERT(current->nodes_len > 0);

	if ( current->nodes_len == 1 )
	{
		/*
		Narrow radix search: there is only one possible node to
		end up at.  The string to match to get there is the one
		contained in the 'chars' member and is no longer than
		'chars_len'
		*/
		size_t chars_len = current->chars_len;

		size_t i = 0;
		while ( i < chars_len )
		{
			if ( pos > key_len || key[pos] != current->chars[i] )
				return skit_trie_stop_lookup(current, pos, i);

			pos++;
			i++;
		}

		return skit_trie_continue_lookup(&current->nodes[0], pos);
	}
	else if ( current->chars_len < SKIT__TRIE_NODE_PREALLOC )
	{
		/*
		There is more than one choice, but few enough that we can
		fit them in a small amount of space.
		We scan the 'chars' array; when an element in that matches the
		current char in our key, then that index is the index to use
		in the 'nodes' array.
		*/
		sASSERT( current->chars_len == current->nodes_len );

		if ( pos >= key_len )
			return skit_trie_stop_lookup(current, pos, 0);

		size_t chars_len = current->chars_len;
		size_t i = 0;
		for ( ; i < chars_len; i++ )
			if ( current->chars[i] == key[pos] )
				break;

		if ( i == chars_len )
			return skit_trie_stop_lookup(current, pos, 0);

		return skit_trie_continue_lookup(&current->nodes[i], pos+1);
	}
	else
	{
		/*
		In this case, there are so many possible paths to take, that
		it becomes worth it to allocate a 256 *node table and do a
		lookup on the given character.  Ideally, the lookup will be
		faster than any attempts to scan through a smaller array
		of characters.
		*/
		if ( pos >= key_len )
			return skit_trie_stop_lookup(current, pos, 0);

		skit_trie_node *next_node = current->node_table[(uint8_t)key[pos]];
		if ( next_node == NULL )
			return skit_trie_stop_lookup(current, pos, 0);

		return skit_trie_continue_lookup(next_node, pos+1);
	}

	sASSERT(0);
}

/* ------------------------------------------------------------------------- */

static skit_trie_coords skit_trie_find(
	skit_trie *trie,
	const char *key,
	size_t key_len)
{
	sASSERT(key != NULL);

	if ( trie->root == NULL )
		return skit_trie_stop_lookup(NULL, 0, 0);

	skit_trie_coords coords;
	skit_trie_coords prev;
	
	skit_trie_coords_ctor(&coords);
	skit_trie_coords_ctor(&prev);
	
	char *key_return_buf = (char*)sLPTR(trie->key_return_buf);
	skit_trie_node *next = trie->root;
	while ( 1 )
	{
		prev = coords;
		coords = skit_trie_next_node(next, key, key_len, 0);

		/* Keep key_return_buf up-to-date. */
		size_t copy_len = coords.pos - prev.pos;
		if ( copy_len > 0 )
		{
			void *dst_start = key_return_buf + prev.pos;
			void *src_start = prev.node->chars;
			memcpy(dst_start, src_start, copy_len);
		}
		
		if ( coords.lookup_stopped )
			break;

		if ( skit_exact_match(coords, key_len) )
			return coords; /* Match success. DONE. */
	}
	
	/* If we only made it partway through a linear node, copy that into the */
	/*    key buffer as well. */
	if ( coords.n_chars_into_node > 0 )
	{
		void *dst_start = key_return_buf + coords.pos - coords.n_chars_into_node;
		void *src_start = coords.node->chars;
		size_t copy_len = coords.n_chars_into_node;
		memcpy(dst_start, src_start, copy_len);
	}

	return coords;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_node_dtor(skit_trie_node *node)
{
	size_t i = 0;
	
	if ( node->nodes_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		for(; i < node->nodes_len; i++ )
			skit_trie_node_dtor(&node->nodes[i]);
		
		skit_free(node->nodes);
	}
	else
	{
		for(; i < 256; i++ )
		{
			if ( node->node_table[i] != NULL )
			{
				skit_trie_node_dtor(node->node_table[i]);
				skit_free(node->node_table[i]);
			}
		}
		
		skit_free(node->node_table);
	}
}

/* ------------------------------------------------------------------------- */

skit_trie *skit_trie_new()
{
	skit_trie *result = skit_malloc(sizeof(skit_trie));
	skit_trie_ctor(result);
	return result;
}

/* ------------------------------------------------------------------------- */

void skit_trie_ctor( skit_trie *trie )
{
	trie->length = 0;
	trie->root = NULL;
	trie->key_return_buf = skit_loaf_new();
	trie->iterator_count = 0;
}

/* ------------------------------------------------------------------------- */

skit_trie *skit_trie_free(skit_trie *trie)
{
	skit_trie_dtor(trie);
	skit_free(trie);
	return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_trie_dtor( skit_trie *trie )
{
	SKIT_USE_FEATURE_EMULATION;
	if ( trie->iterator_count > 0 )
		sTHROW(SKIT_TRIE_WRITE_IN_ITERATION,
			"Call to skit_trie_dtor during iteration. #iters = %d",
			trie->iterator_count);

	if ( trie->root != NULL )
	{
		skit_trie_node_dtor(trie->root);
		skit_free(trie->root);
		trie->root = NULL;
	}

	if ( !skit_loaf_is_null(trie->key_return_buf) )
		trie->key_return_buf = skit_loaf_free(&trie->key_return_buf);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_get( skit_trie *trie, const skit_slice key, void **value, const char *flags )
{
	sASSERT(value != NULL);

	skit_trie_flags tflags = skit_parse_flags( flags );

	sASSERT_MSG(tflags & ICASE, "Case insensitive operations are currently unsupported.");

	size_t key_len = sSLENGTH(key);
	skit_trie_coords coords = skit_trie_find(trie, (char*)sSPTR(key), key_len);

	if ( skit_exact_match(coords, key_len) )
	{
		/* Cast away constness because we are handing this back to the caller. */
		/* We only guarantee that WE won't alter it. */
		*value = (void*)coords.node->value;
		return skit_slice_of(trie->key_return_buf.as_slice, 0, key_len);
	}

	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_getc( skit_trie *trie, const char *key, void **value, const char *flags )
{
	return skit_trie_get(trie, skit_slice_of_cstr(key), value, flags);
}

/* ------------------------------------------------------------------------- */

static void skit_trie_node_ctor( skit_trie_node *node )
{
	node->nodes = NULL;
	node->value = NULL;
	node->nodes_len = 0;
	node->chars_len = 0;
	node->have_value = 0;
}

static skit_trie_node *skit_trie_node_new()
{
	skit_trie_node *result = skit_malloc(sizeof(skit_trie_node));
	skit_trie_node_ctor(result);
	return result;
}

/* ------------------------------------------------------------------------- */

/*
. Converts
.
.     node -> "asdf" -> node -> "qwer" -> value
.
. into
.
.     node -> "asdfqwer" -> value
.
If the tail has more complicated constructs in it, then the folding will
stop at the first non-linear node.
This is purely a space-optimization.
*/
static skit_trie_node *skit_trie_node_fold(skit_trie_node *node)
{
	printf("%s, %d: Stub!\n", __func__, __LINE__);
	return node;
}

static void skit_trie_node_set_value( skit_trie_node *node, const void *value )
{
	node->value = value;
	node->have_value = 1;
}

static void skit_trie_node_insert_non0_str(
	skit_trie_node *node,
	skit_trie_node *tail,
	const char *str_ptr,
	size_t str_len )
{
	sASSERT(node != NULL);
	sASSERT(tail != NULL);
	sASSERT(node != tail);

	skit_trie_node_ctor(node);

	skit_trie_node *next_node;
	if ( str_len > SKIT__TRIE_NODE_PREALLOC )
	{
		next_node = skit_trie_node_new();
		skit_trie_node_insert_non0_str(next_node, tail,
			str_ptr + SKIT__TRIE_NODE_PREALLOC,
			str_len - SKIT__TRIE_NODE_PREALLOC);

		node->chars_len = SKIT__TRIE_NODE_PREALLOC;
	}
	else
	{
		next_node = tail;
		node->chars_len = str_len;
	}

	memcpy(node->chars, str_ptr, node->chars_len);
	node->nodes_len = 1;
	node->nodes = next_node;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_node_append_str(
	skit_trie_node *node,
	skit_trie_node **new_tail,
	const char *str_ptr,
	size_t str_len )
{
	if ( str_len == 0 ) {
		*new_tail = node;
	} else {
		*new_tail = skit_trie_node_new();
		skit_trie_node_insert_non0_str(node, *new_tail, str_ptr, str_len);
	}
}

/* ------------------------------------------------------------------------- */

static skit_trie_node *skit_trie_finish_tail(
	skit_trie_node *given_start_node,
	const skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value)
{
	skit_trie_node *new_value_node;

	skit_trie_node_ctor(given_start_node);

	const char *new_node_str = key_ptr + (coords.pos + 1);
	size_t      new_node_len = key_len - (coords.pos + 1);
	skit_trie_node_append_str(given_start_node, &new_value_node, new_node_str, new_node_len);
	skit_trie_node_set_value(new_value_node, value);
	return given_start_node;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_node(
	skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value);

/* ------------------------------------------------------------------------- */

static void skit_trie_split_table_node(
	skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value)
{
	/* Easy case: just add a new entry in the table. */
	skit_trie_node *node = coords.node;
	sASSERT(node->nodes_len > SKIT__TRIE_NODE_PREALLOC);
	uint8_t new_char = key_ptr[coords.pos];

	/* skit_trie_finish_tail ensures that the rest of the key gets accounted for. */
	node->node_table[new_char] =
		skit_trie_finish_tail(skit_trie_node_new(), coords, key_ptr, key_len, value);

	(node->nodes_len)++;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_multi_to_table(
	skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value)
{
	size_t i;
	skit_trie_node *node = coords.node;
	sASSERT(node->nodes_len == node->chars_len);
	sASSERT(node->nodes_len == SKIT__TRIE_NODE_PREALLOC);
	uint8_t new_char = key_ptr[coords.pos];

	/* Default initialization for a node's lookup table. */
	skit_trie_node **new_node_array = skit_malloc(sizeof(skit_trie_node*)*256);
	for ( i = 0; i < 256; i++ )
		new_node_array[i] = NULL;

	/* COPY all of the nodes out of the node->nodes array and into their */
	/*   own blocks of memory, which are then indexed by node->node_table. */
	/* The copying is important.  Since these keys might be removed later, */
	/*   it needs to be possible to call skit_free on each individual node. */
	/*   Having some, but not all, of the table's nodes be allocated in the */
	/*   same chunk of memory, defeats that purpose.  Calling skit_free on */
	/*   that same chunk repeated and at different offsets is not advised. */
	/*   That's why we do away with the contiguous chunk at this point.    */
	/* This does mean that lookups need to do an extra indirection, but    */
	/*   the extra indirection is a table-lookup that should offset the    */
	/*   cost of having to find an index into an unsorted node array.      */
	for ( i = 0; i < node->nodes_len; i++ )
	{
		sASSERT(node->chars[i] != new_char);
		skit_trie_node *new_node = skit_malloc(sizeof(skit_trie_node));
		memcpy(new_node, &node->nodes[i], sizeof(skit_trie_node));
		new_node_array[node->chars[i]] = new_node;
	}

	/* Free the original chunk of nodes. */
	skit_free(node->nodes);

	/* Replace it with the new node array/table. */
	node->node_table = new_node_array;

	/* Do the value insertion. */
	/* skit_trie_finish_tail ensures that the rest of the key gets accounted for. */
	node->node_table[new_char] =
		skit_trie_finish_tail(skit_trie_node_new(), coords, key_ptr, key_len, value);

	/* Update length metadata. */
	node->chars_len = 0xFF;
	(node->nodes_len)++;

}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_multi_node(
	skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value)
{
	skit_trie_node *node = coords.node;
	sASSERT(node->nodes_len > 1);
	sASSERT(node->nodes_len == node->chars_len);
	sASSERT(node->nodes_len < SKIT__TRIE_NODE_PREALLOC); /* This does NOT handle the convert-to-table case. */

	/*
	New pair is {"cxyz",value}
	Before
	.         ,-> 'a' -> cnode0 -> ...
	. node - {
	.         `-> 'b' -> cnode1 -> ...
	After
	.
	.         ,-> 'a' -> cnode0 -> ...
	.        |
	. node - + -> 'b' -> cnode1 -> ...
	.        |
	.         `-> 'c' -> new_node -> "xyz" ... -> new_cnode -> value
	*/

	(node->nodes_len)++;
	node->nodes = skit_realloc( node->nodes, sizeof(skit_trie_node) * node->nodes_len);

	(node->chars_len)++;
	node->chars[node->chars_len - 1] = key_ptr[coords.pos];
	skit_trie_node *new_node = &node->nodes[node->nodes_len - 1];
	skit_trie_finish_tail(new_node, coords, key_ptr, key_len, value);
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_linear_node(
	skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value)
{
	/* Linear node that has >1 character. */
	skit_trie_node *node = coords.node;
	sASSERT(node->nodes_len == 1);
	sASSERT(node->chars_len > 1);

	if ( coords.n_chars_into_node == 0 )
	{
		/* No common prefix: split it into three nodes. */
		/* Illustration: (new pair is {'xyz',value})
		Before:
		.
		. node -> "abc" -> cnode0 -> ...
		.
		After
		.         ,-> 'a' -> node0 -> "bc" -> cnode0 -> ...
		. node - {
		.         `-> 'x' -> node1 -> "yz" ... -> cnode1 -> value
		.
		*/

		/* Avoid copying and temporary allocations. */
		/* We allow cnode0 to remain where it is, and just create a new */
		/*   block of memory for the new array of nodes. */
		skit_trie_node *cnode0 = &node->nodes[0];

		node->nodes = skit_malloc(2 * sizeof(skit_trie_node));
		skit_trie_node *node0 = &node->nodes[0];
		skit_trie_node *node1 = &node->nodes[1];

		/* Populate node0. */
		const char *node0_str = (const char*)node->chars + 1;
		size_t      node0_len = node->chars_len - 1;
		skit_trie_node_insert_non0_str(node0, cnode0, node0_str, node0_len);

		/* Populate node1. */
		skit_trie_finish_tail(node1, coords, key_ptr, key_len, value);

		/* Setup up the new char-to-node mapping table. */
		/* Do this AFTER chars has been copied into other nodes. */
		/* node->chars[0] = node->chars[0]; (already handled.) */
		node->chars[1] = key_ptr[coords.pos];

		/* Meta-data update.  This should also be done later due to the */
		/* possibility of overwriting values before they get used in the */
		/* previous calculations. */
		node->chars_len = 2;
		node->nodes_len = 2;

		/* Optimizations. */
		skit_trie_node_fold(node0);
		skit_trie_node_fold(node1);
	}
	else
	{
		/* Common prefix: split it into four nodes. */
		/* Illustration: (new pair is {'abyqp',value2})
		Before:
		.
		. node -> "abxdb" -> cnode0 -> ...
		.
		After
		.                          ,-> 'x' -> node1 -> "db" -> cnode0 -> ...
		. node -> "ab" -> node0 - {
		.                          `-> 'y' -> node2 -> "qp" ... -> cnode1 -> value
		.
		To accomplish this, we first turn it into this:
		.
		. node -> "ab" -> node0 -> "xdb" -> cnode0 -> ...
		.
		And then recurse on node0 with new coordinates.
		The new coordinates will be such that the next operation falls
		into the (coords.n_chars_into_node == 0) case and gets finished
		that way.
		*/
		skit_trie_node *node0 = skit_trie_node_new();
		void *dst_start = node0->chars;
		void *src_start = node->chars + coords.n_chars_into_node;
		size_t copy_len = node->chars_len - coords.n_chars_into_node;
		memcpy( dst_start, src_start, copy_len );
		node0->chars_len = copy_len;

		/* Transfer the tail (cnode0 in the drawing) to the new node. */
		node0->nodes = node->nodes;
		node0->nodes_len = 1;

		/* Transfer the new node (node0) to the original node. */
		node->nodes = node0;
		sASSERT(node->nodes_len == 1);
		node->chars_len = coords.n_chars_into_node;

		/* Calculate the coordinates for the next split. */
		skit_trie_coords new_coords = coords;
		new_coords.node = node0;
		new_coords.pos += node->chars_len;
		new_coords.n_chars_into_node = 0;

		/* Note that (node0->chars_len == 1) is possible. */
		/* This means that if we called skit_trie_split_linear_node, then */
		/*   we could easily violate the assertion that (node->chars_len > 1) */
		/* Because of that danger, we call the more generic */
		/*   skit_trie_split_node to force a reclassification of node0. */
		skit_trie_split_node(new_coords, key_ptr, key_len, value);
	}
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_node(
	skit_trie_coords coords,
	const char *key_ptr,
	size_t key_len,
	const void *value)
{
	skit_trie_node *node = coords.node;

	if ( node->nodes_len == 1 && node->chars_len > 1 )
		skit_trie_split_linear_node(coords, key_ptr, key_len, value);
	else if  ( node->nodes_len < SKIT__TRIE_NODE_PREALLOC )
		skit_trie_split_multi_node(coords, key_ptr, key_len, value);
	else if ( node->nodes_len == SKIT__TRIE_NODE_PREALLOC )
		skit_trie_split_multi_to_table(coords, key_ptr, key_len, value);
	else
		skit_trie_split_table_node(coords, key_ptr, key_len, value);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_set( skit_trie *trie, const skit_slice key, const void *value, const char *flags )
{
	SKIT_USE_FEATURE_EMULATION;
	size_t key_len = sSLENGTH(key);
	const char *key_ptr = (const char*)sSPTR(key);
	
	if ( trie->iterator_count > 0 )
		sTHROW(SKIT_TRIE_WRITE_IN_ITERATION,
			"Call to skit_trie_set during iteration. #iters = %d, key = \"%.*s\", flags = \"%s\"",
			trie->iterator_count, key_len, key_ptr, flags);

	skit_trie_flags tflags = skit_parse_flags( flags );
	sASSERT_MSG(tflags & ICASE, "Case insensitive operations are currently unsupported.");

	if ( !(tflags & (CREATE | OVERWRITE)) )
		sTHROW(SKIT_TRIE_BAD_FLAGS,
			"Call to skit_trie_set without providing 'c' or 'o' flags. key = \"%.*s\", flags = \"%s\"",
			key_len, key_ptr, flags);

	if ( key_len > sLLENGTH(trie->key_return_buf) )
		trie->key_return_buf = *skit_loaf_resize(&trie->key_return_buf, key_len);

	/* First insertion. */
	/* Check this first to avoid calling skit_trie_find in such cases. */
	if ( trie->root == NULL )
	{
		if ( !(tflags & CREATE) )
			sTHROW(SKIT_EXCEPTION,
				"Wrote a value to an empty trie. Key is \"%.*s\". 'c' (create) not passed in flags.",
				key_len, key_ptr);

		trie->root = skit_trie_node_new();
		skit_trie_coords zero_coords;
		skit_trie_coords_ctor(&zero_coords);
		skit_trie_finish_tail(trie->root, zero_coords, key_ptr, key_len, value);
		
		memcpy( sLPTR(trie->key_return_buf), key_ptr, key_len );
		return skit_slice_of(trie->key_return_buf.as_slice, 0, key_len);
	}

	/* Insertions/replacements for already-existing keys. */
	skit_trie_coords coords = skit_trie_find(trie, key_ptr, key_len);

	if ( skit_exact_match(coords, key_len) )
	{
		if ( !(tflags & OVERWRITE) )
			sTHROW(SKIT_EXCEPTION,
				"Wrote a value to an already existing key \"%.*s\". 'o' (overwrite) not passed in flags.",
				key_len, key_ptr);

		/* Exact match is the easier case and involves no allocations. */
		/* Just replace an existing value with the new one. */
		coords.node->value = value;
	}
	else if ( skit_prefix_match(coords, key_len) || skit_no_match(coords, key_len) )
	{
		if ( !(tflags & CREATE) )
			sTHROW(SKIT_EXCEPTION,
				"Wrote a value to a non-existant key \"%.*s\". 'c' (create) not passed in flags.",
				key_len, key_ptr);

		/* The first part of key_return_buf will be filled by the call to skit_trie_find. */
		/* The second part is something we need to fill.  We're inserting the */
		/*   rest of the key, so we know that it will end up entered exactly */
		/*   the way the caller supplied it. */
		void       *dst_start = sLPTR(trie->key_return_buf) + coords.pos;
		const void *src_start = key_ptr + coords.pos;
		size_t       copy_len = key_len - coords.pos;
		memcpy(dst_start, src_start, copy_len);

		/* Split a node to insert the new value. */
		skit_trie_split_node(coords, key_ptr, key_len, value);
	}
	else
		sTHROW(SKIT_FATAL_EXCEPTION, "Impossible result for trie lookup on key \"%.*s\".", key_len, sSPTR(key));

	return skit_slice_of(trie->key_return_buf.as_slice, 0, key_len);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_setc( skit_trie *trie, const char *key, const void *value, const char *flags )
{
	return skit_trie_set( trie, skit_slice_of_cstr(key), value, flags );
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_remove( skit_trie *trie, const skit_slice key, const char *flags )
{
	SKIT_USE_FEATURE_EMULATION;
	size_t key_len = sSLENGTH(key);
	const char *key_ptr = (const char*)sSPTR(key);
	
	if ( trie->iterator_count > 0 )
		sTHROW(SKIT_TRIE_WRITE_IN_ITERATION,
			"Call to skit_trie_remove during iteration. #iters = %d, key = \"%.*s\", flags = \"%s\"",
			trie->iterator_count, key_len, key_ptr, flags);

	printf("%s, %d: Stub!  Not implemented.\n", __func__, __LINE__);
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

size_t skit_trie_len( const skit_trie *trie )
{
	return trie->length;
}

/* ------------------------------------------------------------------------- */

static size_t skit_trie_count_descendants( skit_trie_node *node )
{
	if ( node->nodes_len == 0 )
		return 0;

	size_t i = 0;
	size_t result = node->nodes_len;
	if ( node->nodes_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		for(; i < node->nodes_len; i++ )
			result += skit_trie_count_descendants(&node->nodes[i]);
	}
	else
	{
		for(; i < 256; i++ )
			if ( node->node_table[i] != NULL )
				result += skit_trie_count_descendants(node->node_table[i]);
	}

	return result;
}

void skit_trie_dump( const skit_trie *trie, skit_stream *output )
{
	skit_stream_appendln(output, sSLICE("skit_trie_dump not implemented.") );
}

/* ------------------------------------------------------------------------- */
/* ------------------------- Iteration Logic ------------------------------- */
/* ------------------------------------------------------------------------- */

typedef struct skit_trie_frame skit_trie_frame;
struct skit_trie_frame
{
	skit_trie_coords coords;
	uint16_t current_char;
};

struct skit_trie_iter
{
	skit_trie *trie;
	skit_trie_frame *frames;
	ssize_t current_frame;
	ssize_t prev_frame;
	char *key_buffer;
};

/* ------------------------------------------------------------------------- */

static void skit_trie_iter_push( skit_trie_iter *iter, skit_trie_node *node, size_t pos )
{
	skit_trie_frame *frame = &iter->frames[iter->current_frame + 1];
	
	/* initialize and populate the new frame */
	skit_trie_coords_ctor(&frame->coords);
	frame->coords.node = node;
	frame->coords.pos = pos;
	
	/* update the key_buffer as needed. */
	if ( iter->current_frame >= 0 )
	{
		skit_trie_frame *prev_frame = &iter->frames[iter->current_frame];
		if ( prev_frame->coords.pos < pos )
		{
			void *dst_start = iter->key_buffer + prev_frame->coords.pos;
			void *src_start = prev_frame->coords.node->chars;
			size_t copy_len = pos - prev_frame->coords.pos;
			memcpy(dst_start, src_start, copy_len);
		}
	}
	
	/* commit the results */
	iter->prev_frame = iter->current_frame;
	(iter->current_frame)++;
}

static void skit_trie_iter_pop( skit_trie_iter *iter )
{
	iter->prev_frame = iter->current_frame;
	(iter->current_frame)--;
}

/* ------------------------------------------------------------------------- */

skit_trie_iter *skit_trie_iter_new( skit_trie *trie, const skit_slice prefix, const char *flags )
{
	SKIT_USE_FEATURE_EMULATION;
	skit_trie_flags tflags = skit_parse_flags( flags );
	sASSERT_MSG(tflags & ICASE, "Case insensitive operations are currently unsupported.");
	
	size_t key_len = sSLENGTH(prefix);
	const char *key_ptr = (const char*)sSPTR(prefix);

	if ( tflags & (CREATE | OVERWRITE) )
		sTHROW(SKIT_TRIE_BAD_FLAGS,
			"Call to skit_trie_iter_new with 'c' or 'o' flags. These are invalid for iteration. prefix = \"%.\", flags = \"%s\"",
			key_len, key_ptr, flags);
	
	size_t longest_key_len = sLLENGTH(trie->key_return_buf);
	
	skit_trie_iter *result = skit_malloc(sizeof(skit_trie_iter));
	result->trie = trie;
	result->frames = NULL;
	result->current_frame = -1;
	result->prev_frame = -1;
	result->key_buffer = skit_malloc(longest_key_len);
	
	(trie->iterator_count)++;
	
	skit_trie_coords coords = skit_trie_find(trie, key_ptr, key_len);
	if ( skit_no_match(coords, key_len) )
	{
		/* Leave result->current_frame as -1 so that skit_trie_iter_next returns 0 immediately. */
		return result;
	}
	
	result->frames = skit_malloc(sizeof(skit_trie_frame) * ((longest_key_len - coords.pos) + 1));

	if ( coords.n_chars_into_node > 0 )
	{
		/* skit_trie_find should only return coords.n_chars_into_node > 0  */
		/*   for linear nodes that the prefix doesn't completely encompass. */
		/* The iterator can't handle that, so we'll skip ahead to the next */
		/*   complete node in those cases. */
		skit_trie_node *node = coords.node;
		sASSERT_EQ(node->nodes_len, 1, "%d");
		sASSERT_GT(node->chars_len, 1, "%d"); /* chars_len > 1 */

		/* Copy the rest of the node into the key_buffer. */
		void *dst_start = result->key_buffer + coords.pos;
		void *src_start = node->chars + coords.n_chars_into_node;
		size_t copy_len = node->chars_len - coords.n_chars_into_node;
		memcpy(dst_start, src_start, copy_len);
		
		/* Update coords to point to the next node. */
		coords.pos += copy_len;
		coords.n_chars_into_node = 0;
		coords.node = &node->nodes[0];
	}
	
	skit_trie_iter_push( result, coords.node, coords.pos );
	
	return result;
}

/* ------------------------------------------------------------------------- */

skit_trie_iter *skit_trie_iter_free( skit_trie_iter *iter )
{
	SKIT_USE_FEATURE_EMULATION;

	/* frames can't be used for this check because it may be NULL for empty sets. */
	if ( iter->key_buffer == NULL )
		sTHROW(SKIT_TRIE_EXCEPTION, "Attempt to free an already-freed skit_trie_iter.");
	
	if ( iter->frames != NULL )
	{
		skit_free(iter->frames);
		iter->frames = NULL;
	}

	skit_free(iter->key_buffer);
	iter->key_buffer = NULL;
	
	(iter->trie->iterator_count)--;
	
	skit_free(iter);
	return NULL;
}

/* ------------------------------------------------------------------------- */

int skit_trie_iter_next( skit_trie_iter *iter, skit_slice *key, void **value )
{
	/* Global termination case: we just popped the last frame. */
	/*   That means we left the first node we ever visited, and there are no */
	/*   more children to descend into: iteration is over. */
	if ( iter->current_frame < 0 )
	{
		*key = skit_slice_null();
		*value = NULL;
		return 0;
	}

	skit_trie_frame *frame = &iter->frames[iter->current_frame];
	skit_trie_coords *coords = &frame->coords;
	skit_trie_node *node = coords->node;
	
	/* It seems our iterator is alive.  Let's iterate! */
	
	/* Entry case: Catch the case where we just pushed. */
	/* This means we just enetered the node for the first time. */
	if ( iter->prev_frame < iter->current_frame )
	{
		/* Check for a value, yield it if one is found. */
		if ( node->have_value )
		{
			/* Update this so that we don't loop forever. */
			iter->prev_frame = iter->current_frame;
			
			/* Yield the value. */
			*key = skit_slice_of_cstrn(iter->key_buffer, coords->pos);
			*value = (void*)node->value;
			return 1;
		}
	}
	
	/* Exit case: We just exhausted the node and must do a pop. */
	if ( frame->current_char > 255 )
	{
		skit_trie_iter_pop(iter);
		
		/* Refresh function-scope variables by recursing. */
		return skit_trie_iter_next(iter, key, value);
	}
	
	/* Iteration case: visit the next child node. */
	if ( node->nodes_len == 1 && node->chars_len > 1 )
	{
		/* Linear nodes. */
		
		/* Mark this node as exhausted and then push the child into focus. */
		frame->current_char = 256;
		skit_trie_iter_push(iter, &node->nodes[0], coords->pos + node->chars_len);
		
		/* Continue searching for a value. */
		return skit_trie_iter_next(iter, key, value);
	}
	else if ( node->nodes_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		/* Multi-nodes. */
		size_t i;
		ssize_t node_index = -1;
		uint16_t next_char = 256;
		sASSERT_EQ(node->nodes_len, node->chars_len, "%d");

		/* Find the index of the node that corresponds to current_char */
		/*   and also find the character that comes after it, alphabetically. */
		for ( i = 0; i < node->chars_len; i++ )
		{
			uint16_t c = node->chars[i];
		
			if ( c == frame->current_char )
				node_index = i;
			
			if ( frame->current_char < c && c < next_char )
				next_char = c;
		}
		
		/* Advance the iteration. */
		frame->current_char = next_char;
		
		/* When entering a node for the first time, current_char will be 0. */
		/* There will seldom be a '\0' character in a trie node, and so we */
		/*   definitely can't count on finding one.  In the likely case that */
		/*   we don't, we'll have to skip ahead to the lowest character that */
		/*   has a corresponding node and descend into that one instead. */
		/*   We do this by calling ourselves and thus recalculating with */
		/*   the new current_char set.  Iteration should proceed normally */
		/*   after that. */
		if ( node_index < 0 )
			return skit_trie_iter_next(iter, key, value);
		
		/* Normal case: there is a node_index for us to descend into. */
		skit_trie_iter_push(iter, &node->nodes[node_index], coords->pos + 1);
		return skit_trie_iter_next(iter, key, value);
	}
	else
	{
		/* Table nodes. */
		size_t i;
		ssize_t node_index = frame->current_char;
		uint16_t next_char = 256;
		
		/* Scan for the next node that we can descend into. */
		for ( i = node_index + 1; i < 256; i++ )
		{
			if ( node->node_table[i] != NULL )
			{
				next_char = i;
				break;
			}
		}
		
		/* Advance the iteration. */
		frame->current_char = next_char;
		
		/* When entering a node for the first time, current_char will be 0. */
		/* There will seldom be a '\0' character in a trie node, and so we */
		/*   definitely can't count on finding one.  In the likely case that */
		/*   we don't, we'll have to skip ahead to the lowest character that */
		/*   has a corresponding node and descend into that one instead. */
		/*   We do this by calling ourselves and thus recalculating with */
		/*   the new current_char set.  Iteration should proceed normally */
		/*   after that. */
		if ( node->node_table[node_index] == NULL )
			return skit_trie_iter_next(iter, key, value);
		
		/* Normal case: there is a node_index for us to descend into. */
		skit_trie_iter_push(iter, node->node_table[node_index], coords->pos + 1);
		return skit_trie_iter_next(iter, key, value);
	}
	
	sASSERT(0);
	return 0;
}

/* ------------------------------------------------------------------------- */

void skit_trie_unittest()
{
	void *val;
	skit_trie *trie = skit_trie_new();
	skit_trie_setc(trie, "abcde", (void*)1, "c");
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, ""), sSLICE("abcde")); sASSERT_EQ((size_t)val, 1, "%d");

	skit_trie_setc(trie, "abxyz", (void*)2, "c");
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, ""), sSLICE("abcde")); sASSERT_EQ((size_t)val, 1, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abxyz", &val, ""), sSLICE("abxyz")); sASSERT_EQ((size_t)val, 2, "%d");

	skit_trie_setc(trie, "a1234", (void*)3, "c");
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, ""), sSLICE("abcde")); sASSERT_EQ((size_t)val, 1, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abxyz", &val, ""), sSLICE("abxyz")); sASSERT_EQ((size_t)val, 2, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "a1234", &val, ""), sSLICE("a1234")); sASSERT_EQ((size_t)val, 3, "%d");

	skit_trie_setc(trie, "abcd!", (void*)4, "c");
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, ""), sSLICE("abcde")); sASSERT_EQ((size_t)val, 1, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abxyz", &val, ""), sSLICE("abxyz")); sASSERT_EQ((size_t)val, 2, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "a1234", &val, ""), sSLICE("a1234")); sASSERT_EQ((size_t)val, 3, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abcd!", &val, ""), sSLICE("abcd!")); sASSERT_EQ((size_t)val, 4, "%d");

	skit_trie_setc(trie, "abcxy", (void*)5, "c");
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, ""), sSLICE("abcde")); sASSERT_EQ((size_t)val, 1, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abxyz", &val, ""), sSLICE("abxyz")); sASSERT_EQ((size_t)val, 2, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "a1234", &val, ""), sSLICE("a1234")); sASSERT_EQ((size_t)val, 3, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abcd!", &val, ""), sSLICE("abcd!")); sASSERT_EQ((size_t)val, 4, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abcxy", &val, ""), sSLICE("abcxy")); sASSERT_EQ((size_t)val, 5, "%d");
	
	skit_trie_free(trie);
}