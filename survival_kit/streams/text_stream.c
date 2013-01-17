
#if defined(__DECC)
#pragma module skit_streams_text_stream
#endif

#include <stdio.h>
#include <stdarg.h>
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h"

#define SKIT_STREAM_T skit_text_stream
#define SKIT_VTABLE_T skit_stream_vtable_text
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

static int skit_text_stream_initialized = 0;
static skit_stream_vtable_t skit_text_stream_vtable;

void skit_text_stream_vtable_init(skit_stream_vtable_t *arg_table)
{
	skit_stream_vtable_init(arg_table);
	skit_stream_vtable_text *table = (skit_stream_vtable_text*)arg_table;
	table->readln        = &skit_text_stream_readln;
	table->read          = &skit_text_stream_read;
	table->read_fn       = &skit_text_stream_read_fn;
	table->appendln      = &skit_text_stream_appendln;
	table->appendf_va    = &skit_text_stream_appendf_va;
	table->append        = &skit_text_stream_append;
	table->flush         = &skit_text_stream_flush;
	table->rewind        = &skit_text_stream_rewind;
	table->slurp         = &skit_text_stream_slurp;
	table->to_slice      = &skit_text_stream_to_slice;
	table->dump          = &skit_text_stream_dump;
	table->dtor          = &skit_text_stream_dtor;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_static_init()
{
	if ( skit_text_stream_initialized )
		return;

	skit_text_stream_initialized = 1;
	skit_text_stream_vtable_init(&skit_text_stream_vtable);
}

/* ------------------------------------------------------------------------- */

skit_text_stream *skit_text_stream_new()
{
	skit_text_stream *result = skit_malloc(sizeof(skit_text_stream));
	skit_text_stream_init(result);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_text_stream *skit_text_stream_downcast(const skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_text_stream_vtable )
		return (skit_text_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_init(skit_text_stream *tstream)
{
	skit_stream *stream = &tstream->as_stream;
	skit_stream_init(stream);
	stream->meta.vtable_ptr = &skit_text_stream_vtable;
	stream->meta.class_name = sSLICE("skit_text_stream");
	
	skit_text_stream_internal *tstreami = &tstream->as_internal;
	tstreami->buffer = skit_loaf_alloc(16);
	tstreami->text = skit_slice_of(tstreami->buffer.as_slice, 0, 0);
	tstreami->cursor = 0;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_init_str(skit_text_stream *tstream, skit_slice slice)
{
	skit_stream *stream = &tstream->as_stream;
	skit_stream_init(stream);
	stream->meta.vtable_ptr = &skit_text_stream_vtable;
	stream->meta.class_name = sSLICE("skit_text_stream");
	
	skit_text_stream_internal *tstreami = &tstream->as_internal;
	tstreami->buffer = skit_loaf_dup(slice);
	tstreami->text = skit_slice_of(tstreami->buffer.as_slice, 0, sSLENGTH(slice));
	tstreami->cursor = 0;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_readln(skit_text_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	/* We don't care what the caller passed for buffer.  All of our strings
	are in memory anyways, so we can just pass out slices of those. */
	
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	size_t cursor = tstreami->cursor; /* Create a fast, local copy of the cursor. */
	skit_slice text = tstreami->text; /* Ditto for the text slice. */
	ssize_t length = sSLENGTH(text);  /* Ditto for length.  It won't be changing. */
	
	/* Return null when reading past the end of the stream. */
	if ( cursor > length )
		return skit_slice_null();
	
	/* cursor isn't past the end of stream, so grab the next line. */
	ssize_t line_begin = cursor;
	int nl_size = 0;
	while ( cursor < length )
	{
		nl_size = skit_slice_match_nl(text,cursor);
		if ( nl_size > 0 )
			break;
		cursor++;
	}
	ssize_t line_end = cursor;
	
	
	if ( cursor == length )
	{
		/* This should only happen if we were already positioned at the end of */
		/*   the stream.  Such can happen if the stream had a '\n' character at */
		/*   the very end that causes one last empty line to be read.  If this */
		/*   statement is true, then a previous call already read that empty line */
		/*   and cursor==length means that we are /past/ the end-of-stream now. */
		tstreami->cursor = length+1;
	}
	else
	{
		/* Skip the newline.  We just handled it. */
		cursor += nl_size;
		
		/* Since we used a local variable for cursor, we need to update the original. */
		tstreami->cursor = cursor;
	}
	
	return skit_slice_of( text, line_begin, line_end );
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_read(skit_text_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	sASSERT(stream != NULL);
	
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	size_t cursor = tstreami->cursor; /* Create a fast, local copy of the cursor. */
	skit_slice text = tstreami->text; /* Ditto for the text slice. */
	ssize_t length = sSLENGTH(text);  /* Ditto for length.  It won't be changing. */
	
	/* Return null when reading past the end of the stream. */
	if ( cursor >= length )
		return skit_slice_null();
	
	size_t block_begin = cursor;
	size_t block_end = block_begin + nbytes;
	
	if ( length < block_end )
	{
		/* Saturate at the end bounds. */
		block_end = length;
		
		/* We've read /past/ the end of the stream.  Demonstrate this by */
		/*   tossing the cursor over the end. */
		tstreami->cursor = block_end+1;
	}
	else
	{
		/* Normal case: update the cursor. */
		tstreami->cursor = block_end;
	}
	
	return skit_slice_of( text, block_begin, block_end );
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_read_fn(skit_text_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ))
{
	skit_custom_read_context ctx;
	
	sASSERT(stream != NULL);
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	
	size_t block_begin = tstreami->cursor;
	size_t block_end = block_begin;
	skit_slice text = tstreami->text;
	ssize_t length = sSLENGTH(text);
	
	/* Return null when reading past the end of the stream. */
	if ( block_begin >= length )
		return skit_slice_null();
	
	skit_utf8c *text_ptr = sSPTR(text);
	ctx.caller_context = context; /* Pass the caller's context along. */
	
	int done = 0;
	while ( !done )
	{
		if ( block_end >= length )
			break;

		ctx.current_char = text_ptr[block_end];
		block_end++;
		ctx.current_slice = skit_slice_of(text, block_begin, block_end);
		if ( !accept_char( &ctx ) )
			break;
	}
	
	tstreami->cursor = block_end;
	
	return ctx.current_slice;
}

/* ------------------------------------------------------------------------- */

static void skit_tstream_internal_append(skit_text_stream_internal *tstreami, skit_slice text)
{
	skit_slice_buffered_append(&tstreami->buffer, &tstreami->text, text);
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_appendln(skit_text_stream *stream, skit_slice line)
{
	sASSERT(stream != NULL);
	sASSERT(sSPTR(line) != NULL);
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	
	skit_tstream_internal_append(tstreami, line);
	skit_tstream_internal_append(tstreami, sSLICE("\n"));
	
	tstreami->cursor += sSLENGTH(tstreami->text) + 1;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_appendf(skit_text_stream *stream, const char *fmtstr, ...)
{
	va_list vl;
	va_start(vl, fmtstr);
	skit_text_stream_appendf_va(stream, fmtstr, vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_appendf_va(skit_text_stream *stream, const char *fmtstr, va_list vl)
{
	const size_t buf_size = 1024;
	char buffer[buf_size];
	size_t nchars_printed = 0;
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	
	nchars_printed = vsnprintf(buffer, buf_size, fmtstr, vl);
	skit_tstream_internal_append(tstreami, skit_slice_of_cstrn(buffer, nchars_printed));
	
	tstreami->cursor = sSLENGTH(tstreami->text) + 1;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_append(skit_text_stream *stream, skit_slice slice)
{
	sASSERT(stream != NULL);
	sASSERT(sSPTR(slice) != NULL);
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	
	skit_tstream_internal_append(tstreami, slice);
	
	tstreami->cursor = sSLENGTH(tstreami->text) + 1;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_flush(skit_text_stream *stream)
{
	/* Text streams never need flushing. */
	/* This is only for things like disk/network/terminal I/O. */
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_rewind(skit_text_stream *stream)
{
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	tstreami->cursor = 0;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_slurp(skit_text_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	tstreami->cursor = sSLENGTH(tstreami->text);
	return tstreami->text;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_to_slice(skit_text_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_dump(const skit_text_stream *stream, skit_stream *output)
{
	if ( skit_stream_dump_null(output, stream, sSLICE("NULL skit_text_stream\n")) )
		return;
	
	/* Check for improperly cast streams.  Downcast will make sure we have the right vtable. */
	skit_text_stream *tstream = skit_text_stream_downcast(&(stream->as_stream));
	if ( tstream == NULL )
	{
		skit_stream_appendln(output, sSLICE("skit_stream (Error: invalid call to skit_text_stream_dump() with a first argument that isn't a text stream.)"));
		return;
	}
	
	skit_text_stream_internal *tstreami = &tstream->as_internal;
	if ( sLPTR(tstreami->buffer) == NULL )
	{
		skit_stream_appendln(output, sSLICE("Invalid skit_text_stream with NULL buffer."));
		return;
	}
	
	skit_stream_appendf(output, "skit_text_stream with the following properties:\n");
	skit_stream_appendf(output, "Capacity: %d\n", skit_loaf_len(tstreami->buffer));
	skit_stream_appendf(output, "Length:   %d\n", skit_slice_len(tstreami->text));
	skit_stream_append(output, sSLICE("First 60 chars: '"));
	skit_stream_append(output, skit_slice_of(tstreami->text, 0, SKIT_MIN(60, sSLENGTH(tstreami->text))));
	skit_stream_appendln(output, sSLICE("'"));
	return;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_dtor(skit_text_stream *stream)
{
	skit_text_stream_internal *tstreami = &(stream->as_internal);
	
	tstreami->buffer = skit_loaf_free(&tstreami->buffer);
	tstreami->text = skit_slice_null();
}

/* ------------------------------------------------------------------------- */

static skit_slice skit_text_stream_utest_contents( void *context )
{
	skit_text_stream *stream = context;
	skit_text_stream_internal *tstreami = &stream->as_internal;
	
	return tstreami->text;
}

static void skit_text_stream_run_utest(
	void (*utest_function)(
		skit_stream *stream,
		void *context,
		skit_slice (*get_stream_contents)(void *context) ),
	skit_slice initial_contents
)
{
	skit_text_stream tstream;
	skit_stream *stream = &tstream.as_stream;
	skit_text_stream_init_str(&tstream, initial_contents);
	
	utest_function(stream, stream, &skit_text_stream_utest_contents);
	
	skit_text_stream_dtor(&tstream);
}

void skit_text_stream_unittests()
{
	printf("skit_text_stream_unittests()\n");
	
	skit_text_stream_run_utest(&skit_stream_readln_unittest,     sSLICE(SKIT_READLN_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_read_unittest,       sSLICE(SKIT_READ_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_read_xNN_unittest,   sSLICE(SKIT_READ_XNN_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_read_fn_unittest,    sSLICE(SKIT_READ_FN_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_appendln_unittest,   sSLICE(SKIT_APPENDLN_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_appendf_unittest,    sSLICE(SKIT_APPENDF_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_append_unittest,     sSLICE(SKIT_APPEND_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_append_xNN_unittest, sSLICE(SKIT_APPEND_XNN_UNITTEST_CONTENTS));
	skit_text_stream_run_utest(&skit_stream_rewind_unittest,     sSLICE(SKIT_REWIND_UNITTEST_CONTENTS));
	
	printf("  skit_text_stream_unittests passed!\n");
	printf("\n");
}
