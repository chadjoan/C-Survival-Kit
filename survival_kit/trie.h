
#ifndef SKIT_TRIE_INCLUDED
#define SKIT_TRIE_INCLUDED

#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h> /* For ssize_t */

#include "survival_kit/string.h"
#include "survival_kit/flags.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/feature_emulation.h"

/* ------------------------------------------------------------------------- */

extern skit_err_code SKIT_TRIE_EXCEPTION;
extern skit_err_code SKIT_TRIE_KEY_ALREADY_EXISTS;
extern skit_err_code SKIT_TRIE_KEY_NOT_FOUND;
extern skit_err_code SKIT_TRIE_BAD_FLAGS;
extern skit_err_code SKIT_TRIE_WRITE_IN_ITERATION;

#define SKIT__TRIE_NODE_PREALLOC 12

typedef struct skit_trie_node skit_trie_node;
struct skit_trie_node
{
	union
	{
		skit_trie_node *array;
		skit_trie_node **table;
	} nodes;
	const void *value;
	uint16_t nodes_len;
	uint8_t  have_value;
	uint8_t  chars_len;
	uint8_t chars[SKIT__TRIE_NODE_PREALLOC];
};

typedef struct skit_trie skit_trie;
struct skit_trie
{
	size_t length;
	skit_loaf key_return_buf; /* sLLENGTH(key_return_buf) should always be equal to the longest key. */
	skit_trie_node *root;
	
	int32_t iterator_count;
};

typedef struct skit_trie_iter skit_trie_iter;

/* Do not call directly.  skit_init() should handle this. */
void skit_trie_module_init();

/**
Allocates memory for and initializes a new trie.
This is equivalent to calling skit_malloc to allocate a skit_trie struct and
then calling skit_trie_ctor on the skit_trie struct.
*/
skit_trie *skit_trie_new();

/**
Initializes a new trie and acquires any resources needed for it.

This must be matched by a call to skit_trie_dtor, or possibly skit_trie_free
if the memory pointed to by *trie was allocated with skit_malloc.
*/
void skit_trie_ctor( skit_trie *trie );

/**
Frees all memory resources associated with the trie in addition to the memory
directly pointed to by *trie.
*/
skit_trie *skit_trie_free(skit_trie *trie);

/** 
Frees all memory resources associated with the trie without freeing the memory
directly pointed to by *trie.
*/
void skit_trie_dtor( skit_trie *trie );

/**
Retrieves the value associated with the given key.

If the value exists, then the pointer pointed to by the 'value' parameter will
be modified to point to the value associated with the key.  In this case, the
return value is the key that matched.  This returned slice may be modified by 
any subsequent calls on the trie: copy it if you intend to use the value.

If the value does not exist, *value will be set to NULL.  In this case, the
return value is skit_slice_null().

The flags string acts the same as in skit_trie_set, except that both 'c' and
'o' have no meaning for retrieval operations and are ignored.  They can be
passed, but they will do nothing.

When providing no flags, it is allowable to pass either the empty string "" or
NULL into the flags parameter.

case_sensitive determines whether or not case-sensitive matching is used when
checking the key against possible matches.  It is possible for a 
case-insensitive query to match multiple values.  In that case, implementation
defined behavior will select one of the possibilities.
*/
skit_slice skit_trie_get( skit_trie *trie, const skit_slice key, void **value, skit_flags flags );
skit_slice skit_trie_getc( skit_trie *trie, const char *key, void **value, skit_flags flags );

/**
Associate the given key with the given value.
If the key already exists in the trie, then the previous value will be
overwritten with the one given.

The flags string can be used to configure the behavior of this operation.
This allows the calling code to easily enforce its assumptions about the 
state of the trie before the setting operation.
The characters that are allowed in the flags string are as follows:
'i' - Case insensitive.  This determines how strict the check is when looking
        for already existing keys.
'c' - With the 'c' flag cleared, a SKIT_TRIE_KEY_NOT_FOUND exception is thrown
        when attempting to write into a non-existant key's value.  Setting the
        'c' flag allows the setter to create the key as needed if it doesn't
        already exist.
'o' - With the 'o' flag cleared, a SKIT_TRIE_KEY_ALREADY_EXISTS exception is
        thrown when attempting to write to an already existing key's value.
        Setting the 'o' flag allows the setter to overwrite an existing value
        instead of throwing the exception.

Note that it is necessary to pass at least one of 'c' or 'o' in the flags for
this to complete successfully.  Passing neither requires the key to
simultaneously exist and not exist in the trie at the time of the setting.
For this reason, any set of flags without one of those will immediately throw
a SKIT_TRIE_BAD_FLAGS exception to indicate the contradiction.

If any characters besides those specified is found in the flags string, an
assertion may trigger.  The caller is expected to pass valid flags strings.
To clarify: the flags string does not allow separators like spaces or dashes,
and may fail entirely if anything besides the above defined characters is 
placed in it.

Returns the /exact/ key acted on.  This returned slice may be modified by 
any subsequent calls on the trie: copy it if you intend to use the value.
*/
skit_slice skit_trie_set( skit_trie *trie, const skit_slice key, const void *value, skit_flags flags );
skit_slice skit_trie_setc( skit_trie *trie, const char *key, const void *value, skit_flags flags );

/**
Removes a key-value pair from the trie that matches the given key.

The flags string acts the same as in skit_trie_set, with the following
differences:
'c' - With the 'c' flag cleared, a SKIT_TRIE_KEY_NOT_FOUND exception is thrown
        when attempting to remove a non-existant key-value pair.  Setting the
        'c' flag causes removal to do nothing in such cases.
'o' - Is ignored.

When providing no flags, it is allowable to pass either the empty string "" or
NULL into the flags parameter.

Returns the /exact/ key acted on.  This returned slice may be modified by 
any subsequent calls on the trie: copy it if you intend to use the value.
*/
skit_slice skit_trie_remove( skit_trie *trie, const skit_slice key, skit_flags flags );

/**
Returns the number of key-value pairs in the trie.
*/
size_t skit_trie_len( const skit_trie *trie );

/**
Creates a graphical respresentation of the trie using ASCII characters and
writes the resulting text into the output stream.
*/
void skit_trie_dump( const skit_trie *trie, skit_stream *output );

/* TODO: iteration prefix subsets. */

/**
Creates an iterator that can iterate over all key/value pairs in the trie that
have a key beginning with the given prefix.


*/
skit_trie_iter *skit_trie_iter_new( skit_trie *trie, const skit_slice prefix, skit_flags flags );
skit_trie_iter *skit_trie_iter_free( skit_trie_iter *iter );
int skit_trie_iter_next( skit_trie_iter *iter, skit_slice *key, void **value );

void skit_trie_unittest();

#endif
