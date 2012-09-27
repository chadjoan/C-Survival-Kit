
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

void skit_text_stream_init(skit_stream *stream);
skit_slice skit_text_stream_readln(skit_stream *stream);
skit_slice skit_text_stream_read(skit_stream *stream);
void skit_text_stream_writeln(skit_stream *stream, skit_slice line);
void skit_text_stream_writefln(skit_stream *stream, const char *fmtstr, ... );
void skit_text_stream_writefln_va(skit_stream *stream, const char *fmtstr, va_list vl );
void skit_text_stream_write(skit_stream *stream, skit_slice slice);
void skit_text_stream_flush(skit_stream *stream);
void skit_text_stream_rewind(skit_stream *stream);
skit_slice skit_text_stream_slurp(skit_stream *stream);
skit_slice skit_text_stream_to_slice(skit_stream *stream);
void skit_text_stream_dump(skit_stream *stream, skit_stream *output);

#endif