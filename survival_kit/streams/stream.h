
#ifndef SKIT_STREAMS_STREAM_INCLUDED
#define SKIT_STREAMS_STREAM_INCLUDED

#include <stdarg.h>
#include <inttypes.h>

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
	size_t place_holder;
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

/** */
typedef struct skit_custom_read_context skit_custom_read_context;
struct skit_custom_read_context
{
	void        *caller_context;
	skit_slice  current_slice;   /** Read-only. Includes current_char. */
	skit_utf8c  current_char;
};

#define SKIT_STREAM_T void
#define SKIT_VTABLE_T skit_stream_vtable_t
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

void skit_stream_vtable_init(skit_stream_vtable_t *table);
void skit_stream_module_init();

/** 
This initialization function is only for inheriting classes to call.
Do not try to construct a pure skit_stream object: it is considered an abstract
class and none of its methods will be able to dispatch to anything, which
would result in thrown exceptions or some form of crashing.
*/
void skit_stream_ctor(skit_stream *stream);

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
For more complex reading operations, this method exposes a way for the stream
to feed the caller a character at a time while still maintaining the entire
result in a buffer that may be provided by the stream.  

The caller is expected to provide an "accept_char" function.  Once the read_fn
method is entered, accept_char will be called repeatedly by the stream until
one of two things happens:
(1) accept_char returns 0.
(2) The end of the stream is reached.

When one of the above happens, the skit_stream_read_fn call will return
whatever slice was last found in the custom read context's "current_slice"
member.

It is also possible for the stream to throw an exception before the iteration
is finished.  This is most likely to happen when the stream attempts to read
the next character and receives an I/O error from the underlying layer.

Do not call the stream's methods while in the accept_char callback.  This
could potentially cause strange things to happen, especially if the method
called requires use of the stream's internal buffer that is being used to
provide the "current_slice" member of the custom read context.  

'context' is an optional (but recommended) parameter that allows the caller
to pass its own variables and state into the accept_char function.  It is
exposed in calls to accept_char through the 'caller_context' member of the
skit_custom_read_context structure.

'buffer' is an optional parameter.  
If it is provided, then the stream /may/ use it to store the returned slice.
The stream may also resize this buffer as required.
If NULL is passed for 'buffer', then the stream must use an internally 
allocated buffer to store the returned slice.
It is not guaranteed that the stream will actually use the buffer parameter, 
since some streams, like skit_text_stream, can avoid copying by returning a 
slice into the stream's internal buffers.
*/
skit_slice skit_stream_read_fn(skit_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));

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
void skit_stream_appendf(skit_stream *stream, const char *fmtstr, ...);
void skit_stream_appendf_va(skit_stream *stream, const char *fmtstr, va_list vl);

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
void skit_stream_dump(const skit_stream *stream, skit_stream *out);

/**
(virtual)
Frees any memory resources associated with the given stream.
This does not free the data at the end of the *stream pointer itself; that
  data is left untouched because it may be stack-allocated.
Calling skit_stream_dtor on an already free'd stream is a safe do-nothing op.
If the caller allocated a stream using skit_*_stream_new(), skit_malloc(), or
  a similar manner of dynamic allocation, then it is recommended to call
  skit_stream_free instead of skit_stream_dtor so that the memory used to
  store the stream object itself may be freed.
Be careful when stack-allocating polymorphic data structures like streams!
  Never pass streams by value as this will "slice" off the part of memory
  that contains data for the derived instance.  Downcasting and virtual
  calls may appear to work but silently corrupt data under such conditions.
  Always pass/assign streams as pointers.
*/
void skit_stream_dtor(skit_stream *stream);

/**
(virtual, optional to implement)
These functions allow manipulation of streams supporting indentation awareness.

If a stream does not support indentation awareness, then calling these
functions will do nothing and the stream will always behave as if the
indentation level is zero and the indentation string is "".

A stream supporting indentation awareness will scan all inputs to its append
methods and replace instances of newlines with the newlines followed by the
indentation string repeated (indentation level) number of times.

This set of functions is useful for formatting nested structures without
needing the nested structures to pass around indentation counts.  This solves
the common problem with passing indentation counts: recursive formatting calls
will often lose such counts when they must pass through other calls (such as 
from 3rd parties) that do not support indentation.  Passing an indentation-
aware stream into such functions will allow indentation information to be 
preserved and even acted upon by 3rd party functions, without those functions 
even having to consider indentation as a design decision at all.

See skit_ind_stream for a basic stream that can forward all operations on it
to another skit_stream while inserting indentation as necessary.
*/
void skit_stream_incr_indent(skit_stream *stream);
void skit_stream_decr_indent(skit_stream *stream); /** ditto */
int16_t skit_stream_get_ind_lvl(const skit_stream *stream); /** ditto */
int16_t skit_stream_get_peak(const skit_stream *stream); /** ditto */
const char *skit_stream_get_ind_str(const skit_stream *stream); /** ditto */
void skit_stream_set_ind_str(skit_stream *stream, const char *c); /** ditto */

/**
(final)
Calls skit_stream_dtor on the given stream object, then calls skit_free on
  the memory pointed to by *stream.  
This is intended for use with dynamically allocated stream objects, such as
  those created by calls to the skit_*_stream_new() methods defined with the
  derived classes.
*/
void skit_stream_free(skit_stream *stream);

/* Streams can't be opened/closed in general, so those methods are absent. */

/* -------------------------- final methods -------------------------------- */

