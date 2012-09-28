
#if defined(__DECC)
#pragma module skit_streams_pfile_stream
#endif

#include <stdarg.h>
#include <fcntl.h>
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/file_stream.h"
#include "survival_kit/streams/pfile_stream.h"

static int skit_pfile_stream_initialized = 0;
static skit_stream_vtable_t skit_pfile_stream_vtable;

void skit_pfile_stream_vtable_init(skit_stream_vtable_t *table)
{
	skit_stream_vtable_init(table);
	table->init          = &skit_pfile_stream_init;
	table->readln        = &skit_pfile_stream_readln;
	table->read          = &skit_pfile_stream_read;
	table->writeln       = &skit_pfile_stream_writeln;
	table->writefln_va   = &skit_pfile_stream_writefln_va;
	table->write         = &skit_pfile_stream_write;
	table->flush         = &skit_pfile_stream_flush;
	table->rewind        = &skit_pfile_stream_rewind;
	table->slurp         = &skit_pfile_stream_slurp;
	table->to_slice      = &skit_pfile_stream_to_slice;
	table->dump          = &skit_pfile_stream_dump;
	table->dtor          = &skit_pfile_stream_dtor;
	table->open          = &skit_pfile_stream_open;
	table->close         = &skit_pfile_stream_close;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_static_init()
{
	if ( skit_pfile_stream_initialized )
		return;
	
	skit_pfile_stream_initialized = 1;
	skit_pfile_stream_vtable_init(&skit_pfile_stream_vtable);
}

/* ------------------------------------------------------------------------- */

skit_pfile_stream *skit_pfile_stream_new()
{
	skit_pfile_stream *result = (skit_pfile_stream*)skit_malloc(sizeof(skit_pfile_stream));
	skit_pfile_stream_init(&result->as_stream);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_pfile_stream *skit_pfile_stream_downcast(skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_pfile_stream_vtable )
		return (skit_pfile_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_init(skit_stream *stream)
{
	skit_stream_init(stream);
	stream->meta.vtable_ptr = &skit_pfile_stream_vtable;
	stream->meta.class_name = sSLICE("skit_pfile_stream");
	
	skit_pfile_stream_internal *pstreami = &(skit_pfile_stream_downcast(stream)->as_internal);
	pstreami->name = skit_loaf_null();
	pstreami->err_msg_buf = skit_loaf_null();
	pstreami->file_descriptor = 0;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_readln(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_read(skit_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_writeln(skit_stream *stream, skit_slice line)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_writefln(skit_stream *stream, const char *fmtstr, ... )
{
	va_list vl;
	va_start(vl, fmtstr);
	skit_pfile_stream_writefln_va(stream, fmtstr, vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_writefln_va(skit_stream *stream, const char *fmtstr, va_list vl )
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_write(skit_stream *stream, skit_slice slice)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_flush(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_rewind(skit_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_slurp(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT_MSG(0, "Not implemented yet.");
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_to_slice(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	/* skit_pfile_stream_internal *pstreami = &(skit_pfile_stream_downcast(stream)->as_internal); */
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_dump(skit_stream *stream, skit_stream *output)
{
	sASSERT(output != NULL);
	
	if ( stream == NULL )
	{
		skit_stream_writeln(output, sSLICE("NULL skit_pfile_stream"));
		return;
	}
	
	skit_pfile_stream *pstream = skit_pfile_stream_downcast(stream);
	if ( pstream == NULL )
	{
		skit_stream_writeln(output, sSLICE("skit_stream (Error: invalid call to skit_pfile_stream_dump() with a first argument that isn't a pfile stream.)"));
		return;
	}
	
	skit_pfile_stream_internal *pstreami = &pstream->as_internal;
	if ( pstreami->name.chars == NULL )
	{
		skit_stream_writeln(output, sSLICE("Unopened skit_pfile_stream"));
		return;
	}
	
	skit_stream_writefln(output, "Opened skit_pfile_stream with the following properties:");
	skit_stream_writefln(output, "File name: %s\n", skit_loaf_as_cstr(pstreami->name));
	skit_stream_writefln(output, "File descriptor: %d\n", pstreami->file_descriptor);

	return;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_dtor(skit_stream *stream)
{
	/* 
	This shouldn't need to do anything.  Resources are cleaned up when close happens.
	TODO: It might be wise to leave around buffers after close happens, and then
	reuse them on open.  This could be an optimization in cases where file streams
	are repeatedly opened and closed to the same files.
	*/
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_open(skit_file_stream *stream, skit_slice fname, const char *access_mode)
{
	sASSERT_MSG(0, "Not implemented yet.");
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_close(skit_file_stream *stream)
{
	sASSERT_MSG(0, "Not implemented yet.");
}
