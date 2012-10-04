
#ifndef SKIT_STREAMS_STREAM_INCLUDED
#define SKIT_STREAMS_STREAM_INCLUDED

#include <stdarg.h>

#include "survival_kit/feature_emulation.h"
#include "survival_kit/streams/meta.h"

/**
Justifications for a stream package when the C standard already defines stream
operations:
- skit's implementation supports portable in-memory text streams.
- It provides streams that throw exceptions rather than returning ignorable 
    error codes.
- Better debugging output when things fail. Every stream has a *_dump function.
- Extensibility: since this code is not OS/vendor-specific, it can be used to 
    modify the behavior of code that is otherwise inaccessible.
*/

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

/** 
This initialization function is only for inheriting classes to call.
Do not try to construct a pure skit_stream object: it is considered an abstract
class and none of its methods will be able to dispatch to anything, which
would result in thrown exceptions or some form of crashing.
*/
void skit_stream_init(skit_stream *stream);

/**
(virtual)
Reads a line from the stream.
The line will be in the returned slice.
If the end of the stream has already been reached, then skit_slice_null() is
returned.
The line ending itself is not returned in the slice.  (In general, some streams
may not even use the conventional newline delimiters to separate lines.)

'buffer' is an optional parameter.  
If it is provided, then the stream /may/ use it to store the returned slice.
The stream may also resize this buffer as required.
If NULL is passed for 'buffer', then the stream must use an internally 
allocated buffer to store the returned slice.
It is not guaranteed that the stream will actually use the buffer parameter, 
since some streams, like skit_text_stream, can avoid copying by returning a 
slice into the stream's internal buffers.

Example:

// The given stream has the contents "foo\n\n\0bar\nbaz\n"
void skit_stream_readln_unittest(skit_stream *stream)
{
	skit_loaf buf = skit_loaf_alloc(3);
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("foo"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE(""));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("bar"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("baz"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE(""));
	sASSERT_EQS(skit_stream_readln(stream,&buf), skit_slice_null());
	skit_loaf_free(&buf);
}
*/
skit_slice skit_stream_readln(skit_stream *stream, skit_loaf *buffer);

/**
(virtual)
Reads a block of bytes from the stream.
These will be in the returned slice.
If the block requested extends beyond the end of the stream, then the stream's
remaining bytes will be returned.
If the end of the stream has already been reached, then skit_slice_null() is
returned.  This way, forgetting to check for EOF conditions becomes a bug that
causes segfaults/crashing instead of a bug that causes infinite loops.

'buffer' is an optional parameter.  
If it is provided, then the stream /may/ use it to store the returned slice.
The stream may also resize this buffer as required.
If NULL is passed for 'buffer', then the stream must use an internally 
allocated buffer to store the returned slice.
It is not guaranteed that the stream will actually use the buffer parameter, 
since some streams, like skit_text_stream, can avoid copying by returning a 
slice into the stream's internal buffers.

Example:

// The given stream has the contents "foobarbaz"
void skit_stream_read_unittest(skit_stream *stream)
{
	skit_loaf buf = skit_loaf_alloc(3);
	sASSERT_EQS(skit_stream_read(stream,&buf,3), sSLICE("foo"));
	sASSERT_EQS(skit_stream_read(stream,&buf,0), sSLICE(""));
	sASSERT_EQS(skit_stream_read(stream,&buf,3), sSLICE("bar"));
	sASSERT_EQS(skit_stream_read(stream,&buf,4), sSLICE("baz"));
	sASSERT_EQS(skit_stream_read(stream,&buf,3), skit_slice_null());
	skit_loaf_free(&buf);
}
*/
skit_slice skit_stream_read(skit_stream *stream, skit_loaf *buffer, size_t nbytes);

/**
(virtual)
Positions the cursor to the end of the stream and then writes the given slice
into the stream, followed by a line ending.

For targets that use textual line delimiters, the linefeed character '\n' is
written after the given slice.  

Example:

// The given stream has the contents ""
void skit_stream_appendln_unittest(skit_stream *stream)
{
	skit_loaf buf = skit_loaf_alloc(64);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE(""));
	skit_stream_appendln(stream, sSLICE("foo"));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\n"));
	skit_stream_appendln(stream, sSLICE(""));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\n\n"));
	skit_stream_appendln(stream, sSLICE("bar"));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\n\nbar\n"));
	skit_stream_appendln(stream, sSLICE("baz"));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\n\nbar\nbaz\n"));
	skit_loaf_free(&buf);
}
*/
void skit_stream_appendln(skit_stream *stream, skit_slice line);

/**
(virtual)
Positions the cursor to the end of the stream and then writes formatted text
into the stream, followed by a line ending.

For targets that use textual line delimiters, the linefeed character '\n' is
written after the given slice.  

Example:

// The given stream has the contents ""
void skit_stream_appendfln_unittest(skit_stream *stream)
{
	skit_loaf buf = skit_loaf_alloc(64);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE(""));
	skit_stream_appendfln(stream, "foo");
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\n"));
	skit_stream_appendfln(stream, "%s", "bar");
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\nbar\n"));
	skit_stream_appendfln(stream, "%d", 3);
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo\nbar\n3\n"));
	skit_loaf_free(&buf);
}
*/
void skit_stream_appendfln(skit_stream *stream, const char *fmtstr, ...);
void skit_stream_appendfln_va(skit_stream *stream, const char *fmtstr, va_list vl);