/** 
This does a number of routine things that objects tend to do when implementing
a function/method of the form namespace_object_dump( instance, output ).
These are the following:
(1) Asserts that output is non-null.
(2) If ptr is NULL, it will dump the contents of text_if_null to output.
    If ptr is non-null, it will write nothing to output.

Returns 1 if ptr is NULL, returns 0 if ptr is non-NULL.

Example:
void mylib_mytype_dump( const mytype *instance, skit_stream *output )
{
	if ( skit_stream_dump_null(output, instance, sSLICE("NULL mytype\n")) )
		return;
	
	// Print detailed stuff in the non-null case.
}
*/
int skit_stream_dump_null( skit_stream *output, const void *ptr, const skit_slice text_if_null );

/**
These read an integer of the desired size and signed-ness from the stream.
(The signed-ness doesn't actually matter to this operation, but is provided 
to allow avoidance of casting.)
*/
uint64_t skit_stream_read_u64(skit_stream *stream);
uint32_t skit_stream_read_u32(skit_stream *stream);
uint16_t skit_stream_read_u16(skit_stream *stream);
uint8_t  skit_stream_read_u8 (skit_stream *stream);
int64_t  skit_stream_read_i64(skit_stream *stream);
int32_t  skit_stream_read_i32(skit_stream *stream);
int16_t  skit_stream_read_i16(skit_stream *stream);
int8_t   skit_stream_read_i8 (skit_stream *stream);

/**
These write an integer of the desired size and signed-ness to the 
end of the stream.
(The signed-ness doesn't actually matter to this operation, but is provided
to allow avoidance of casting.)
*/
void skit_stream_append_u64(skit_stream *stream, uint64_t val);
void skit_stream_append_u32(skit_stream *stream, uint32_t val);
void skit_stream_append_u16(skit_stream *stream, uint16_t val);
void skit_stream_append_u8 (skit_stream *stream, uint8_t val);
void skit_stream_append_i64(skit_stream *stream, int64_t val);
void skit_stream_append_i32(skit_stream *stream, int32_t val);
void skit_stream_append_i16(skit_stream *stream, int16_t val);
void skit_stream_append_i8 (skit_stream *stream, int8_t val);

/**
(final)
Reads the into the given stream until the regular expression is matched,
the regular expression rejects what is being read, or the end of the stream
is reached.  
If the expression fails to match before the end of the stream is reached,
then the string read up to that point will be placed into the return value.

TODO: there is no regex engine.  This can be used to read up to the next
null-terminated byte by passing sSLICE(".*\0") as the regex.  It is expressed
as a regex match to allow for future expansion of functionality without
deprecating special-cased functions later.
*/
skit_slice skit_stream_read_regex(skit_stream *stream, skit_loaf *buffer, skit_slice regex );

/* -------------------------- Useful internals ----------------------------- */

/**
This throws an exception with the given code and message.
Additionally, it will call the given stream's skit_stream_dump method and
  dump that information into the exception text.
This is the preferred way to throw exceptions with streams because of the
  superior debugging output it provides.
*/
void skit_stream_throw_exc( skit_err_code ecode, skit_stream *stream, const char *msg, ... );

/**
This is a function used internally by streams that use an internal buffer to
store line output when the caller doesn't provide such a buffer.
It allows such streams to quickly implement the typical 
skit_streamname_get_read_buffer function that is used to select which
buffer should be the destination of data retreived from the stream.
*/
skit_loaf *skit_stream_get_read_buffer( skit_loaf *internal_buf, skit_loaf *arg_buffer );

/**
This is a function used internally by streams that can read chunks of data
at a time but don't know when the end-of-stream is until it's hit.
It allows such streams to quickly implement the _slurp method.  All they need
to do is define the "data_source" function that describes how to pull some
bytes from the stream and place them into a buffer.

The return value for data_source should indicate the number of bytes read
from the stream and written into 'sink'.  A return value less than the
requested_chunk_size shall indicate an end-of-stream condition.
*/
skit_slice skit_stream_buffered_slurp(
	void *context,
	skit_loaf *buffer,
	size_t (*data_source)(void *context, void *sink, size_t requested_chunk_size)
	);

/* ------------------------- generic unittests ----------------------------- */

// The given stream has the contents "foo\n\n\0bar\nbaz\n"
void skit_stream_readln_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_READLN_UNITTEST_CONTENTS "foo\n\n\0bar\nbaz\n"

// The given stream has the contents "foobarbaz"
void skit_stream_read_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_READ_UNITTEST_CONTENTS "foobarbaz"

// The given stream has the contents "XYYdoodabcddcba"
void skit_stream_read_xNN_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_READ_XNN_UNITTEST_CONTENTS "XYYdoodabcddcba"

// The given stream has the contents "abc"
void skit_stream_read_fn_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_READ_FN_UNITTEST_CONTENTS "abc"

// The given stream has the contents ""
void skit_stream_appendln_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_APPENDLN_UNITTEST_CONTENTS ""

// The given stream has the contents ""
void skit_stream_appendf_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_APPENDF_UNITTEST_CONTENTS ""

// The given stream has the contents ""
void skit_stream_append_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_APPEND_UNITTEST_CONTENTS ""

// The given stream has the contents ""
void skit_stream_append_xNN_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_APPEND_XNN_UNITTEST_CONTENTS ""

// The given stream has the contents "".  The cursor starts at the begining.
void skit_stream_rewind_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_REWIND_UNITTEST_CONTENTS ""

void skit_stream_indent_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) );
#define SKIT_INDENT_UNITTEST_CONTENTS ""

#endif
