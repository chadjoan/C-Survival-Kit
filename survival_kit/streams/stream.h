
#ifndef SKIT_STREAMS_STREAM_INCLUDED
#define SKIT_STREAMS_STREAM_INCLUDED

#include "survival_kit/streams/meta.h"

typedef struct skit_stream_common_fields skit_stream_common_fields;
struct skit_stream_common_fields
{
	char   indent_char;
	short  indent_level;
};


typedef struct skit_text_stream_internal skit_text_stream_internal;
struct skit_text_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 buffer;
	skit_slice                text;
};

typedef union skit_stream skit_stream;
union skit_stream
{
	skit_stream_metadata meta;
};

void skit_stream_vtable_init(skit_stream_vtable_t *table);
skit_slice skit_stream_read_line(skit_stream *stream);
skit_slice skit_stream_read_bytes(skit_stream *stream);
void skit_stream_write_line(skit_stream *stream, skit_slice slice);
void skit_stream_write_bytes(skit_stream *stream, skit_slice slice);
void skit_stream_flush(skit_stream *stream);
void skit_stream_rewind(skit_stream *stream);
skit_slice skit_stream_slurp(skit_stream *stream);
skit_slice skit_stream_to_slice(skit_stream *stream);
void skit_stream_dump(skit_stream *stream, skit_stream *out);

/* Streams can't be opened/closed in general, so those methods are absent. */

/* -------------------------- final methods -------------------------------- */

void skit_stream_incr_indent(skit_stream *stream);
void skit_stream_decr_indent(skit_stream *stream);
short skit_stream_get_indent_lvl(skit_stream *stream);
char skit_stream_get_indent_char(skit_stream *stream);
void skit_stream_set_indent_char(skit_stream *stream, char c);

#endif
