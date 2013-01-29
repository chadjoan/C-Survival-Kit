
#ifndef SKIT_TRIE_INCLUDED
#define SKIT_TRIE_INCLUDED

#define SKIT_TRIE_NODE_PREALLOC 14

typedef struct skit_trie skit_trie;
struct skit_trie
{
	size_t length;
	skit_trie_node *root;
};

typedef struct skit_trie_node skit_trie_node;
struct skit_trie_node
{
	void *nodes;
	char chars[SKIT_TRIE_NODE_PREALLOC];
	uint8_t n_chars_used;
	uint8_t n_nodes_used;
};

#endif
