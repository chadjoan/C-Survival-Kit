
#ifndef SKIT_STREAMS_TEXT_STREAM_INCLUDED
#define SKIT_STREAMS_TEXT_STREAM_INCLUDED

#include "survival_kit/streams/stream.h"

typedef struct skit_text_stream_internal skit_text_stream_internal;
struct skit_text_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 buffer;
	skit_slice                text;
	ssize_t                   cursor;
};

typedef union skit_text_stream skit_text_stream;
union skit_text_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_text_stream_internal  as_internal;
};

void skit_text_stream_static_init();

/**
Constructor
Allocates a new skit_text_stream and calls skit_text_stream_init(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_text_stream_init(*)
directly.
*/
skit_text_stream *skit_text_stream_new();

/**
Casts the given stream into a text stream.
This will return NULL if the given stream isn't actually a skit_text_stream.
*/
skit_text_stream *skit_text_stream_downcast(skit_stream *stream);

void skit_text_stream_init(skit_text_stream *tstream);
void skit_text_stream_init_str(skit_text_stream *tstream, skit_slice slice);
skit_slice skit_text_stream_readln(skit_text_stream *stream, skit_loaf *buffer);
skit_slice skit_text_stream_read(skit_text_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_text_stream_read_fn(skit_text_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_text_stream_appendln(skit_text_stream *stream, skit_slice line);

/** TODO: the number of characters that can be written this way is currently
limited to 1024.  This restriction should be lifted in the future, assuming
sufficient programming time/resources to do so. */
void skit_text_stream_appendf(skit_text_stream *stream, const char *fmtstr, ... );
void skit_text_stream_appendf_va(skit_text_stream *stream, const char *fmtstr, va_list vl );
void skit_text_stream_append(skit_text_stream *stream, skit_slice slice);
void skit_text_stream_flush(skit_text_stream *stream);
void skit_text_stream_rewind(skit_text_stream *stream);
skit_slice skit_text_stream_slurp(skit_text_stream *stream, skit_loaf *buffer);
skit_slice skit_text_stream_to_slice(skit_text_stream *stream, skit_loaf *buffer);
void skit_text_stream_dump(skit_text_stream *stream, skit_stream *output);
void skit_text_stream_dtor(skit_text_stream *stream);

void skit_text_stream_unittests();

#endif
