
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

void skit_text_stream_module_init();

/**
Constructor
Allocates a new skit_text_stream and calls skit_text_stream_ctor(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_text_stream_ctor(*)
directly.
*/
skit_text_stream *skit_text_stream_new();

/**
Casts the given stream into a text stream.
This will return NULL if the given stream isn't actually a skit_text_stream.
*/
skit_text_stream *skit_text_stream_downcast(const skit_stream *stream);

void skit_text_stream_ctor(skit_text_stream *tstream);
void skit_text_stream_ctor_n(skit_text_stream *tstream, size_t est_buffer_size);
void skit_text_stream_init_str(skit_text_stream *tstream, skit_slice slice);

/**
Removes all text currently stored in the existing text stream.
The end result is an "empty" text stream.
This is useful because the stream's internal buffers are not freed by the
operation, so it is a fast and efficient way to repeatedly use the same
stream as a write buffer (ex: when serializing the same data structures
over and over).
*/
void skit_text_stream_clear(skit_text_stream *stream);

/**
Constructs a text stream that uses the given buffer as its internal buffer
and a cursor_pos variable to indicate how much of the buffer is already
filled and where to place newly appended contents.

This is a way to efficiently integrate streams with the idiom of having
a slice to a buffer and growing the slice using calls to
skit_slice_buffered_append.  With this function, it becomes possible to
accept such a buffer+slice pair and then pass a stream into other functions
that require streams, and to do so with no unnecessary allocations.

After the stream is done being manipulated, use skit_text_stream_slurp to
recover the new slice, and then use skit_text_stream_dissociate to get the 
text buffer back and prep the stream for destruction/freeing.

Ex:
	SKIT_LOAF_ON_STACK(buf, 32);
	skit_slice result = skit_slice_of(buf.as_slice, 0, 0);
	skit_slice_buffered_append(&buf, &result, sSLICE("Hello "));

	skit_text_stream tstream;
	skit_text_stream_subsume(&tstream, buf, sSLENGTH(result));
	skit_text_stream_append(&tstream, sSLICE("world!"));
	result = skit_text_stream_slurp(&tstream, NULL);
	buf = skit_text_stream_dissociate(&tstream);
	skit_text_stream_dtor(&tstream);

	// 'buf' should still be valid at this point.
	// The call to skit_text_stream_dissociate will cause the caller to 
	//   once again own the buffer, which will prevent the buffer from 
	//   being deallocated in the call to skit_text_stream_dtor(&tstream).

	sASSERT_EQS(result, sSLICE("Hello world!"));

	skit_loaf_free(&buf);
	// /Now/ 'buf' is gone.
*/
void skit_text_stream_subsume(skit_text_stream *tstream, skit_loaf buffer, size_t cursor_pos);
skit_loaf skit_text_stream_dissociate(skit_text_stream *tstream); /** ditto */

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
void skit_text_stream_dump(const skit_text_stream *stream, skit_stream *output);
void skit_text_stream_dtor(skit_text_stream *stream);

void skit_text_stream_unittests();

#endif
