
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

/**
(virtual)
Reads a line from the stream.
The line will be in the returned slice.
If the end of the stream has already been reached, then skit_slice_null() is
returned.
The line ending itself is not returned in the slice.  (In general, some streams
may not even use the conventional newline delimiters to separate lines.)

'buffer' is an optional parameter.  If it is provided, then the stream /may/
use it to store the returned slice.  If NULL is passed for 'buffer', then the
stream must use an internally allocated buffer to store the returned slice.
It is not guaranteed that the stream will actually use the buffer parameter, 
since some streams, like skit_text_stream, can avoid copying by returning a 
slice into the stream's internal buffers.

Example:

// The given stream has the contents "foo\n\nbar\nbaz"
void skit_stream_readln_unittest(skit_stream *stream)
{
	sASSERT_EQS(skit_stream_readln(stream), sSLICE("foo"));
	sASSERT_EQS(skit_stream_readln(stream), sSLICE(""));
	sASSERT_EQS(skit_stream_readln(stream), sSLICE("bar"));
	sASSERT_EQS(skit_stream_readln(stream), sSLICE("baz"));
	sASSERT_EQS(skit_stream_readln(stream), skit_slice_null());
}

*/
skit_slice skit_stream_readln(skit_stream *stream, skit_loaf *buffer);
skit_slice skit_stream_read(skit_stream *stream, skit_loaf *buffer, size_t nbytes);
void skit_stream_writeln(skit_stream *stream, skit_slice line);
void skit_stream_writefln(skit_stream *stream, const char *fmtstr, ...);
void skit_stream_writefln_va(skit_stream *stream, const char *fmtstr, va_list vl);
void skit_stream_write(skit_stream *stream, skit_slice slice);
void skit_stream_flush(skit_stream *stream);
void skit_stream_rewind(skit_stream *stream);
skit_slice skit_stream_slurp(skit_stream *stream, skit_loaf *buffer);
skit_slice skit_stream_to_slice(skit_stream *stream, skit_loaf *buffer);
void skit_stream_dump(skit_stream *stream, skit_stream *out);

/**
(virtual)
Frees any memory resources associated with the given stream.
This does not free the data at the end of the *stream pointer itself; that
  data is left untouched because it may be stack-allocated.
Calling skit_stream_dtor on an already free'd stream is a safe do-nothing op.
If the caller allocated a stream using skit_*_stream_new(), skit_malloc(), or
  a similar manner of dynamic allocation, then it is recommended to call
  skit_stream_delete instead of skit_stream_dtor so that the memory used to
  store the stream object itself may be freed.
Be careful when stack-allocating polymorphic data structures like streams!
  Never pass streams by value as this will "slice" off the part of memory
  that contains data for the derived instance.  Downcasting and virtual
  calls may appear to work but silently corrupt data under such conditions.
  Always pass/assign streams as pointers.
*/
void skit_stream_dtor(skit_stream *stream);

/**
(final)
Calls skit_stream_dtor on the given stream object, then calls skit_free on
  the memory pointed to by *stream.  
This is intended for use with dynamically allocated stream objects, such as
  those created by calls to the skit_*_stream_new() methods defined with the
  derived classes.
*/
void skit_stream_delete(skit_stream *stream);

/* Streams can't be opened/closed in general, so those methods are absent. */

/* -------------------------- final methods -------------------------------- */

void skit_stream_incr_indent(skit_stream *stream);
void skit_stream_decr_indent(skit_stream *stream);
short skit_stream_get_indent_lvl(skit_stream *stream);
const char *skit_stream_get_indent_char(skit_stream *stream);
void skit_stream_set_indent_char(skit_stream *stream, const char *c);

#endif
