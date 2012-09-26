
#ifndef SKIT_STREAMS_STREAM_INCLUDED
#define SKIT_STREAMS_STREAM_INCLUDED

#include <stdarg.h>

#include "survival_kit/streams/meta.h"

typedef struct skit_stream_common_fields skit_stream_common_fields;
struct skit_stream_common_fields
{
	const char *indent_str;
	short      indent_level;
};


typedef struct skit_stream_internal skit_stream_internal;
struct skit_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
};

typedef union skit_stream skit_stream;
union skit_stream
{
	skit_stream_metadata meta;
	skit_stream_internal as_internal;
};

void skit_stream_vtable_init(skit_stream_vtable_t *table);
void skit_stream_static_init();

void skit_stream_init(skit_stream *stream);
skit_slice skit_stream_readln(skit_stream *stream);
skit_slice skit_stream_read(skit_stream *stream);
void skit_stream_writeln(skit_stream *stream, skit_slice line);
void skit_stream_writefln(skit_stream *stream, const char *fmtstr, ...);
void skit_stream_writefln_va(skit_stream *stream, const char *fmtstr, va_list vl);
void skit_stream_write(skit_stream *stream, skit_slice slice);
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
const char *skit_stream_get_indent_char(skit_stream *stream);
void skit_stream_set_indent_char(skit_stream *stream, const char *c);

#endif
