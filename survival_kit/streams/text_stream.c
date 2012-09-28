
#if defined(__DECC)
#pragma module skit_streams_text_stream
#endif

#include <stdio.h>
#include <stdarg.h>
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h"

static int skit_text_stream_initialized = 0;
static skit_stream_vtable_t skit_text_stream_vtable;

void skit_text_stream_vtable_init(skit_stream_vtable_t *table)
{
	skit_stream_vtable_init(table);
	table->init          = &skit_text_stream_init;
	table->readln        = &skit_text_stream_readln;
	table->read          = &skit_text_stream_read;
	table->writeln       = &skit_text_stream_writeln;
	table->writefln_va   = &skit_text_stream_writefln_va;
	table->write         = &skit_text_stream_write;
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
	skit_text_stream *result = (skit_text_stream*)skit_malloc(sizeof(skit_text_stream));
	skit_text_stream_init(&result->as_stream);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_text_stream *skit_text_stream_downcast(skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_text_stream_vtable )
		return (skit_text_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_init(skit_stream *stream)
{
	skit_stream_init(stream);
	stream->meta.vtable_ptr = &skit_text_stream_vtable;
	stream->meta.class_name = sSLICE("skit_text_stream");
	
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	tstreami->buffer = skit_loaf_alloc(16);
	tstreami->text = skit_slice_of(tstreami->buffer.as_slice, 0, 0);
	tstreami->cursor = 0;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_readln(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	/* We don't care what the caller passed for buffer.  All of our strings
	are in memory anyways, so we can just pass out slices of those. */
	
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	size_t cursor = tstreami->cursor; /* Create a fast, local copy of the cursor. */
	skit_slice text = tstreami->text; /* Ditto for the text slice. */
	ssize_t length = sSLENGTH(text);  /* Ditto for length.  It won't be changing. */
	
	/* Return null when reading past the end of the stream. */
	if ( cursor == length )
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
	cursor += nl_size; /* note: nl_size will be 0 if we reached the end of the buffer, so this addition works it both cases. */
	tstreami->cursor = cursor; /* Since we used a local variable for cursor, we need to update the original. */
	
	return skit_slice_of( text, line_begin, line_end );
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_read(skit_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	sASSERT(stream != NULL);
	
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	size_t cursor = tstreami->cursor; /* Create a fast, local copy of the cursor. */
	skit_slice text = tstreami->text; /* Ditto for the text slice. */
	ssize_t length = sSLENGTH(text);  /* Ditto for length.  It won't be changing. */
	
	/* Return null when reading past the end of the stream. */
	if ( cursor == length )
		return skit_slice_null();
	
	size_t block_begin = cursor;
	size_t block_end = block_begin + nbytes;
	if ( length < block_end )
		block_end = length;
	
	return skit_slice_of( text, block_begin, block_end );
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_writeln(skit_stream *stream, skit_slice line)
{
	sASSERT(stream != NULL);
	sASSERT(line.chars != NULL);
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	
	skit_slice_buffered_append(&tstreami->buffer, &tstreami->text, line);
	skit_slice_buffered_append(&tstreami->buffer, &tstreami->text, sSLICE("\n"));
	tstreami->cursor += sSLENGTH(line) + 1;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_writefln(skit_stream *stream, const char *fmtstr, ...)
{
	va_list vl;
	va_start(vl, fmtstr);
	skit_text_stream_writefln_va(stream, fmtstr, vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_writefln_va(skit_stream *stream, const char *fmtstr, va_list vl)
{
	const size_t buf_size = 1024;
	char buffer[buf_size];
	size_t nchars_printed = 0;
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	sASSERT_MSG(tstreami->cursor == sSLENGTH(tstreami->text), "Insert of text into the interior of text streams is currently not implemented.  Only appends are allowed.");
	
	nchars_printed = vsnprintf(buffer, buf_size, fmtstr, vl);
	skit_slice_buffered_append(&tstreami->buffer, &tstreami->text, skit_slice_of_cstrn(buffer, nchars_printed));
	tstreami->cursor += nchars_printed;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_write(skit_stream *stream, skit_slice slice)
{
	sASSERT(stream != NULL);
	sASSERT(slice.chars != NULL);
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	sASSERT_MSG(tstreami->cursor == sSLENGTH(tstreami->text), "Insert of text into the interior of text streams is currently not implemented.  Only appends are allowed.");
	
	skit_slice_buffered_append(&tstreami->buffer, &tstreami->text, slice);
	tstreami->cursor += sSLENGTH(slice);
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_flush(skit_stream *stream)
{
	/* Text streams never need flushing. */
	/* This is only for things like disk/network/terminal I/O. */
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_rewind(skit_stream *stream)
{
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	tstreami->cursor = 0;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_slurp(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	return tstreami->text;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_to_slice(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_dump(skit_stream *stream, skit_stream *output)
{
	sASSERT(output != NULL);
	
	if ( stream == NULL )
	{
		skit_stream_writeln(output, sSLICE("NULL skit_text_stream"));
		return;
	}
	
	skit_text_stream *tstream = skit_text_stream_downcast(stream);
	if ( tstream == NULL )
	{
		skit_stream_writeln(output, sSLICE("skit_stream (Error: invalid call to skit_text_stream_dump() with a first argument that isn't a pfile stream.)"));
		return;
	}
	
	skit_text_stream_internal *tstreami = &tstream->as_internal;
	if ( tstreami->buffer.chars == NULL )
	{
		skit_stream_writeln(output, sSLICE("Invalid skit_text_stream with NULL buffer."));
		return;
	}
	
	skit_stream_writefln(output, "skit_text_stream with the following properties:");
	skit_stream_writefln(output, "Capacity: %d\n", skit_loaf_len(tstreami->buffer));
	skit_stream_writefln(output, "Length:   %d\n", skit_slice_len(tstreami->text));
	skit_stream_write(output, sSLICE("First 60 chars: '"));
	skit_stream_write(output, skit_slice_of(tstreami->text, 0, SKIT_MIN(60, sSLENGTH(tstreami->text))));
	skit_stream_writeln(output, sSLICE("'"));
	return;
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_dtor(skit_stream *stream)
{
	skit_text_stream_internal *tstreami = &(skit_text_stream_downcast(stream)->as_internal);
	
	tstreami->buffer = *skit_loaf_free(&tstreami->buffer);
	tstreami->text = skit_slice_null();
}

/* ------------------------------------------------------------------------- */
