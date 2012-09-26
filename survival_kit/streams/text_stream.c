
#if defined(__DECC)
#pragma module skit_streams_text_stream
#endif

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
	tstreami->buffer = skit_loaf_null();
	tstreami->text = skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_readln(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_read(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_writeln(skit_stream *stream, skit_slice line)
{
	sASSERT_MSG(0, "Not implemented yet.");
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
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_write(skit_stream *stream, skit_slice slice)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_flush(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_rewind(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_slurp(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_text_stream_to_slice(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

void skit_text_stream_dump(skit_stream *stream, skit_stream *output)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */
