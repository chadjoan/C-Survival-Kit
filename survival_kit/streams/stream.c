
#if defined(__DECC)
#pragma module skit_streams_stream
#endif

#include <stdarg.h>
#include <inttypes.h>

#include "survival_kit/feature_emulation.h"

#include "survival_kit/regex.h"
#include "survival_kit/streams/meta.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h" /* Used in skit_stream_throw_exc(...) */

#define SKIT_STREAM_T skit_stream
#define SKIT_VTABLE_T skit_stream_vtable_base
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

skit_err_code SKIT_EOS_EXCEPTION;

/* ------------------------- vtable stuff ---------------------------------- */
static int skit_stream_initialized = 0;
static skit_stream_vtable_t skit_stream_vtable;

static void skit_stream_func_not_implc(const skit_stream* stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sTHROWC((void*)stream, SKIT_NOT_IMPLEMENTED,
		"Attempt to call a virtual function on an "
		"instance of the abstract class skit_stream (or skit_file_stream).");
}

static void skit_stream_func_not_impl(skit_stream* stream)
{
	skit_stream_func_not_implc(stream);
}

static skit_slice skit_stream_read_not_impl(skit_stream* stream, skit_loaf *buffer)
{
	skit_stream_func_not_impl(stream);
	return skit_slice_null();
}

static skit_slice skit_stream_readn_not_impl(skit_stream* stream, skit_loaf *buffer, size_t nbytes )
{
	skit_stream_func_not_impl(stream);
	return skit_slice_null();
}

static skit_slice skit_stream_read_fn_not_impl(skit_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)(skit_custom_read_context *ctx))
{
	skit_stream_func_not_impl(stream);
	return skit_slice_null();
}

static void skit_stream_append_not_impl(skit_stream *stream, skit_slice slice)
{
	skit_stream_func_not_impl(stream);
}

static void skit_stream_appendva_not_impl(skit_stream *stream, const char *fmtstr, va_list vl)
{
	skit_stream_func_not_impl(stream);
}

static void skit_stream_dump_not_impl(const skit_stream *stream, skit_stream* output)
{
	skit_stream_func_not_implc(stream);
}

void skit_stream_incr_indent_base(skit_stream *stream)
{
	/* Do nothing. */
}

void skit_stream_decr_indent_base(skit_stream *stream)
{
	/* Do nothing. */
}

int16_t skit_stream_get_ind_lvl_base(const skit_stream *stream)
{
	/* Streams that don't implement indentation will always have an indentation level of 0. */
	return 0;
}

int16_t skit_stream_get_peak_base(const skit_stream *stream)
{
	/* Streams that don't implement indentation will always have an indentation level of 0. */
	return 0;
}

const char *skit_stream_get_ind_str_base(const skit_stream *stream)
{
	/* Streams that don't implement indentation will always have a blank indentation string. */
	return "";
}

void skit_stream_set_ind_str_base(skit_stream *stream, const char *c)
{
	/* Do nothing. */
}

union skit_file_stream;
static void skit_file_stream_open_not_impl(union skit_stream *stream, skit_slice fname, const char *mode)
{
	skit_stream_func_not_impl((skit_stream*)stream);
}

static void skit_file_stream_close_not_impl(union skit_stream *stream)
{
	skit_stream_func_not_impl((skit_stream*)stream);
}

void skit_stream_vtable_init(skit_stream_vtable_t *arg_table)
{
	skit_stream_vtable_base *table = (skit_stream_vtable_base*)arg_table;
	table->readln        = &skit_stream_read_not_impl;
	table->read          = &skit_stream_readn_not_impl;
	table->read_fn       = &skit_stream_read_fn_not_impl;
	table->appendln      = &skit_stream_append_not_impl;
	table->appendf_va    = &skit_stream_appendva_not_impl;
	table->append        = &skit_stream_append_not_impl;
	table->flush         = &skit_stream_func_not_impl;
	table->rewind        = &skit_stream_func_not_impl;
	table->slurp         = &skit_stream_read_not_impl;
	table->to_slice      = &skit_stream_read_not_impl;
	table->dump          = &skit_stream_dump_not_impl;
	table->dtor          = &skit_stream_func_not_impl;
	
	table->incr_indent   = &skit_stream_incr_indent_base;
	table->decr_indent   = &skit_stream_decr_indent_base;
	table->get_ind_lvl   = &skit_stream_get_ind_lvl_base;
	table->get_peak      = &skit_stream_get_peak_base;
	table->get_ind_str   = &skit_stream_get_ind_str_base;
	table->set_ind_str   = &skit_stream_set_ind_str_base;
	
	table->open          = &skit_file_stream_open_not_impl;
	table->close         = &skit_file_stream_close_not_impl;
}

