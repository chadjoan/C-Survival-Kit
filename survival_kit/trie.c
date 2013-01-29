
#if defined(__DECC)
#pragma module skit_trie
#endif

typedef struct skit_trie skit_trie;
struct skit_trie
{
	size_t length;
	skit_trie_node *root;
};

typedef struct skit_trie_node skit_trie_node;
struct skit_trie_node
{
	union
	{
		void *node;   /* Use if ( current->n_nodes_used == 1 ) */
		void **nodes; /* Use if ( current->n_nodes_used > 1 ) */
	}
	char chars[SKIT_TRIE_NODE_PREALLOC];
	uint8_t n_chars_used;
	uint8_t n_nodes_used;
};

#define SKIT_TRIE_LOOKUP_FAILED 0

/* This is a globally available read-only node that is used instead of NULL
to indicate the end of a trie.  Whenever the lookup is scanning a trie and
finds an address to this thing, it means, assuming all input is consumed,
that the lookup is successful.
*/
const static skit_trie_node SUCCESS_NODE;

typedef struct skit_trie_result skit_trie_result;
struct skit_trie_result
{
	skit_trie_node *next_node;
	size_t         next_pos;
};

static skit_trie_result skit_trie_result_fail()
{
	skit_trie_result result;
	result.next_node = NULL;
	result.next_pos = SKIT_TRIE_LOOKUP_FAILED;
	return result;
}

static skit_trie_result skit_trie_next_node(
	skit_trie_node *current,
	const char *key,
	size_t key_len,
	size_t pos)
{
	skit_trie_result result;
	
	sASSERT(current->nodes != NULL);
	sASSERT(current->n_nodes_used > 0 );
	
	if ( current->n_nodes_used == 1 )
	{
		/*
		Narrow radix search: there is only one possible node to
		end up at.  The string to match to get there is the one
		contained in the 'chars' member and is no longer than
		'n_chars_used'
		*/
		size_t chars_len = current->n_chars_used;
		if ( pos + chars_len > key_len )
			return skit_trie_result_fail();
		
		size_t i = 0;
		while ( i < chars_len )
		{
			if ( key[pos] != current->chars[i] )
				return skit_trie_result_fail();
			
			pos++;
			i++;
		}
		
		result.next_node = current->node;
		result.next_pos = pos;
		return result;
	}
	else if ( current->n_chars_used < SKIT_TRIE_NODE_PREALLOC )
	{
		/*
		There is more than one choice, but few enough that we can
		fit them in a small amount of space.
		We scan the 'chars' array; when an element in that matches the
		current char in our key, then that index is the index to use
		in the 'nodes' array.
		*/
		sASSERT( current->n_chars_used == current->n_nodes_used );
		
		if ( pos >= key_len )
			return skit_trie_result_fail();
		
		size_t chars_len = current->n_chars_used;
		size_t i = 0;
		for ( ; i < chars_len; i++ )
			if ( current->chars[i] == key[pos] )
				break;
		
		if ( i == chars_len )
			return skit_trie_result_fail();
		
		result.next_node = current->nodes[i];
		result.next_pos = pos + 1;
		return result;
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
			return skit_trie_result_fail();
		
		result.next_node = current->nodes[key[pos]];
		if ( result.next_node != NULL )
			result.next_pos = pos + 1;
		else
			return skit_trie_result_fail();
		
		return result;
	}
	
	sASSERT(0);
}