/**
(virtual)
Positions the cursor to the end of the stream and then writes the bytes in the
given slice into the stream.

Example:

// The given stream has the contents ""
void skit_stream_append_unittest(skit_stream *stream)
{
	skit_loaf buf = skit_loaf_alloc(64);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE(""));
	skit_stream_append(stream, sSLICE("foo"));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo"));
	skit_stream_append(stream, sSLICE(""));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foo"));
	skit_stream_append(stream, sSLICE("bar"));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foobar"));
	skit_stream_append(stream, sSLICE("baz"));
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_slurp(stream,&buf), sSLICE("foobarbaz"));
	skit_loaf_free(&buf);
}
*/
void skit_stream_append(skit_stream *stream, skit_slice slice);

/**
(virtual)
Causes any buffered writes into the stream to be actualized on the stream's
target.

For targets that don't require the extra layer of buffering (such as the
skit_text_stream) this will be a do-nothing operation.  Such targets will
always have writes actualize immediately without having to flush.
*/
void skit_stream_flush(skit_stream *stream);

/**
(virtual)
Sets the stream's cursor position to the very beginning of the stream.

// The given stream has the contents "".  The cursor starts at the begining.
void skit_stream_rewind_unittest(skit_stream *stream)
{
	skit_loaf buf = skit_loaf_alloc(64);
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE(""));
	skit_stream_appendln(stream, sSLICE("foo"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), skit_slice_null());
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("foo"));
	skit_loaf_free(&buf);
}
*/
void skit_stream_rewind(skit_stream *stream);

/**
(virtual)
Returns the the stream's contents starting at the cursor and ending at the
end of the stream.  The contents are returned as one long string.
The result of this operation moves the stream's cursor to the end of the
stream.

'buffer' is an optional parameter.  
If it is provided, then the stream /may/ use it to store the returned slice.
The stream may also resize this buffer as required.
If NULL is passed for 'buffer', then the stream must use an internally 
allocated buffer to store the returned slice.
It is not guaranteed that the stream will actually use the buffer parameter, 
since some streams, like skit_text_stream, can avoid copying by returning a 
slice into the stream's internal buffers.

Note that some streams, when passed NULL for the 'buffer' parameter on the
slurp function, may hold onto the internal buffer's memory until the stream
has its destructor called.  If the caller wishes to have this memory freed
before the destructor call, then the caller should pass a 'buffer' argument
and free the buffer in calling code.  Since this function is likely to be
called at the end of a stream's lifetime, it may be perfectly feasible to
just let the destructor handle it in such cases.  

See skit_stream_appendln for an example of how this is used.
*/
skit_slice skit_stream_slurp(skit_stream *stream, skit_loaf *buffer);

/**
(virtual)
Returns a brief string identifying the stream in some way.
This is usually the stream's class name (ex: "skit_text_stream", 
"skit_pfile_stream", etc).
*/
skit_slice skit_stream_to_slice(skit_stream *stream, skit_loaf *buffer);

/**
(virtual)
Writes target-specific information about the stream into the given output
stream.
This is useful in debugging and logging as a means to know what condition
a stream was in before any errors might have occured.  For example, a file
stream's dump will include things like the file's name/path, it's access
mode, and possibly information about the internal buffer(s) used to store
data being read from or written to the disk.
*/
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

/**
This throws an exception with the given code and message.
Additionally, it will call the given stream's skit_stream_dump method and
  dump that information into the exception text.
This is the preferred way to throw exceptions with streams because of the
  superior debugging output it provides.
*/
void skit_stream_throw_exc( skit_err_code ecode, skit_stream *stream, const char *msg, ... );

/* ------------------------- generic unittests ----------------------------- */

// The given stream has the contents "foo\n\n\0bar\nbaz\n"
void skit_stream_readln_unittest(skit_stream *stream);
#define SKIT_READLN_UNITTEST_CONTENTS "foo\n\n\0bar\nbaz\n"

// The given stream has the contents "foobarbaz"
void skit_stream_read_unittest(skit_stream *stream);
#define SKIT_READ_UNITTEST_CONTENTS "foobarbaz"

// The given stream has the contents ""
void skit_stream_appendln_unittest(skit_stream *stream);
#define SKIT_APPENDLN_UNITTEST_CONTENTS ""

// The given stream has the contents ""
void skit_stream_appendfln_unittest(skit_stream *stream);
#define SKIT_APPENDFLN_UNITTEST_CONTENTS ""

// The given stream has the contents ""
void skit_stream_append_unittest(skit_stream *stream);
#define SKIT_APPEND_UNITTEST_CONTENTS ""

// The given stream has the contents "".  The cursor starts at the begining.
void skit_stream_rewind_unittest(skit_stream *stream);
#define SKIT_REWIND_UNITTEST_CONTENTS ""

#endif