void skit_stream_module_init()
{
	if ( skit_stream_initialized )
		return;
	
	skit_stream_initialized = 1;
	skit_stream_vtable_init(&skit_stream_vtable);

	SKIT_REGISTER_EXCEPTION(SKIT_EOS_EXCEPTION, SKIT_IO_EXCEPTION, "End Of Stream exception.");
}

/* ------------------------ virtual methods -------------------------------- */

void skit_stream_ctor(skit_stream *stream)
{
	skit_stream_internal *streami = &stream->as_internal;
	streami->meta.vtable_ptr = &skit_stream_vtable;
	streami->meta.class_name = sSLICE("skit_stream");
}

skit_slice skit_stream_readln(skit_stream *stream, skit_loaf *buffer)
{
	return SKIT_STREAM_DISPATCH(stream, readln, buffer);
}

skit_slice skit_stream_read(skit_stream *stream, skit_loaf *buffer, size_t nbytes )
{
	return SKIT_STREAM_DISPATCH(stream, read, buffer, nbytes);
}

skit_slice skit_stream_read_fn(skit_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ))
{
	return SKIT_STREAM_DISPATCH(stream, read_fn, buffer, context, accept_char);
}

void skit_stream_appendln(skit_stream *stream, skit_slice line)
{
	SKIT_STREAM_DISPATCH(stream, appendln, line);
}

void skit_stream_appendf(skit_stream *stream, const char* fmtstr, ...)
{
	va_list vl;
	va_start(vl, fmtstr);
	SKIT_STREAM_DISPATCH(stream, appendf_va, fmtstr, vl);
	va_end(vl);
}

void skit_stream_appendf_va(skit_stream *stream, const char* fmtstr, va_list vl)
{
	SKIT_STREAM_DISPATCH(stream, appendf_va, fmtstr, vl);
}

void skit_stream_append(skit_stream *stream, skit_slice slice)
{
	SKIT_STREAM_DISPATCH(stream, append, slice);
}

void skit_stream_flush(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, flush);
}

void skit_stream_rewind(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, rewind);
}

skit_slice skit_stream_slurp(skit_stream *stream, skit_loaf *buffer)
{
	return SKIT_STREAM_DISPATCH(stream, slurp, buffer);
}

skit_slice skit_stream_to_slice(skit_stream *stream, skit_loaf *buffer)
{
	return SKIT_STREAM_DISPATCH(stream, to_slice, buffer);
}

void skit_stream_dump(const skit_stream *stream, skit_stream *out)
{
	SKIT_STREAM_DISPATCH(stream, dump, out);
}

void skit_stream_dtor(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, dtor);
}

void skit_stream_incr_indent(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, incr_indent);
}

void skit_stream_decr_indent(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, decr_indent);
}

int16_t skit_stream_get_ind_lvl(const skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, get_ind_lvl);
}

int16_t skit_stream_get_peak(const skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, get_peak);
}

const char *skit_stream_get_ind_str(const skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, get_ind_str);
}

void skit_stream_set_ind_str(skit_stream *stream, const char *c)
{
	SKIT_STREAM_DISPATCH(stream, set_ind_str, c);
}

void skit_stream_free(skit_stream *stream)
{
	skit_stream_dtor(stream);
	skit_free(stream);
}

/* Streams can't be opened/closed in general, so those methods are absent. */

/* -------------------------- final methods -------------------------------- */

int skit_stream_dump_null( skit_stream *output, const void *ptr, const skit_slice text_if_null )
{
	sASSERT(output != NULL);
	
	if ( ptr == NULL )
	{
		skit_stream_append(output, text_if_null);
		return 1;
	}
	
	return 0;
}

/* ------------------------------------------------------------------------- */

/* This is probably a slow way to do it. */
/* But it's convenient, and there isn't a need for speed right now. */
/* TODO: BUG: end of stream conditions! */
/* NOTE: Currently, throwing is better than crashing. */
#define read_xNN_impl(T) \
	SKIT_USE_FEATURE_EMULATION; \
	SKIT_LOAF_ON_STACK(buffer, sizeof(T)); \
	skit_slice ret = skit_stream_read(stream, &buffer, sizeof(T)); \
	if ( skit_slice_is_null(ret) ) \
		sTHROWC(stream, SKIT_EOS_EXCEPTION, "End of stream reached."); \
	T result = *(T*)sSPTR(ret); \
	skit_loaf_free(&buffer); \
	return result;

