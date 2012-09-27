
#ifndef SKIT_STREAMS_PFILE_STREAM_INCLUDED
#define SKIT_STREAMS_PFILE_STREAM_INCLUDED

#include <fcntl.h>

#include "survival_kit/streams/stream.h"

typedef struct skit_pfile_stream_internal skit_pfile_stream_internal;
struct skit_pfile_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 name;
	skit_loaf                 err_msg_buf;
	int                       file_descriptor;
};

/** File stream backed by POSIX style file I/O or equivalent. */
typedef union skit_pfile_stream skit_pfile_stream;
union skit_pfile_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_file_stream           as_file_stream;
	skit_pfile_stream_internal as_internal;
};

void skit_pfile_stream_static_init();


/**
Constructor
Allocates a new skit_pfile_stream and calls skit_pfile_stream_init(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_pfile_stream_init(*)
directly.
*/
skit_pfile_stream *skit_pfile_stream_new();

/**
Casts the given stream into a pfile stream.
This will return NULL if the given stream isn't actually a skit_pfile_stream.
*/
skit_pfile_stream *skit_pfile_stream_downcast(skit_stream *stream);

void skit_pfile_stream_init(skit_stream *stream);
skit_slice skit_pfile_stream_readln(skit_stream *stream);
skit_slice skit_pfile_stream_read(skit_stream *stream);
void skit_pfile_stream_writeln(skit_stream *stream, skit_slice line);
void skit_pfile_stream_writefln(skit_stream *stream, const char *fmtstr, ... );
void skit_pfile_stream_writefln_va(skit_stream *stream, const char *fmtstr, va_list vl );
void skit_pfile_stream_write(skit_stream *stream, skit_slice slice);
void skit_pfile_stream_flush(skit_stream *stream);
void skit_pfile_stream_rewind(skit_stream *stream);
skit_slice skit_pfile_stream_slurp(skit_stream *stream);
skit_slice skit_pfile_stream_to_slice(skit_stream *stream);
void skit_pfile_stream_dump(skit_stream *stream, skit_stream *output);
void skit_pfile_stream_open(skit_file_stream *stream, skit_slice fname, const char *access_mode);
void skit_pfile_stream_close(skit_file_stream *stream);

#endif