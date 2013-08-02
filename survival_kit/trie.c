
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
#include "survival_kit/math.h"

#include "survival_kit/streams/pfile_stream.h"

#define SKIT_DO_FIND_DEBUG 0
#if SKIT_DO_FIND_DEBUG != 0
#define FIND_DEBUG(...) printf(__VA_ARGS__)
#else
#define FIND_DEBUG(...) ((void)0)
#endif

#define SKIT_DO_SPLIT_DEBUG 0
#if SKIT_DO_SPLIT_DEBUG != 0
#define SPLIT_DEBUG(...) printf(__VA_ARGS__)
#else
#define SPLIT_DEBUG(...) ((void)0)
#endif

#define SKIT_DO_ENTRY_DEBUG 0
#if SKIT_DO_ENTRY_DEBUG != 0
#define ENTRY_DEBUG(...) printf(__VA_ARGS__)
#else
#define ENTRY_DEBUG(...) ((void)0)
#endif

#define SKIT_DO_ITER_DEBUG 0
#if SKIT_DO_ITER_DEBUG != 0
#define ITER_DEBUG(...) printf(__VA_ARGS__)
#else
#define ITER_DEBUG(...) ((void)0)
#endif

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
	ICASE     = SKIT_FLAG_I,
	CREATE    = SKIT_FLAG_C,
	OVERWRITE = SKIT_FLAG_O

} skit_trie_flags;

/* ------------------------------------------------------------------------- */