/**
These read an integer of the desired size and signed-ness from the stream.
(The signed-ness doesn't actually matter to this operation, but is provided 
to allow avoidance of casting.)
*/
uint64_t skit_stream_read_u64(skit_stream *stream) { read_xNN_impl(uint64_t) }
uint32_t skit_stream_read_u32(skit_stream *stream) { read_xNN_impl(uint32_t) }
uint16_t skit_stream_read_u16(skit_stream *stream) { read_xNN_impl(uint16_t) }
uint8_t  skit_stream_read_u8 (skit_stream *stream) { read_xNN_impl(uint8_t) }
int64_t  skit_stream_read_i64(skit_stream *stream) { read_xNN_impl(int64_t) }
int32_t  skit_stream_read_i32(skit_stream *stream) { read_xNN_impl(int32_t) }
int16_t  skit_stream_read_i16(skit_stream *stream) { read_xNN_impl(int16_t) }
int8_t   skit_stream_read_i8 (skit_stream *stream) { read_xNN_impl(int8_t) }

#define write_xNN_impl(T) \
	T* buf = &val; \
	skit_slice outgoing = skit_slice_of_cstrn((char*)buf, sizeof(val)); \
	skit_stream_append(stream, outgoing);

/**
These write an integer of the desired size and signed-ness to the 
end of the stream.
(The signed-ness doesn't actually matter to this operation, but is provided
to allow avoidance of casting.)
*/
void skit_stream_append_u64(skit_stream *stream, uint64_t val) { write_xNN_impl(uint64_t) }
void skit_stream_append_u32(skit_stream *stream, uint32_t val) { write_xNN_impl(uint32_t) }
void skit_stream_append_u16(skit_stream *stream, uint16_t val) { write_xNN_impl(uint16_t) }
void skit_stream_append_u8 (skit_stream *stream, uint8_t val)  { write_xNN_impl(uint8_t ) }
void skit_stream_append_i64(skit_stream *stream, int64_t val)  { write_xNN_impl(int64_t ) }
void skit_stream_append_i32(skit_stream *stream, int32_t val)  { write_xNN_impl(int32_t ) }
void skit_stream_append_i16(skit_stream *stream, int16_t val)  { write_xNN_impl(int16_t ) }
void skit_stream_append_i8 (skit_stream *stream, int8_t val)   { write_xNN_impl(int8_t  ) }

/* ------------------------------------------------------------------------- */

skit_slice skit_stream_read_regex(skit_stream *stream, skit_loaf *buffer, skit_slice regex )
{
	SKIT_USE_FEATURE_EMULATION;
	sTHROWC(stream, SKIT_IO_EXCEPTION, "skit_stream_read_regex is not implemented.");
	return skit_slice_null();
}

/* --------------------- Useful non-virtual stuff -------------------------- */

void skit_stream_throw_exc( skit_err_code ecode, skit_stream *stream, const char *msg, ... )
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	skit_text_stream err_stream;
	skit_text_stream_ctor(&err_stream);
	va_list vl;
	
	/* Prevent memory leaks. */
	/* sSCOPE_EXIT is awesome and allows us to do this despite using this */
	/*   very string in the sTHROWC expression that leaves this function. */
	/* sTHROWC does specify that it copies its string argument, so there is */
	/*   no need to keep our copy of the string data around while the stack */
	/*   unwinds. */
	sSCOPE_EXIT(skit_text_stream_dtor(&err_stream));
	
	/* Mention what we are. */
	skit_slice class_name = stream->meta.class_name;
	skit_text_stream_appendln( &err_stream, sSLICE("") );
	skit_text_stream_appendf( &err_stream, "%.*s error:\n", sSLENGTH(class_name), sSPTR(class_name) );
	
	/* Dump the message. It's probably something from errno. */
	va_start(vl, msg);
	skit_text_stream_appendf_va( &err_stream, msg, vl );
	skit_text_stream_appendln( &err_stream, sSLICE("") );
	va_end(vl);
	
	/* Dump our file stream's info.  This makes it easier to track what went wrong and where/why. */
	skit_text_stream_appendf( &err_stream, "\n" );
	skit_text_stream_appendf( &err_stream, "Stream metadata at time of error:\n" );
	skit_stream_dump( stream, &(err_stream.as_stream) );
	
	/* Convert the text stream into a string that we can feed into the exception. */
	skit_text_stream_rewind( &err_stream );
	skit_slice errtxt = skit_text_stream_slurp( &err_stream, NULL );
	
	/* Pass our message into the exception, being sure to handle two things right: */
	/* 1. Prevent any formatter expansions in errtxt. */
	/* 2. The slice is not necessarily null-terminated, so use the %.*s specifier and length-bound it. */
	sTHROWC( stream, ecode, "%.*s", sSLENGTH(errtxt), sSPTR(errtxt) );
