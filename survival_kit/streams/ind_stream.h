
/**
A stream that indents text that is appended to it.
This stream will proxy all reads and writes to another stream that is assigned
at the time it is constructed.
*/

#ifndef SKIT_STREAMS_IND_STREAM_INCLUDED
#define SKIT_STREAMS_IND_STREAM_INCLUDED

#include "survival_kit/string.h"
#include "survival_kit/streams/stream.h"

typedef struct skit_ind_stream_internal skit_ind_stream_internal;
struct skit_ind_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_stream               *backing_stream;
	const char                *indent_str;
	short                     indent_level;
	char                      last_write_was_newline;
};

typedef union skit_ind_stream skit_ind_stream;
union skit_ind_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_ind_stream_internal   as_internal;
};

void skit_ind_stream_static_init();

/**
Constructor
Allocates a new skit_ind_stream and calls skit_ind_stream_init(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_ind_stream_init(*)
directly.
*/
skit_ind_stream *skit_ind_stream_new(skit_stream *backing);

/**
Casts the given stream into an indented stream.
This will return NULL if the given stream isn't actually a skit_ind_stream.
*/
skit_ind_stream *skit_ind_stream_downcast(const skit_stream *stream);

void skit_ind_stream_init(skit_ind_stream *istream, skit_stream *backing);
skit_slice skit_ind_stream_readln(skit_ind_stream *stream, skit_loaf *buffer);
skit_slice skit_ind_stream_read(skit_ind_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_ind_stream_read_fn(skit_ind_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_ind_stream_appendln(skit_ind_stream *stream, skit_slice line);

/** BUG: any newlines that appear from the expansion of format arguments will not be indented. */
/** (Only the formatstring itself will be subject to indentation replacement.) */
void skit_ind_stream_appendf(skit_ind_stream *stream, const char *fmtstr, ... );
void skit_ind_stream_appendf_va(skit_ind_stream *stream, const char *fmtstr, va_list vl );
void skit_ind_stream_append(skit_ind_stream *stream, skit_slice slice);
void skit_ind_stream_flush(skit_ind_stream *stream);
void skit_ind_stream_rewind(skit_ind_stream *stream);
skit_slice skit_ind_stream_slurp(skit_ind_stream *stream, skit_loaf *buffer);
skit_slice skit_ind_stream_to_slice(skit_ind_stream *stream, skit_loaf *buffer);
void skit_ind_stream_dump(const skit_ind_stream *stream, skit_stream *output);

void skit_ind_stream_incr_indent(skit_ind_stream *stream);
void skit_ind_stream_decr_indent(skit_ind_stream *stream);
short skit_ind_stream_get_ind_lvl(const skit_ind_stream *stream);
const char *skit_ind_stream_get_ind_str(const skit_ind_stream *stream);
void skit_ind_stream_set_ind_str(skit_ind_stream *stream, const char *c);


/** Note that this does NOT free the backing stream. */
void skit_ind_stream_dtor(skit_ind_stream *stream);

void skit_ind_stream_unittests();

#endif