static void skit_trie_enforce_valid_flags(skit_flags flags_given, skit_flags flags_allowed)
{
	skit_flags bad_flags = flags_given & ~flags_allowed;
	if ( !bad_flags )
	{
		char flag_buf1[SKIT_FLAGS_BUF_SIZE];
		char flag_buf2[SKIT_FLAGS_BUF_SIZE];
		skit_flags_to_str(flags_given, flag_buf1);
		skit_flags_to_str(bad_flags, flag_buf2);
		sENFORCE_MSGF( !bad_flags, "Unexpected flags passed into skit_trie: \"%s\".  Flags given: \"%s\"\n", bad_flags, flags_given);
	}
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

static void skit_trie_coords_dump( const skit_trie_coords *coords, skit_stream *output )
{
	skit_stream_appendf( output, "coords(%p)\n", coords );
	if ( coords == NULL )
		return;
	
	skit_stream_appendf( output, "  node == %p\n", coords->node );
	skit_stream_appendf( output, "  pos == %d\n", coords->pos );
	skit_stream_appendf( output, "  n_chars_into_node == %d\n", coords->n_chars_into_node );
	skit_stream_appendf( output, "  lookup_stopped == %d\n", coords->lookup_stopped );
}

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
	return ( coords.pos == key_len && coords.node != NULL && coords.node->have_value );
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

static void skit_trie_accumulate_key(skit_trie *trie, size_t pos, const uint8_t *key_frag, size_t frag_len)
{
	/* printf("skit_trie_accumulate_key(trie, %d, \"%.*s\")\n", pos, (int)frag_len, key_frag); */
	uint8_t *key_return_buf = sLPTR(trie->key_return_buf);
	
	/* printf("  -> '%.*s' ~ '%.*s'\n", pos, key_return_buf, (int)frag_len, key_frag); */
	
	void       *dst_start = key_return_buf + pos;
	const void *src_start = key_frag;
	memcpy(dst_start, src_start, frag_len);
}

/* ------------------------------------------------------------------------- */

static skit_trie_coords skit_trie_next_node(
	skit_trie *trie,
	skit_trie_node *current,
	const uint8_t *key,
	size_t key_len,
	size_t pos,
	int case_sensitive)
{
	FIND_DEBUG("skit_trie_next_node(trie, \"%.*s\", %ld)\n", (int)key_len, key, pos);

	if ( current->nodes_len == 0 )
	{
		/*
		This happens if the key is has one of the trie's keys as its
		prefix.  It will cause the next_node algorithm to wander into
		a value node that has no child nodes.
		*/
		return skit_trie_stop_lookup(current, pos, 0);
	}
	else if ( current->nodes_len == 1 )
	{
		/*
		Narrow radix search: there is only one possible node to
		end up at.  The string to match to get there is the one
		contained in the 'chars' member and is no longer than
		'chars_len'
		*/
		FIND_DEBUG("%s, %d: Narrow node. node->chars == \"%.*s\"\n", __func__, __LINE__, (int)current->chars_len, (char*)current->chars);
		size_t chars_len = current->chars_len;

		size_t start_pos = pos;
		size_t i = 0;
		while ( i < chars_len )
		{
			if ( pos >= key_len ||
				0 != skit_char_ascii_ccmp(key[pos], current->chars[i], case_sensitive) )
			{
				skit_trie_accumulate_key(trie, start_pos, (const uint8_t*)current->chars, i);
				return skit_trie_stop_lookup(current, pos, i);
			}

			pos++;
			i++;
		}

		skit_trie_accumulate_key(trie, start_pos, (const uint8_t*)current->chars, current->chars_len);
		return skit_trie_continue_lookup(&current->nodes.array[0], pos);
	}
	else if ( current->chars_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		/*
		There is more than one choice, but few enough that we can
		fit them in a small amount of space.
		We scan the 'chars' array; when an element in that matches the
		current char in our key, then that index is the index to use
		in the 'nodes' array.
		*/
		FIND_DEBUG("%s, %d: Multi node. node->chars == \"%.*s\"\n", __func__, __LINE__, (int)current->chars_len, (char*)current->chars);
		sASSERT( current->chars_len == current->nodes_len );

		if ( pos >= key_len )
			return skit_trie_stop_lookup(current, pos, 0);

		size_t chars_len = current->chars_len;
		size_t i = 0;
		for ( ; i < chars_len; i++ )
			if ( 0 == skit_char_ascii_ccmp(current->chars[i], key[pos], case_sensitive) )
				break;

		if ( i == chars_len )
			return skit_trie_stop_lookup(current, pos, 0);

		skit_trie_accumulate_key(trie, pos, &current->chars[i], 1);
		return skit_trie_continue_lookup(&current->nodes.array[i], pos+1);
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
		FIND_DEBUG("%s, %d: Table node. node->nodes_len == %d\n", __func__, __LINE__, current->nodes_len);
		if ( pos >= key_len )
			return skit_trie_stop_lookup(current, pos, 0);

		skit_trie_node *next_node = NULL;
		if ( case_sensitive )
			next_node = current->nodes.table[key[pos]];
		else
		{
			next_node = current->nodes.table[skit_char_ascii_to_upper(key[pos])];
			if ( next_node == NULL )
				next_node = current->nodes.table[skit_char_ascii_to_lower(key[pos])];
		}
		
		if ( next_node == NULL )
			return skit_trie_stop_lookup(current, pos, 0);

		skit_trie_accumulate_key(trie, pos, &key[pos], 1);
		return skit_trie_continue_lookup(next_node, pos+1);
	}

	sASSERT(0);
}

/* ------------------------------------------------------------------------- */

static skit_trie_coords skit_trie_find(
	skit_trie *trie,
	const uint8_t *key_ptr,
	size_t key_len,
	int case_sensitive)
{
	FIND_DEBUG("skit_trie_find(trie, \"%.*s\")\n", (int)key_len, key_ptr);
	sASSERT(key_ptr != NULL);

	FIND_DEBUG("%s, %d: trie->root == %p\n", __func__, __LINE__, trie->root);
	if ( trie->root == NULL )
		return skit_trie_stop_lookup(NULL, 0, 0);

	skit_trie_coords coords;
	skit_trie_coords prev;
	
	skit_trie_coords_ctor(&coords);
	skit_trie_coords_ctor(&prev);
	
	coords.node = trie->root;
	
	while ( 1 )
	{
		prev = coords;
		coords = skit_trie_next_node(trie, coords.node, key_ptr, key_len, coords.pos, case_sensitive);

		FIND_DEBUG("%s, %d: \n", __func__, __LINE__);
		
		if ( coords.lookup_stopped )
			break;

		if ( skit_exact_match(coords, key_len) )
			return coords; /* Match success. DONE. */
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
			skit_trie_node_dtor(&node->nodes.array[i]);
		
		skit_free(node->nodes.array);
	}
	else
	{
		for(; i < 256; i++ )
		{
			if ( node->nodes.table[i] != NULL )
			{
				skit_trie_node_dtor(node->nodes.table[i]);
				skit_free(node->nodes.table[i]);
			}
		}
		
		skit_free(node->nodes.table);
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

skit_slice skit_trie_get( skit_trie *trie, const skit_slice key, void **value, skit_flags flags )
{
	const uint8_t *key_ptr = sSPTR(key);
	sENFORCE_MSG(value != NULL, "NULL value pointer given in call to skit_trie_set.  "
		"This must be a pointer to a variable that can accept the returned value.");
	sENFORCE_MSG(key_ptr != NULL, "NULL key given in call to skit_trie_set.");

	skit_trie_enforce_valid_flags(flags, CREATE | OVERWRITE | ICASE);
	
	ENTRY_DEBUG("%s, %d: \n", __func__, __LINE__);

	size_t key_len = sSLENGTH(key);
	skit_trie_coords coords = skit_trie_find(trie, key_ptr, key_len, (flags & ICASE) ? 0 : 1);

	if ( skit_exact_match(coords, key_len) )
	{
		/* Cast away constness because we are handing this back to the caller. */
		/* We only guarantee that WE won't alter it. */
		*value = (void*)coords.node->value;
		return skit_slice_of(trie->key_return_buf.as_slice, 0, key_len);
	}

	*value = NULL;
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_getc( skit_trie *trie, const char *key, void **value, skit_flags flags )
{
	return skit_trie_get(trie, skit_slice_of_cstr(key), value, flags);
}

/* ------------------------------------------------------------------------- */

static void skit_trie_node_ctor( skit_trie_node *node )
{
	node->nodes.array = NULL;
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

(If you look at the output of skit_trie_dump after the tests in the linear_node
unittest, you might notice that this function might not actually need to be
implemented.  Some emergent behavior in the node splitting code seems to 
inherently handle this optimization.  The calls to it are left around as hints
for where it should be used, should it become useful.)
*/
static skit_trie_node *skit_trie_node_fold(skit_trie_node *node)
{
	/* printf("%s, %d: Stub!\n", __func__, __LINE__); */
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
	const uint8_t *str_ptr,
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
	node->nodes.array = next_node;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_node_append_str(
	skit_trie_node *node,
	skit_trie_node **new_tail,
	const uint8_t *str_ptr,
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
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value)
{
	skit_trie_node *new_value_node;

	skit_trie_node_ctor(given_start_node);

	const uint8_t *new_node_str = key_ptr + (coords.pos + 1);
	size_t         new_node_len = key_len - (coords.pos + 1);
	skit_trie_node_append_str(given_start_node, &new_value_node, new_node_str, new_node_len);
	skit_trie_node_set_value(new_value_node, value);
	return given_start_node;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_node(
	skit_trie_coords coords,
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value);

/* ------------------------------------------------------------------------- */

static void skit_trie_split_table_node(
	skit_trie_coords coords,
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value)
{
	SPLIT_DEBUG("%s, %d: \n", __func__, __LINE__);
	/* Easy case: just add a new entry in the table. */
	skit_trie_node *node = coords.node;
	sASSERT(node->nodes_len > SKIT__TRIE_NODE_PREALLOC);
	uint8_t new_char = key_ptr[coords.pos];

	/* skit_trie_finish_tail ensures that the rest of the key gets accounted for. */
	node->nodes.table[new_char] =
		skit_trie_finish_tail(skit_trie_node_new(), coords, key_ptr, key_len, value);

	(node->nodes_len)++;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_multi_to_table(
	skit_trie_coords coords,
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value)
{
	SPLIT_DEBUG("%s, %d: \n", __func__, __LINE__);
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
	/*   own blocks of memory, which are then indexed by node->nodes.table. */
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
		memcpy(new_node, &node->nodes.array[i], sizeof(skit_trie_node));
		new_node_array[node->chars[i]] = new_node;
	}

	/* Free the original chunk of nodes. */
	skit_free(node->nodes.array);

	/* Replace it with the new node array/table. */
	node->nodes.table = new_node_array;

	/* Do the value insertion. */
	/* skit_trie_finish_tail ensures that the rest of the key gets accounted for. */
	node->nodes.table[new_char] =
		skit_trie_finish_tail(skit_trie_node_new(), coords, key_ptr, key_len, value);

	/* Update length metadata. */
	node->chars_len = 0xFF;
	(node->nodes_len)++;

}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_multi_node(
	skit_trie_coords coords,
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value)
{
	SPLIT_DEBUG("%s, %d: \n", __func__, __LINE__);
	skit_trie_node *node = coords.node;
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
	node->nodes.array = skit_realloc( node->nodes.array, sizeof(skit_trie_node) * node->nodes_len);

	(node->chars_len)++;
	node->chars[node->chars_len - 1] = key_ptr[coords.pos];
	skit_trie_node *new_node = &node->nodes.array[node->nodes_len - 1];
	skit_trie_finish_tail(new_node, coords, key_ptr, key_len, value);
}

/* ------------------------------------------------------------------------- */

static void skit_trie_split_linear_node(
	skit_trie_coords coords,
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value)
{
	SPLIT_DEBUG("%s, %d: \n", __func__, __LINE__);
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
		skit_trie_node *cnode0 = &node->nodes.array[0];

		node->nodes.array = skit_malloc(2 * sizeof(skit_trie_node));
		skit_trie_node *node0 = &node->nodes.array[0];
		skit_trie_node *node1 = &node->nodes.array[1];

		/* Populate node0. */
		const uint8_t *node0_str = (const uint8_t*)node->chars + 1;
		size_t         node0_len = node->chars_len - 1;
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
		node0->nodes.array = node->nodes.array;
		node0->nodes_len = 1;

		/* Transfer the new node (node0) to the original node. */
		node->nodes.array = node0;
		sASSERT(node->nodes_len == 1);
		node->chars_len = coords.n_chars_into_node;

		/* Calculate the coordinates for the next split. */
		skit_trie_coords new_coords = coords;
		new_coords.node = node0;
		new_coords.pos  = coords.pos;
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
	const uint8_t *key_ptr,
	size_t key_len,
	const void *value)
{
	SPLIT_DEBUG("%s, %d: \n", __func__, __LINE__);
	skit_trie_node *node = coords.node;

	if ( coords.n_chars_into_node == 0 && coords.pos == key_len )
		skit_trie_node_set_value(node, value);
	else if ( node->nodes_len == 1 && node->chars_len > 1 )
		skit_trie_split_linear_node(coords, key_ptr, key_len, value);
	else if  ( node->nodes_len < SKIT__TRIE_NODE_PREALLOC )
		skit_trie_split_multi_node(coords, key_ptr, key_len, value);
	else if ( node->nodes_len == SKIT__TRIE_NODE_PREALLOC )
		skit_trie_split_multi_to_table(coords, key_ptr, key_len, value);
	else
		skit_trie_split_table_node(coords, key_ptr, key_len, value);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_set( skit_trie *trie, const skit_slice key, const void *value, skit_flags flags )
{
	SKIT_USE_FEATURE_EMULATION;
	size_t key_len = sSLENGTH(key);
	const uint8_t *key_ptr = (const uint8_t*)sSPTR(key);
	ENTRY_DEBUG("skit_trie_set(trie, \"%.*s\", %p, %x)\n", (int)key_len, key_ptr, value, flags);
	
	sENFORCE_MSG(trie != NULL, "NULL trie given in call to skit_trie_set.");
	sENFORCE_MSG(key_ptr != NULL, "NULL key given in call to skit_trie_set.");
	
	if ( trie->iterator_count > 0 )
	{
		char flags_str[SKIT_FLAGS_BUF_SIZE];
		skit_flags_to_str(flags, flags_str);
		sTHROW(SKIT_TRIE_WRITE_IN_ITERATION,
			"Call to skit_trie_set during iteration. #iters = %d, key = \"%.*s\", flags = \"%s\"",
			trie->iterator_count, key_len, key_ptr, flags_str);
	}

	skit_trie_enforce_valid_flags(flags, CREATE | OVERWRITE | ICASE);
	
	if ( !(flags & (CREATE | OVERWRITE)) )
	{
		char flags_str[SKIT_FLAGS_BUF_SIZE];
		skit_flags_to_str(flags, flags_str);
		sTHROW(SKIT_TRIE_BAD_FLAGS,
			"Call to skit_trie_set without providing 'c' or 'o' flags. key = \"%.*s\", flags = \"%s\"",
			key_len, key_ptr, flags_str);
	}

	if ( key_len > sLLENGTH(trie->key_return_buf) )
		trie->key_return_buf = *skit_loaf_resize(&trie->key_return_buf, key_len);

	/* First insertion. */
	/* Check this first to avoid calling skit_trie_find in such cases. */
	if ( trie->root == NULL )
	{
		if ( !(flags & CREATE) )
			sTHROW(SKIT_EXCEPTION,
				"Wrote a value to an empty trie. Key is \"%.*s\". 'c' (create) not passed in flags.",
				key_len, key_ptr);

		ENTRY_DEBUG("%s, %d: new root.\n", __func__, __LINE__);
		trie->root = skit_trie_node_new();
		skit_trie_node *end_node;
		skit_trie_node_append_str(trie->root, &end_node, key_ptr, key_len);
		skit_trie_node_set_value(end_node, value);
		
		memcpy( sLPTR(trie->key_return_buf), key_ptr, key_len );
		
		(trie->length)++;
		
		return skit_slice_of(trie->key_return_buf.as_slice, 0, key_len);
	}

	/* Insertions/replacements for already-existing keys. */
	skit_trie_coords coords = skit_trie_find(trie, key_ptr, key_len, (flags & ICASE) ? 0 : 1);

	if ( skit_exact_match(coords, key_len) )
	{
		if ( !(flags & OVERWRITE) )
			sTHROW(SKIT_EXCEPTION,
				"Wrote a value to an already existing key \"%.*s\". 'o' (overwrite) not passed in flags.",
				key_len, key_ptr);

		/* Exact match is the easier case and involves no allocations. */
		/* Just replace an existing value with the new one. */
		coords.node->value = value;
	}
	else if ( skit_prefix_match(coords, key_len) || skit_no_match(coords, key_len) )
	{
		if ( !(flags & CREATE) )
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
		
		/* Update this. */
		(trie->length)++;
	}
	else
		sTHROW(SKIT_FATAL_EXCEPTION, "Impossible result for trie lookup on key \"%.*s\".", key_len, sSPTR(key));

	return skit_slice_of(trie->key_return_buf.as_slice, 0, key_len);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_setc( skit_trie *trie, const char *key, const void *value, skit_flags flags )
{
	return skit_trie_set( trie, skit_slice_of_cstr(key), value, flags );
}

/* ------------------------------------------------------------------------- */

skit_slice skit_trie_remove( skit_trie *trie, const skit_slice key, skit_flags flags )
{
	SKIT_USE_FEATURE_EMULATION;
	size_t key_len = sSLENGTH(key);
	const uint8_t *key_ptr = (const uint8_t*)sSPTR(key);
	
	if ( trie->iterator_count > 0 )
	{
		char flags_str[SKIT_FLAGS_BUF_SIZE];
		skit_flags_to_str(flags, flags_str);
		sTHROW(SKIT_TRIE_WRITE_IN_ITERATION,
			"Call to skit_trie_remove during iteration. #iters = %d, key = \"%.*s\", flags = \"%s\"",
			trie->iterator_count, key_len, key_ptr, flags_str);
	}

	printf("%s, %d: Stub!  Not implemented.\n", __func__, __LINE__);
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

size_t skit_trie_len( const skit_trie *trie )
{
	return trie->length;
}

/* ------------------------------------------------------------------------- */

static int32_t skit_trie_count_leaves( const skit_trie_node *node )
{
	if ( node == NULL || node->nodes_len == 0 )
		return 1;

	size_t i = 0;
	size_t result = 0;
	if ( node->nodes_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		for(; i < node->nodes_len; i++ )
			result += skit_trie_count_leaves(&node->nodes.array[i]);
	}
	else
	{
		for(; i < 256; i++ )
			if ( node->nodes.table[i] != NULL )
				result += skit_trie_count_leaves(node->nodes.table[i]);
	}

	return result;
}

typedef struct skit_trie_drawing skit_trie_drawing;
struct skit_trie_drawing
{
	int32_t width;
	int32_t height;
	char *buffer;
};

typedef struct skit_trie_draw_cursor skit_trie_draw_cursor;
struct skit_trie_draw_cursor
{
	int32_t x;
	int32_t y;
};

static char *skit_trie_cell_addr( skit_trie_drawing *dst, int32_t x, int32_t y )
{
	return &dst->buffer[x + (y * dst->width)];
}

static void skit_trie_ndraw( skit_trie_drawing *dst, skit_trie_draw_cursor *cursor, const char *str_ptr, int32_t str_len )
{
	int32_t rclip = cursor->x + str_len;
	if ( rclip > dst->width )
		str_len -= (dst->width - rclip);
	
	sASSERT_GE(cursor->x, 0, "%d");
	
	memcpy(skit_trie_cell_addr(dst, cursor->x, cursor->y), str_ptr, str_len);
	(cursor->x) += str_len;
}

#define skit_trie_draw( dst, cursor, str ) \
	(skit_trie_ndraw( (dst), (cursor), (str), sizeof(str)-1 ))

static void skit_trie_draw_value( skit_trie_drawing *dst, skit_trie_draw_cursor *cursor, const void *value )
{
	char spbuf[22];
	
	/* Use spbuf to avoid writing a '\0' to the drawing. */
	int n_chars = snprintf( spbuf, sizeof(spbuf), "(%p)", value );
	skit_trie_ndraw( dst, cursor, spbuf, n_chars );
}

static void skit_trie_draw_char( skit_trie_drawing *dst, skit_trie_draw_cursor *cursor, uint8_t c )
{
	if ( c == '\\' )
		skit_trie_draw( dst, cursor, "\\\\" );
	else if ( 0x20 <= c && c < 0x7F ) /* Printable chars between space (inclusive) and delete (exclusive) */
		skit_trie_ndraw( dst, cursor, (const char*)&c, 1 );
	else
	{
		/* Non-printables, ex: the null byte becomes \x00 */
		char spbuf[5];
		
		/* Use spbuf to avoid writing a '\0' to the drawing. */
		int n_chars = snprintf( spbuf, sizeof(spbuf), "\\x%.2X", c );
		skit_trie_ndraw( dst, cursor, spbuf, n_chars );
	}
}

static void skit_trie_draw_vert_branch(
	skit_trie_drawing *dst,
	int32_t x,
	int32_t y1,
	int32_t y2,
	const char *distinction,
	int32_t vert_branch_top,
	int32_t vert_branch_bottom)
{
	skit_trie_draw_cursor cursor;
	size_t i;
	
	cursor.x = x;
	cursor.y = (y1 + y2) / 2;
	skit_trie_draw( dst, &cursor, "-" );
	
	int32_t distinction_len = strlen(distinction);
	if ( distinction_len > 0 )
		skit_trie_ndraw( dst, &cursor, distinction, distinction_len );
	
	int32_t vert_branch_x = cursor.x;
	cursor.x = vert_branch_x;
	cursor.y = vert_branch_top;
	skit_trie_draw( dst, &cursor, "," );
	
	for ( i = vert_branch_top+1; i < vert_branch_bottom; i++ )
	{
		cursor.x = vert_branch_x;
		cursor.y = i;
		skit_trie_draw( dst, &cursor, "|" );
	}
	
	cursor.x = vert_branch_x;
	cursor.y = vert_branch_bottom;
	skit_trie_draw( dst, &cursor, "`" );
}
	

static void skit_trie_draw_node( skit_trie_drawing *dst, const skit_trie_node *node, int32_t x, int32_t y1, int32_t y2 );

static void skit_trie_draw_horiz_branch(
	skit_trie_drawing *dst,
	const skit_trie_node *node, 
	const uint8_t c,
	int32_t x,
	int32_t *cy1,
	int32_t *cy2,
	int32_t *vert_branch_top,
	int32_t *vert_branch_bottom)
{
	skit_trie_draw_cursor cursor;
	
	/* This child's lower bound is the previous child's upper bound. */
	*cy1 = *cy2;
	int32_t n_leaves = skit_trie_count_leaves(node) * 2 - 1;
	*cy2 += n_leaves;
	
	cursor.x = x;
	cursor.y = (*cy1 + *cy2) / 2;
	
	/* Draw the branch, followed by the next node. */
	skit_trie_draw( dst, &cursor, "-> '" );
	skit_trie_draw_char( dst, &cursor, c );
	skit_trie_draw( dst, &cursor, "' " );
	
	skit_trie_draw_node( dst, node, cursor.x, *cy1, *cy2 );

	/* Record the sizing info for the vertical branch. */
	*vert_branch_top    = SKIT_MIN( *vert_branch_top, cursor.y );
	*vert_branch_bottom = SKIT_MAX( *vert_branch_bottom, cursor.y );
	
	/* This inserts a space between the branches. */
	(*cy2)++;
}

static void skit_trie_draw_node( skit_trie_drawing *dst, const skit_trie_node *node, int32_t x, int32_t y1, int32_t y2 )
{
	skit_trie_draw_cursor cursor;
	
	if ( node == NULL )
	{
		/* Completely empty tree. */
		cursor.x = x;
		cursor.y = (y2 + y1) / 2;
		
		skit_trie_draw( dst, &cursor, "-> {null node}\n" );
	}
	else if ( node->nodes_len == 0 )
	{
		/* Leaf nodes. */
		cursor.x = x;
		cursor.y = (y2 + y1) / 2;
		
		/*skit_trie_draw( dst, &cursor, "-> " );*/
		
		if ( node->have_value )
			skit_trie_draw_value( dst, &cursor, node->value );
		else
			skit_trie_draw( dst, &cursor, "{empty leaf}");
		
		/*skit_trie_draw( dst, &cursor, "\n" );*/
	}
	else if ( node->nodes_len == 1 )
	{
		/* Linear nodes. (and multi-nodes with 1 child) */
		cursor.x = x;
		cursor.y = (y2 + y1) / 2;
		
		if ( node->have_value )
		{
			skit_trie_draw_value( dst, &cursor, node->value );
			skit_trie_draw( dst, &cursor, " " );
		}
		
		skit_trie_draw( dst, &cursor, "-> " );
		
		const char *delimiter;
		if ( node->chars_len > 1 )
			delimiter = "\"";
		else
			delimiter = "'";
		
		size_t i;
		skit_trie_ndraw( dst, &cursor, delimiter, 1 );
		for ( i = 0; i < node->chars_len; i++ )
			skit_trie_draw_char( dst, &cursor, node->chars[i] );
		skit_trie_ndraw( dst, &cursor, delimiter, 1 );
		skit_trie_draw( dst, &cursor, " " );
		
		skit_trie_draw_node( dst, &node->nodes.array[0], cursor.x, y1, y2 );
	}
	else if ( node->nodes_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		/* Multi-nodes. */
		size_t i;
		int32_t cy1 = y1;
		int32_t cy2 = y1;
		int32_t vert_branch_top = dst->height;
		int32_t vert_branch_bottom = 0;
		
		cursor.x = x;
		cursor.y = (y2 + y1) / 2;
		if ( node->have_value )
		{
			skit_trie_draw_value( dst, &cursor, node->value );
			skit_trie_draw( dst, &cursor, " " );
		}
		
		for( i = 0; i < node->nodes_len; i++ )
		{
			/* x+2 makes room for the vertical branch
			.                      ,  
			.                     -|
			.                      `
			. construct that will be filled in later. */
			skit_trie_draw_horiz_branch(
				dst, &node->nodes.array[i], node->chars[i], cursor.x+2, 
				&cy1, &cy2, &vert_branch_top, &vert_branch_bottom);
		}
		
		skit_trie_draw_vert_branch(
			dst, cursor.x, y1, y2, "", vert_branch_top, vert_branch_bottom);
	}
	else
	{
		/* Table nodes. */
		size_t i;
		int32_t cy1 = y1;
		int32_t cy2 = y1;
		int32_t vert_branch_top = dst->height;
		int32_t vert_branch_bottom = 0;
		
		cursor.x = x;
		cursor.y = (y2 + y1) / 2;
		if ( node->have_value )
		{
			skit_trie_draw_value( dst, &cursor, node->value );
			skit_trie_draw( dst, &cursor, " " );
		}
		
		for( i = 0; i < 256; i++ )
		{
			if ( node->nodes.table[i] == NULL )
				continue;
			
			/* x+4 makes room for the vertical branch
			.                      ,  
			.                   -[]|
			.                      `
			. construct that will be filled in later. */
			skit_trie_draw_horiz_branch(
				dst, node->nodes.table[i], i, cursor.x+4,
				&cy1, &cy2, &vert_branch_top, &vert_branch_bottom);
		}
		
		skit_trie_draw_vert_branch(
			dst, cursor.x, y1, y2, "[]", vert_branch_top, vert_branch_bottom);
	}
	
}

static void skit_trie_draw_tree( const skit_trie *trie, skit_stream *output )
{
	/*
	A (slightly incorrect) example trie drawing:
	.
	.           ,-> 'x' -> (0xF00Df00dF00Df00d)
	.           |
	. -> 'a' -[]|-> '\x00' -> "h@x" -> (0xF00Df00dF00Df00d)
	.           |
	.           `-> 'z' -> (0xF00Df00dF00Df00d) "qasdf" -> (0xF00Df00dF00Df00d)
	.
	This example is used to give an intuitive grasp of sizing concerns on a
	per-node basis.  There are some elements to consider:
	- Single letter branches take up to 6 characters just to draw the character. (ex: '\x00')
	- Multi-letter branches will be more space efficient than single letter branches.
	- Filler between nodes: " -[]|-> " which is about 8 characters max.
	    (The [] is used to indicate that the node is a table lookup.  The
		above drawing is incorrect because a table lookup would never be used
		for a mere 3-node wide split.  A normal multi-node would draw " -|-> ")
	- Up to 16+2+2+1 = 21 characters for hex dumps of void* values.
		- 16 for the hex digits.
		- 2 for the ()
		- 2 for the 0x
		- 1 for the extra space needed.
	
	Armed with these numbers, we can calculate a theoretical maximum number of
	screen width used for each node:
		6+8+21 = 35 characters.
	
	For total width, add 1 to the sum to make room for newlines.
	
	If the total width by that calculation is low, then it may be because
	of an empty trie.  In that case, we want at least enough characters to
	draw the "-> {null node}\n" used to represent empty tries.  We'll give
	ourselves some extra room and say that we must always have 64 bytes.
	
	The height will be the number of leaves, multiplied by 2 to leave spacing
	inbetween them for readability.  Minus 1 because the last line doesn't
	need spacing after it (we will print a newline later to separate it from
	adjacent characters in the stream).
	*/
	
	skit_trie_drawing drawing;
	int32_t x, y;
	size_t longest_key_len = sLLENGTH(trie->key_return_buf);
	int32_t n_leaves = skit_trie_count_leaves(trie->root);
	drawing.width  = SKIT_MAX(longest_key_len*35 + 1, 64);
	drawing.height = n_leaves*2 - 1;
	drawing.buffer = skit_malloc(drawing.width * drawing.height);
	memset(drawing.buffer, ' ', drawing.width*drawing.height); /* Fill it with spaces. */
	
	/* Do drawing with a recursive traversal. */
	skit_trie_draw_node(&drawing, trie->root, 0, 0, drawing.height);
	
	/* Send the finished rendering to the stream. */
	/* This pass will strip out all of the extra spaces that follow the */
	/*   ends of lines (and the \n character itself: let the stream worry */
	/*   about converting that). */
	for ( y = 0; y < drawing.height; y++ )
	{
		for ( x = drawing.width - 1; x >= 0; x-- )
		{
			if ( drawing.buffer[x + (y*drawing.width)] != ' ' )
			{
				skit_stream_appendln(output, skit_slice_of_cstrn(&drawing.buffer[y*drawing.width], x+1));
				break;
			}
		}
	}
	
	/* Cleanup. */
	skit_free(drawing.buffer);
}

void skit_trie_dump( const skit_trie *trie, skit_stream *output )
{
	skit_stream_appendln(output, sSLICE("skit_trie with the following tree:"));
	skit_stream_appendln(output, sSLICE(""));
	skit_trie_draw_tree( trie, output );
	skit_stream_appendln(output, sSLICE(""));
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
	uint8_t *key_buffer;
};

/* ------------------------------------------------------------------------- */

static void skit_iter_accumulate_key(skit_trie_iter *iter, size_t pos, const uint8_t *key_frag, size_t frag_len)
{
	/* printf("skit_trie_accumulate_key(iter, %d, \"%.*s\")\n", pos, (int)frag_len, key_frag); */
	uint8_t *key_return_buf = iter->key_buffer;
	
	/* printf("  -> '%.*s' ~ '%.*s'\n", pos, key_return_buf, (int)frag_len, key_frag); */
	
	void       *dst_start = key_return_buf + pos;
	const void *src_start = key_frag;
	memcpy(dst_start, src_start, frag_len);
}

/* ------------------------------------------------------------------------- */

static void skit_trie_iter_push( skit_trie_iter *iter, skit_trie_node *node, size_t pos )
{
	skit_trie_frame *frame = &iter->frames[iter->current_frame + 1];
	
	/* initialize and populate the new frame */
	skit_trie_coords_ctor(&frame->coords);
	frame->coords.node = node;
	frame->coords.pos = pos;
	frame->current_char = 0;
	
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

skit_trie_iter *skit_trie_iter_new( skit_trie *trie, const skit_slice prefix, skit_flags flags )
{
	SKIT_USE_FEATURE_EMULATION;
	
	/*
	Case-insensitive iteration is potentially tricky because skit_trie_find
	would need to be able to return multiple nodes that the iterator could
	walk over to cover all possible prefixes that case-insensitively match.
	Ex:
	trie = trie of { "ABCD", "abcd", "aBcX", "AbCx" }
	iter = skit_trie_iter_new( trie, "abc", sFLAGS("i") );
	This iterator should return all keys in the above trie.  Using the
	current implementation of skit_trie_find, it would probably only return
	"ABCD" because "ABC" is the first node that skit_trie_find would find.
	*/
	sENFORCE_MSG( !(flags & ICASE), "Case insensitive operations are currently unimplemented for skit_trie_iter.");
	
	ITER_DEBUG("%s, %d: \n", __func__, __LINE__);
	skit_trie_enforce_valid_flags(flags, CREATE | OVERWRITE | ICASE);
	size_t key_len = sSLENGTH(prefix);
	const uint8_t *key_ptr = (const uint8_t*)sSPTR(prefix);

	if ( flags & (CREATE | OVERWRITE) )
	{
		char flags_str[SKIT_FLAGS_BUF_SIZE];
		skit_flags_to_str(flags, flags_str);
		sTHROW(SKIT_TRIE_BAD_FLAGS,
			"Call to skit_trie_iter_new with 'c' or 'o' flags. These are invalid for iteration. prefix = \"%.\", flags = \"%s\"",
			key_len, key_ptr, flags_str);
	}
	
	size_t longest_key_len = sLLENGTH(trie->key_return_buf);
	
	skit_trie_iter *result = skit_malloc(sizeof(skit_trie_iter));
	result->trie = trie;
	result->frames = NULL;
	result->current_frame = -1;
	result->prev_frame = -1;
	result->key_buffer = skit_malloc(longest_key_len);
	
	(trie->iterator_count)++;
	
	skit_trie_coords coords = skit_trie_find(trie, key_ptr, key_len, 1);
	if ( skit_no_match(coords, key_len) )
	{
		/* Leave result->current_frame as -1 so that skit_trie_iter_next returns 0 immediately. */
		return result;
	}
	
	/* skit_trie_find accumulates the found key into trie->key_return_buf. */
	/* We actually need it in iter->key_buffer. */
	/* This call should fix that. */
	skit_iter_accumulate_key(result, 0, sLPTR(trie->key_return_buf), coords.pos);
	
	/* */
	result->frames = skit_malloc(sizeof(skit_trie_frame) * ((longest_key_len - coords.pos) + 1));

	/* skit_trie_find should only return coords.n_chars_into_node > 0  */
	/*   for linear nodes that the prefix doesn't completely encompass. */
	/* The iterator can't handle that, so we'll skip ahead to the next */
	/*   complete node in those cases. */
	if ( coords.n_chars_into_node > 0 )
	{
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
		coords.node = &node->nodes.array[0];
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
	ITER_DEBUG("skit_trie_iter_next(...): %d\n", __LINE__);
	ITER_DEBUG("%s, %d: iter->current_frame == %d\n", __func__, __LINE__, iter->current_frame);
	sENFORCE_MSG(value != NULL, "NULL value pointer given in call to skit_trie_iter_next.  "
		"This must be a pointer to a variable that can accept the returned value.");
	sENFORCE_MSG(key != NULL, "NULL key pointer given in call to skit_trie_iter_next.  "
		"This must be a pointer to a skit_slice that can accept the returned key.");
	
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

	ITER_DEBUG("%s, %d: current key: %.*s\n", __func__, __LINE__, coords->pos, iter->key_buffer);
	
	/* It seems our iterator is alive.  Let's iterate! */
	
	/* Entry case: Catch the case where we just pushed. */
	/* This means we just enetered the node for the first time. */
	if ( iter->prev_frame < iter->current_frame )
	{
		ITER_DEBUG("%s, %d: Entry case.\n", __func__, __LINE__);
		/* Check for a value, yield it if one is found. */
		if ( node->have_value )
		{
			ITER_DEBUG("%s, %d: Have value.\n", __func__, __LINE__);
			/* Update this so that we don't loop forever. */
			iter->prev_frame = iter->current_frame;
			
			/* Yield the value. */
			*key = skit_slice_of_cstrn((char*)iter->key_buffer, coords->pos);
			*value = (void*)node->value;
			return 1;
		}
	}
	
	/* Exit case: We just exhausted the node and must do a pop. */
	if ( frame->current_char > 255 )
	{
		ITER_DEBUG("%s, %d: Exiting.\n", __func__, __LINE__);
		skit_trie_iter_pop(iter);
		
		/* Refresh function-scope variables by recursing. */
		return skit_trie_iter_next(iter, key, value);
	}
	
	/* Iteration case: visit the next child node. */
	if ( node->nodes_len == 1 && node->chars_len > 1 )
	{
		ITER_DEBUG("%s, %d: Linear node.\n", __func__, __LINE__);
		/* Linear nodes. */
		
		/* Mark this node as exhausted and then push the child into focus. */
		frame->current_char = 256;
		skit_trie_iter_push(iter, &node->nodes.array[0], coords->pos + node->chars_len);
		skit_iter_accumulate_key(iter, coords->pos, node->chars, node->chars_len);
		
		/* Continue searching for a value. */
		return skit_trie_iter_next(iter, key, value);
	}
	else if ( node->nodes_len <= SKIT__TRIE_NODE_PREALLOC )
	{
		ITER_DEBUG("%s, %d: Multi-node.\n", __func__, __LINE__);
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
		skit_trie_iter_push(iter, &node->nodes.array[node_index], coords->pos + 1);
		skit_iter_accumulate_key(iter, coords->pos, &node->chars[node_index], 1);
		
		return skit_trie_iter_next(iter, key, value);
	}
	else
	{
		ITER_DEBUG("%s, %d: Table node.\n", __func__, __LINE__);
		/* Table nodes. */
		size_t i;
		ssize_t node_index = frame->current_char;
		uint16_t next_char = 256;
		
		/* Scan for the next node that we can descend into. */
		for ( i = node_index + 1; i < 256; i++ )
		{
			if ( node->nodes.table[i] != NULL )
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
		if ( node->nodes.table[node_index] == NULL )
			return skit_trie_iter_next(iter, key, value);
		
		/* Normal case: there is a node_index for us to descend into. */
		skit_trie_iter_push(iter, node->nodes.table[node_index], coords->pos + 1);
		
		uint8_t c = node_index;
		skit_iter_accumulate_key(iter, coords->pos, &c, 1);
		
		return skit_trie_iter_next(iter, key, value);
	}
	
	sASSERT(0);
	return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ testing! --------------------------------- */


typedef struct skit_trie_test skit_trie_test;
struct skit_trie_test
{
	skit_slice slice;
	skit_slice islice;
	size_t val;
};

static void skit_trie_test_ctor(
	skit_trie_test *test,
	skit_slice slice,
	skit_slice islice,
	size_t val)
{
	test->slice = slice;
	test->islice = islice;
	test->val = val;
}

/* ------------------------------------------------------------------------- */

static void skit_trie_exhaustive_get_test(
	skit_trie *trie,
	const skit_trie_test* tests,
	size_t trie_size,
	size_t n_slices,
	int test_vals )
{
	void *val;
	size_t i;
	
	sASSERT_EQ(skit_trie_len(trie), trie_size, "%d");
	
	/* Test for false negatives. */
	for ( i = 0; i < trie_size; i++ )
	{
		sASSERT_EQS(skit_trie_get(trie, tests[i].slice, &val, SKIT_FLAGS_NONE), tests[i].slice);
		if ( test_vals )
			sASSERT_EQ((size_t)val, tests[i].val, "%d");
		
		sASSERT_IEQS(skit_trie_get(trie, tests[i].islice, &val, SKIT_FLAG_I), tests[i].slice );
		/* sASSERT_EQ((size_t)val, tests[i].val, "%d"); */
	}
	
	/* Test against false positives. */
	for ( i = trie_size; i < n_slices; i++ )
	{
		sASSERT_EQS(skit_trie_get(trie, tests[i].slice, &val, SKIT_FLAGS_NONE), skit_slice_null());
		sASSERT_EQ(val, NULL, "%d");
	}
}

static int skit_trie_cmp_test( const void *a, const void *b )
{
	skit_trie_test *t1 = (skit_trie_test*)a;
	skit_trie_test *t2 = (skit_trie_test*)b;
	return skit_slice_ascii_cmp( t1->slice, t2->slice );
}

static void skit_trie_iter_test(
	skit_trie *trie,
	skit_trie_test *tests, /* Must be sorted. */
	size_t n_tests,
	size_t pos,
	int recurse )
{
	skit_slice prefix = skit_slice_of(tests[0].slice, 0, pos);
	
	ITER_DEBUG("\n\n");
	ITER_DEBUG("skit_trie_iter_test(trie, tests[%ld], %ld, %d), prefix == '%.*s'\n",
		n_tests, pos, recurse, sSLENGTH(prefix), sSPTR(prefix));
	
	skit_trie_iter *iter = skit_trie_iter_new(trie, prefix, SKIT_FLAGS_NONE);
	
	skit_slice key;
	void *val;
	size_t i = 0;
	while ( skit_trie_iter_next( iter, &key, &val ) )
	{
		ITER_DEBUG("%s, %d: calling skit_trie_iter_next(iter, \"%.*s\", %ld); i == %d; expect %ld\n",
			__func__, __LINE__,
			sSLENGTH(key), sSPTR(key),
			(size_t)val, i, tests[i].val);
		
		sASSERT_EQS( tests[i].slice, key );
		sASSERT_EQ ( tests[i].val, (size_t)val, "%d" );
		i++;
	}
	
	sASSERT_EQ( i, n_tests, "%d" );
	
	skit_trie_iter_free(iter);
	
	if ( recurse && n_tests > 1 )
	{
		uint8_t *key_ptr = sSPTR(tests[0].slice);
		size_t   key_len = sSLENGTH(tests[0].slice);
		uint16_t prev_char = 0x800;
		uint16_t this_char = (pos == key_len ? 0x800 : key_ptr[pos]);
		size_t prev_index = 0;
		for ( i = 1; i < n_tests; i++ )
		{
			key_ptr = sSPTR(tests[i].slice);
			key_len = sSLENGTH(tests[i].slice);
			
			prev_char = this_char;
			
			sASSERT( pos <= key_len ); /* We should reach n_tests before falling off any cliffs. */
			if ( pos == key_len )
				this_char = 0x800; /* Cases where we iterate on exact matches. */
			else
				this_char = key_ptr[pos]; /* Prefix stuff. */
			
			if ( this_char != prev_char && i > prev_index )
			{
				if ( prev_char < 0x800 )
					skit_trie_iter_test( trie, &tests[prev_index], i - prev_index, pos+1, 1 );
				prev_index = i;
			}
		}

		/* Pick up anything left-over. */
		skit_trie_iter_test( trie, &tests[prev_index], n_tests - prev_index, pos+1, 1 );
	}
}

static void skit_trie_exhaustive_test( skit_trie_test* tests, size_t n_tests, int harder )
{
	SKIT_USE_FEATURE_EMULATION;
	size_t i;
	skit_trie *trie = skit_trie_new();
	
	/* ----- Get/set ----- */
	for ( i = 0; i < n_tests; i++ )
	{
		/*printf("Going to set with %p, %ld\n", sSPTR(tests[i]), sSLENGTH(tests[i]));*/
		sASSERT_EQS(skit_trie_set(trie, tests[i].slice, (void*)tests[i].val, sFLAGS("c")), tests[i].slice);
		
		/*
		if ( harder )
			skit_trie_dump(trie, skit_stream_stdout);
		*/

		if ( harder )
			sTRACE4(skit_trie_exhaustive_get_test(trie, tests, i+1, n_tests, 1));
	}
	
	#if 0
	if ( !harder )
	{
		skit_trie_dump(trie, skit_stream_stdout);
		/*sASSERT_MSG(0, "Here is your tree, sir.");*/
	}
	#endif
	
	if ( !harder )
		sTRACE0(skit_trie_exhaustive_get_test(trie, tests, n_tests, n_tests, 1));
	
	/* ----- Iteration test ----- */
	qsort( tests, n_tests, sizeof(skit_trie_test), &skit_trie_cmp_test );
	
	int recurse = 0;
	if ( harder )
		recurse = 1;

	ITER_DEBUG("\n\n-------- Iteration test begins. ---------\n\n\n");
	sTRACE0(skit_trie_iter_test( trie, tests, n_tests, 0, recurse ));
	
	/* ----- Case sensitivity ----- */
	/* It should be possible to do overwriting assignments into all of the
	keys in a case-insensitive manner without altering the trie.
	We test this by inserting all of the test islice's and then checking
	the length and running the get test again.  
	If these insertions grow the trie, then the length assertion will 
	catch it. 
	If these insertions alter the keys then the get_test will catch it.
	This test should be done last, because it may alter the values
	corresponding to various keys in unpredictable ways. */
	size_t length_before = skit_trie_len(trie);
	for ( i = 0; i < n_tests; i++ )
	{
		sASSERT_IEQS(skit_trie_set(trie, tests[i].islice, (void*)tests[i].val, sFLAGS("io")), tests[i].slice);
		
		/*
		if ( harder )
			skit_trie_dump(trie, skit_stream_stdout);
		*/
	}
	
	sASSERT_EQ( length_before, skit_trie_len(trie), "%d" );
	sTRACE0(skit_trie_exhaustive_get_test(trie, tests, n_tests, n_tests, 0));
	
	sTRACE0(skit_trie_free(trie));
}

static void skit_trie_unittest_basics()
{
	SKIT_USE_FEATURE_EMULATION;
	size_t n_tests = 10;
	skit_trie_test *tests = skit_malloc(sizeof(skit_trie_test) * n_tests);
	
	/* Successively shorter keys.  Also special characters. */
	skit_trie_test_ctor(&tests[0], sSLICE("abcde"        ), sSLICE("Abcde"        ), 1  );
	skit_trie_test_ctor(&tests[1], sSLICE("abxyz"        ), sSLICE("abXYZ"        ), 2  );
	skit_trie_test_ctor(&tests[2], sSLICE("a1234"        ), sSLICE("A1234"        ), 3  );
	skit_trie_test_ctor(&tests[3], sSLICE("abcd!"        ), sSLICE("aBcD!"        ), 4  );
	skit_trie_test_ctor(&tests[4], sSLICE("abcxy"        ), sSLICE("abCxy"        ), 5  );
	skit_trie_test_ctor(&tests[5], sSLICE("abc"          ), sSLICE("ABC"          ), 6  );
	skit_trie_test_ctor(&tests[6], sSLICE("\0\xFF\0\xFF" ), sSLICE("\0\xFF\0\xFF" ), 7  );
	skit_trie_test_ctor(&tests[7], sSLICE("\0\xFF\x7F"   ), sSLICE("\0\xFF\x7F"   ), 8  );
	skit_trie_test_ctor(&tests[8], sSLICE("\0\x7F\x7F"   ), sSLICE("\0\x7F\x7F"   ), 9 );
	skit_trie_test_ctor(&tests[9], sSLICE(""             ), sSLICE(""             ), 10 );

	sTRACE0(skit_trie_exhaustive_test(tests, n_tests, 1));
	
	void *val;
	skit_trie *trie;

	/* Test inserting successively longer keys. */
	/* (The previous mix tests successively shorter keys.) */
	trie = skit_trie_new();
	sASSERT_EQS(skit_trie_setc(trie, "abc", (void*)1, sFLAGS("c")), sSLICE("abc"));
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, SKIT_FLAGS_NONE), skit_slice_null());
	sASSERT_EQ(val, NULL, "%d");
	sASSERT_EQS(skit_trie_setc(trie, "abcde", (void*)1, sFLAGS("c")), sSLICE("abcde"));
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, SKIT_FLAGS_NONE), sSLICE("abcde"));
	sASSERT_EQ((size_t)val, 1, "%d");
	sTRACE0(skit_trie_free(trie));
	
	trie = skit_trie_new();
	sASSERT_EQS(skit_trie_setc(trie, "", (void*)1, sFLAGS("c")), sSLICE(""));
	sASSERT_EQS(skit_trie_getc(trie, "", &val, SKIT_FLAGS_NONE), sSLICE(""));
	sTRACE0(skit_trie_free(trie));
	
	/* Bug found in the wild: retrieving from a zero-length string caused a segfault.  It should return a null-slice instead. */
	trie = skit_trie_new();
	sASSERT_EQS(skit_trie_getc(trie, "", &val, SKIT_FLAGS_NONE), skit_slice_null());
	sTRACE0(skit_trie_free(trie));
	
	skit_free(tests);
	
	printf("  skit_trie_unittest_basics passed.\n");
}

static void skit_trie_unittest_linear_nodes()
{
	SKIT_USE_FEATURE_EMULATION;
	skit_trie_test *tests = skit_malloc(sizeof(skit_trie_test) * 3);
	size_t i;
	
	#define teststr_len  (SKIT__TRIE_NODE_PREALLOC*3 + 2)
	#define teststr_mid1 (SKIT__TRIE_NODE_PREALLOC*2 - 4)
	#define teststr_mid2 (SKIT__TRIE_NODE_PREALLOC*2 - 2)
	
	/* This is not a requirement of the trie, but it IS an assumption of the unittest. */
	/* It mostly affects the choice of teststr_mid1 and teststr_mid2, */
	/*   which were chosen to leave at least 2 characters between the */
	/*   splitting that they cause. */
	sASSERT_GE(SKIT__TRIE_NODE_PREALLOC, 6, "%d");
	
	/* "abcdefghijklmnopqrstuvwxyzabcdefg..." */
	char teststr0[teststr_len+1];
	tests[0].slice = skit_slice_of_cstrn(teststr0, teststr_len);
	for ( i = 0; i < teststr_len; i++ )
		teststr0[i] = 'a' + (i % 26);
	teststr0[teststr_len] = '\0';
	
	/* "abcdefghijklmnopqRSTUVWXYZABCDEFG..." */
	char teststr1[teststr_len+1];
	tests[1].slice = skit_slice_of_cstrn(teststr1, teststr_len);
	memcpy(teststr1, teststr0, teststr_len);
	for ( i = teststr_mid1; i < teststr_len; i++ )
		teststr1[i] = skit_char_ascii_to_upper(teststr1[i]);
	teststr1[teststr_len] = '\0';
	
	/* "abcdefghijklmnopqrs" */
	char teststr2[teststr_mid2+1];
	tests[2].slice = skit_slice_of_cstrn(teststr2, teststr_mid2);
	memcpy(teststr2, teststr0, teststr_mid2);
	teststr2[teststr_mid2] = '\0';
	
	tests[0].islice = tests[1].slice;
	tests[1].islice = tests[0].slice;
	tests[2].islice = tests[2].slice; /* TODO: this test could be better. */
	
	tests[0].val = 1;
	tests[1].val = 2;
	tests[2].val = 3;
	
	sTRACE0(skit_trie_exhaustive_test(tests, 3, 1));
	
	sTRACE0(skit_free(tests));
	
	printf("  skit_trie_unittest_linear_nodes passed.\n");
}

/* If don't already know what a trigram is and you want to know, then I suggest
lookup up ngram or n-gram rather than trigram.  As a search term, "trigram" is
likely to bring up the Chinese bagua, which is quite a different entity. 

Long and short: trigrams are sequences of 3-characters, usually the first three
of words in a corpus of text, and usually used to index that text for fast 
lookup of contained words and such.
*/
static void skit_trie_test_make_trigram(
	skit_loaf *loaf,
	size_t *loaf_index,
	size_t max_index,
	const skit_slice parent,
	int breadth,
	int depth)
{
	size_t i;
	
	/*
	printf("%s, %d: skit_trigram(%p, %ld, %ld, \"%.*s\", %d, %d)\n", __func__, __LINE__,
		loaf, *loaf_index, max_index, (int)sSLENGTH(parent), sSPTR(parent), breadth, depth);
	*/
	sASSERT_LE( *loaf_index, max_index, "%d" );
	
	if ( depth > 3 )
		return;
	
	if ( depth == 0 )
	{
		skit_loaf current = skit_loaf_new();
		loaf[*loaf_index] = current;
		(*loaf_index)++;
		skit_trie_test_make_trigram(loaf, loaf_index, max_index, current.as_slice, breadth, depth+1);
		return;
	}
	
	int frequency = 256 / SKIT__TRIE_NODE_PREALLOC;
	
	for ( i = 0; i < breadth; i++ )
	{
		skit_loaf current = skit_loaf_alloc(depth);
		loaf[*loaf_index] = current;
		skit_loaf_assign_slice(&current, parent);
		uint8_t *ptr = sLPTR(current);
		
		if ( i == 0 )
			ptr[depth-1] = 0x00;
		else if ( i == breadth-1 ) /* Always ensure that the highest possible branch value gets tested. */
			ptr[depth-1] = 0xFF;
		else 
			ptr[depth-1] = (i*frequency) + depth;
		/*ptr[depth-1] = i + 'a';*/
		
		(*loaf_index)++;
		skit_trie_test_make_trigram(loaf, loaf_index, max_index, current.as_slice, breadth, depth+1);
	}
}

static void skit_trie_unittest_table_nodes()
{
	SKIT_USE_FEATURE_EMULATION;
	size_t i, j;
	size_t breadth = SKIT__TRIE_NODE_PREALLOC+1;
	
	/* Create a trie of trigrams (and their subsets). */
	/* Make each level slightly fatter than the prealloc number. */
	size_t n_strings = 
		skit_ipow(breadth, 3)+
		skit_ipow(breadth, 2)+
		skit_ipow(breadth, 1)+1;
	
	skit_loaf      *loaves  = skit_malloc(sizeof(skit_loaf) * n_strings);
	skit_loaf      *iloaves = skit_malloc(sizeof(skit_loaf) * n_strings);
	skit_trie_test *tests   = skit_malloc(sizeof(skit_trie_test) * n_strings);
	size_t loaf_index = 0;
	skit_trie_test_make_trigram( loaves, &loaf_index, n_strings, sSLICE(""), breadth, 0 );
	
	for ( i = 0; i < n_strings; i++ )
	{
		uint8_t *lptr = sLPTR(loaves[i]);
		ssize_t len = sLLENGTH(loaves[i]);
		
		iloaves[i] = skit_loaf_alloc(len);
		uint8_t *ilptr = sLPTR(iloaves[i]);
		
		/* Reverse casing where it matters. */
		for ( j = 0; j < len; j++ )
		{
			uint8_t c = lptr[j];
			if ( c == skit_char_ascii_to_lower(c) )
				ilptr[j] = skit_char_ascii_to_upper(c);
			else
				ilptr[j] = skit_char_ascii_to_lower(c);
		}
	}
	
	for ( i = 0; i < n_strings; i++ )
	{
		tests[i].slice = loaves[i].as_slice;
		tests[i].islice = iloaves[i].as_slice;
		tests[i].val = i+1;
	}
	
	sTRACE0(skit_trie_exhaustive_test(tests, n_strings, 0));
	
	for ( i = 0; i < n_strings; i++ )
	{
		skit_loaf_free(&loaves[i]);
		skit_loaf_free(&iloaves[i]);
	}
	
	sTRACE0(skit_free(loaves));
	sTRACE0(skit_free(iloaves));
	sTRACE0(skit_free(tests));
}

static void skit_trie_unittest_examples()
{
	skit_trie *trie = skit_trie_new();
	sASSERT_EQS(skit_trie_setc(trie, "abc", (void*)1, sFLAGS("c")), sSLICE("abc"));
	sASSERT_EQS(skit_trie_setc(trie, "ABC", (void*)2, sFLAGS("io")), sSLICE("abc"));
	sASSERT_EQS(skit_trie_setc(trie, "XYz", (void*)3, SKIT_FLAG_C), sSLICE("XYz"));

	void *val;
	sASSERT_EQS(skit_trie_getc(trie, "abc", &val, SKIT_FLAGS_NONE), sSLICE("abc"));
	sASSERT_EQ((size_t)val, 2, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "xyz", &val, SKIT_FLAG_I), sSLICE("XYz"));
	sASSERT_EQ((size_t)val, 3, "%d");
	sASSERT_EQS(skit_trie_getc(trie, "abcde", &val, SKIT_FLAGS_NONE), skit_slice_null());
	sASSERT_EQ(val, NULL, "%d");
	
	skit_trie_free(trie);
	
	printf("  skit_trie_unittest_examples passed.\n");
}

static void skit_trie_test_iter_examples()
{
	skit_trie *trie = skit_trie_new();
	sASSERT_EQS(skit_trie_setc(trie, "123", (void*)1, sFLAGS("c")), sSLICE("123"));
	sASSERT_EQS(skit_trie_setc(trie, "1234", (void*)2, sFLAGS("c")), sSLICE("1234"));
	sASSERT_EQS(skit_trie_setc(trie, "124", (void*)3, sFLAGS("c")), sSLICE("124"));
	
	skit_slice key;
	void *val;
	skit_trie_iter* iter = skit_trie_iter_new(trie, sSLICE("123"), SKIT_FLAGS_NONE);
	while ( skit_trie_iter_next(iter, &key, &val) )
	{
		if ( skit_slice_eqs(key, sSLICE("123")) )
			sASSERT_EQ((size_t)val, 1, "%d");
		if ( skit_slice_eqs(key, sSLICE("1234")) )
			sASSERT_EQ((size_t)val, 2, "%d");
		sASSERT_NE((size_t)val, 3, "%d"); /* {"124",3} should not be returned. */
	}
	skit_trie_iter_free(iter);
	
	skit_trie_free(trie);
	
	printf("  skit_trie_test_iter_examples passed.\n");
}

void skit_trie_unittest()
{
	printf("skit_trie_unittest()\n");
	skit_trie_unittest_examples();
	skit_trie_test_iter_examples();
	skit_trie_unittest_basics();
	skit_trie_unittest_linear_nodes();
	skit_trie_unittest_table_nodes();
	printf("  skit_trie_unittest passed!\n");
	printf("\n");
	
	if ( 0 )
	{
		/* Shut up the compiler's complaints about useful diagnostic */
		/*   functions being "defined but not used". */
		skit_trie_coords coords;
		(void)skit_trie_coords_dump(&coords, skit_stream_stdout);
		
	}
}

/* Define skit_trie_loaf */
#define SKIT_T_NAMESPACE skit
#define SKIT_T_ELEM_TYPE skit_trie
#define SKIT_T_NAME trie
#include "survival_kit/templates/array.c"
#undef SKIT_T_ELEM_TYPE
#undef SKIT_T_NAME
#undef SKIT_T_NAMESPACE