sEND_SCOPE

/* ------------------------------------------------------------------------- */

skit_loaf *skit_stream_get_read_buffer( skit_loaf *internal_buf, skit_loaf *arg_buffer )
{
	skit_loaf *buffer_to_use;
	if ( arg_buffer == NULL )
	{
		if ( skit_loaf_is_null(*internal_buf) )
			*internal_buf = skit_loaf_alloc(16);
		
		buffer_to_use = internal_buf;
	}
	else
		buffer_to_use = arg_buffer;
	
	return buffer_to_use;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_stream_buffered_slurp(
	void *context,
	skit_loaf *buffer,
	size_t (*data_source)(void *context, void *sink, size_t requested_chunk_size)
	)
{
	SKIT_USE_FEATURE_EMULATION;
	const size_t default_chunk_size = 1024; // This number is completely unoptimized.
	sASSERT(context != NULL);

	// If buffer is NULL, then we have no way to transfer ownership of the
	// loaf to the caller, and any memory allocated will almost certainly be
	// leaked.  Thus, the caller MUST provide us with a pointer to something
	// that can reference any memory we allocate.
	// (Streams can accept NULL buffer arguments because they have a way to
	// buffer the memory internally.  This function does not have such a
	// context.  Typically, the stream's internal buffer will be pointed to
	// by the buffer argument, if the stream's caller didn't already provide
	// their own buffer to use instead.)
	sASSERT(buffer != NULL);

	// It IS OK if the caller didn't allocate any memory.  We will just have
	// to make some ourselves, in this case.  The caller is already responsible
	// for any buffer allocations in this function.
	if ( skit_loaf_is_null(*buffer) )
		*buffer = skit_loaf_alloc(default_chunk_size);
	
	// Make sure the buffer used is large enough.
	// If it is too small, then the resizes by %150 will truncate to be no
	// larger than the size before the multiplication by %150,
	//   Ex: (1 * 3) / 2 -> 3 / 2 -> integer(1.5) -> 1
	// and that would prevent the buffer from ever growing.  We pick something
	// that will grow reasonably fast, if needed.  8 is very conservative, but
	// should work.
	ssize_t buf_length = sLLENGTH(*buffer);
	if ( buf_length < 8 )
	{
		skit_loaf_resize(buffer, default_chunk_size);
		buf_length = sLLENGTH(*buffer);
	}
	
	// Do the read.
	size_t offset = 0;
	size_t chunk_length = buf_length;
	size_t nbytes_read = 0;
	while ( 1 )
	{
		// Read the next chunk of bytes from the source.
		nbytes_read = sETRACE2(data_source(context, sLPTR(*buffer) + offset, chunk_length));

		// ...
		offset += nbytes_read;

		// Check for EOF.
		if ( nbytes_read < chunk_length )
			break;
	
		// Grow by 1.5 each time, and prep the extra 0.5 for filling.
		skit_loaf_resize(buffer, (buf_length * 3) / 2);
		buf_length = sLLENGTH(*buffer);

		chunk_length = buf_length - offset;
	}

	size_t slurp_length = offset;
	
	// done.
	return skit_slice_of(buffer->as_slice, 0, slurp_length);
}

/* ------------------------- generic unittests ----------------------------- */

#define SKIT_ASSERT_CONTENTS(cstr) \
	do { \
		sASSERT_EQS(get_stream_contents(context, strlen(cstr)), sSLICE(cstr)); \
	} while (0)

// The given stream has the contents "foo\n\n\0bar\nbaz\n"
void skit_stream_readln_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	skit_loaf buf = skit_loaf_alloc(3);
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("foo"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE(""));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("\0bar"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("baz"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE(""));
	sASSERT_EQS(skit_stream_readln(stream,&buf), skit_slice_null());
	skit_loaf_free(&buf);
	printf("  skit_stream_readln_unittest passed.\n");
}

// The given stream has the contents "foobarbaz"
void skit_stream_read_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	skit_loaf buf = skit_loaf_alloc(3);
	sASSERT_EQS(skit_stream_read(stream,&buf,3), sSLICE("foo"));
	sASSERT_EQS(skit_stream_read(stream,&buf,0), sSLICE(""));
	sASSERT_EQS(skit_stream_read(stream,&buf,3), sSLICE("bar"));
	sASSERT_EQS(skit_stream_read(stream,&buf,4), sSLICE("baz"));
	sASSERT_EQS(skit_stream_read(stream,&buf,3), skit_slice_null());
	skit_loaf_free(&buf);
	printf("  skit_stream_read_unittest passed.\n");
}

// The given stream has the contents "XYYdoodabcddcba"
void skit_stream_read_xNN_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	sASSERT_EQ_HEX( skit_stream_read_i8(stream), 'X');
	
	int16_t i16res = skit_stream_read_i16(stream);
	skit_slice i16slice = skit_slice_of_cstrn( (char*)&i16res, 2 );
	sASSERT_EQS( i16slice, sSLICE("YY") );
	
	int32_t i32res = skit_stream_read_i32(stream);
	skit_slice i32slice = skit_slice_of_cstrn( (char*)&i32res, 4 );
	sASSERT_EQS( i32slice, sSLICE("dood") );
	
	int64_t i64res = skit_stream_read_i64(stream);
	skit_slice i64slice = skit_slice_of_cstrn( (char*)&i64res, 8 );
	sASSERT_EQS( i64slice, sSLICE("abcddcba") );
	
	printf("  skit_stream_read_xNN_unittest passed.\n");
}

static int skit_stream_read_fn_accept1( skit_custom_read_context *ctx )
{
	int *count = ctx->caller_context;
	if ( *count == 0 )
	{
		(*count)++;
		sASSERT_EQ_HEX(ctx->current_char, 'a');
		sASSERT_EQS(ctx->current_slice, sSLICE("a"));
		return 1;
	}
	else if ( *count == 1 )
	{
		(*count)++;
		sASSERT_EQ_HEX(ctx->current_char, 'b');
		sASSERT_EQS(ctx->current_slice, sSLICE("ab"));
		return 0;
	}
	else
	{
		sASSERT(0);
	}
	return 0;
}

static int skit_stream_read_fn_accept2( skit_custom_read_context *ctx )
{
	sASSERT_EQ_HEX(ctx->current_char, 'c');
	sASSERT_EQS(ctx->current_slice, sSLICE("c"));
	return 1;
}

// The given stream has the contents "abc"
void skit_stream_read_fn_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	int count = 0;
	skit_slice result;
	result = skit_stream_read_fn( stream, NULL, &count, &skit_stream_read_fn_accept1 );
	sASSERT_EQS(result, sSLICE("ab"));
	result = skit_stream_read_fn( stream, NULL, &count, &skit_stream_read_fn_accept2 );
	sASSERT_EQS(result, sSLICE("c"));
	
	printf("  skit_stream_read_fn_unittest passed.\n");
}

// The given stream has the contents ""
void skit_stream_appendln_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	SKIT_ASSERT_CONTENTS("");
	skit_stream_appendln(stream, sSLICE("foo"));
	SKIT_ASSERT_CONTENTS("foo\n");
	skit_stream_appendln(stream, sSLICE(""));
	SKIT_ASSERT_CONTENTS("foo\n\n");
	skit_stream_appendln(stream, sSLICE("bar"));
	SKIT_ASSERT_CONTENTS("foo\n\nbar\n");
	skit_stream_appendln(stream, sSLICE("baz"));
	SKIT_ASSERT_CONTENTS("foo\n\nbar\nbaz\n");
	printf("  skit_stream_appendln_unittest passed.\n");
}

// The given stream has the contents ""
void skit_stream_appendf_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	SKIT_ASSERT_CONTENTS("");
	skit_stream_appendf(stream, "foo\n");
	SKIT_ASSERT_CONTENTS("foo\n");
	skit_stream_appendf(stream, "%s\n", "bar");
	SKIT_ASSERT_CONTENTS("foo\nbar\n");
	skit_stream_appendf(stream, "%d\n", 3);
	SKIT_ASSERT_CONTENTS("foo\nbar\n3\n");
	printf("  skit_stream_appendf_unittest passed.\n");
}

// The given stream has the contents ""
void skit_stream_append_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	SKIT_ASSERT_CONTENTS("");
	skit_stream_append(stream, sSLICE("foo"));
	SKIT_ASSERT_CONTENTS("foo");
	skit_stream_append(stream, sSLICE(""));
	SKIT_ASSERT_CONTENTS("foo");
	skit_stream_append(stream, sSLICE("bar"));
	SKIT_ASSERT_CONTENTS("foobar");
	skit_stream_append(stream, sSLICE("baz"));
	SKIT_ASSERT_CONTENTS("foobarbaz");
	printf("  skit_stream_append_unittest passed.\n");
}

// The given stream has the contents ""
void skit_stream_append_xNN_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	SKIT_ASSERT_CONTENTS("");
	skit_stream_append_i8(stream, 'X');
	SKIT_ASSERT_CONTENTS("X");
	skit_stream_append_i16(stream, *(int16_t*)"YY");
	SKIT_ASSERT_CONTENTS("XYY");
	skit_stream_append_i32(stream, *(int32_t*)"dood");
	SKIT_ASSERT_CONTENTS("XYYdood");
	skit_stream_append_i64(stream, *(int64_t*)"abcddbca");
	SKIT_ASSERT_CONTENTS("XYYdoodabcddbca");
	printf("  skit_stream_append_xNN_unittest passed.\n");
}

// The given stream has the contents "".  The cursor starts at the begining.
void skit_stream_rewind_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	skit_loaf buf = skit_loaf_alloc(64);
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE(""));
	skit_stream_appendln(stream, sSLICE("foo"));
	sASSERT_EQS(skit_stream_readln(stream,&buf), skit_slice_null());
	skit_stream_rewind(stream);
	sASSERT_EQS(skit_stream_readln(stream,&buf), sSLICE("foo"));
	skit_loaf_free(&buf);
	printf("  skit_stream_rewind_unittest passed.\n");
}

// The given stream has the contents ""
void skit_stream_indent_unittest(
	skit_stream *stream,
	void *context,
	skit_slice (*get_stream_contents)(void *context, int expected_size) )
{
	sASSERT_EQ( skit_stream_get_ind_lvl(stream), 0 );
	sASSERT_EQ( skit_stream_get_peak(stream),    0 );
	sASSERT_EQS( skit_slice_of_cstr(skit_stream_get_ind_str(stream)), sSLICE("\t") );
	SKIT_ASSERT_CONTENTS("");
	skit_stream_appendf(stream, "foo\n");
	SKIT_ASSERT_CONTENTS("foo\n");
	
	skit_stream_incr_indent(stream);
	sASSERT_EQ( skit_stream_get_ind_lvl(stream), 1 );
	sASSERT_EQ( skit_stream_get_peak(stream),    1 );
	skit_stream_append(stream, sSLICE("bar\nbaz"));
	SKIT_ASSERT_CONTENTS("foo\n\tbar\n\tbaz");
	
	skit_stream_incr_indent(stream);
	skit_stream_set_ind_str(stream, "  ");
	sASSERT_EQ( skit_stream_get_ind_lvl(stream), 2 );
	sASSERT_EQ( skit_stream_get_peak(stream),    2 );
	sASSERT_EQS( skit_slice_of_cstr(skit_stream_get_ind_str(stream)), sSLICE("  ") );
	skit_stream_appendln(stream, sSLICE("\nqux"));
	SKIT_ASSERT_CONTENTS("foo\n\tbar\n\tbaz\n    qux\n");
	
	skit_stream_decr_indent(stream);
	skit_stream_decr_indent(stream);
	sASSERT_EQ( skit_stream_get_ind_lvl(stream), 0 );
	sASSERT_EQ( skit_stream_get_peak(stream),    2 );
	skit_stream_append(stream, sSLICE("quux"));
	SKIT_ASSERT_CONTENTS("foo\n\tbar\n\tbaz\n    qux\nquux");
	
	skit_stream_incr_indent(stream);
	sASSERT_EQ( skit_stream_get_ind_lvl(stream), 1 );
	sASSERT_EQ( skit_stream_get_peak(stream),    1 );
	
	printf("  skit_stream_indent_unittest passed.\n");
}
