
#if defined(__DECC)
#pragma module skit_streams_pfile_stream
#endif

#include <stdarg.h>
#include <stdio.h>
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
	pstreami->line_buffer = skit_loaf_null();
	pstreami->err_msg_buf = skit_loaf_null();
	pstreami->file_handle = NULL;
	pstreami->eof_reached = 0;
}

/* ------------------------------------------------------------------------- */

/* This readln implementation uses the advice posted by "R.." in the following
article:
http://stackoverflow.com/questions/4411345/c-reading-a-text-file-with-variable-length-lines-line-by-line-using-fread-f
It should be able to handle lines with '\0' characters embedded in them, 
read no more characters from the file than exactly one line, and also
terminate correctly at the end-of-file.
*/
static ssize_t skit_try_fgets(skit_pfile_stream_internal *pstreami, skit_slice next_chunk)
{
	char *buf = (char*)sSPTR(next_chunk);
	size_t buf_len = sSLENGTH(next_chunk);
	memset(buf, '\n', buf_len); /* Fill the chunk with newline characters. */
	fgets(buf, buf_len, pstreami->file_handle); /* Read data over it. */
	char *ptr = memchr(buf, '\n', buf_len); /* Find the first newline. */
	
	if ( !ptr ) /* No newlines found: our buffer was completely overwritten, and is thus not large enough. */
		return -1;
	
	if ( *(ptr+1) == '\0' )
	{
		/* Termination due to end-of-line. */
		sASSERT_EQ( *ptr, '\n', "%c" );
		
		/* Don't return the last '\n'. */
		/* The '\n' is supposed to be absent per the stream interface. */
		return ptr - buf - 1; 
	}
	else
	{
		/* Termination due to end-of-file. */
		pstreami->eof_reached = 1;
		
		/* The newline was from our memset. */
		/* There should be a '\0' placed before it by fgets.  This is the end-of-line. */
		/* The '\0' isn't from the file, so we don't return it.  */
		/* Because we aren't return the '\n' from the memset or the '\0' from fgets, */
		/*   we'll subtract 2 from our final pointer. */
		sASSERT_EQ( *(ptr-1), '\0', "%c" );
		return ptr - buf - 2;
	}
	
	sASSERT(0);
	return 0;
}

skit_slice skit_pfile_stream_readln(skit_stream *stream, skit_loaf *buffer)
{
	sASSERT_MSG(0, "Not implemented yet.");
	skit_pfile_stream_internal *pstreami = &(skit_pfile_stream_downcast(stream)->as_internal);
	skit_loaf *line_buf;
	
	/* Figure out which buffer to use. */
	if ( buffer == NULL )
	{
		if ( skit_loaf_is_null(pstreami->line_buffer) )
			pstreami->line_buffer = skit_loaf_alloc(16);
	}
	
	/* Fill the buffer with one line. */
	skit_slice next_chunk = line_buf->as_slice;
	size_t buf_len = sLLENGTH(*line_buf);
	ssize_t nbytes_to_end = -1;
	while ( 1 )
	{
		nbytes_to_end = skit_try_fgets(pstreami, next_chunk);
		if ( nbytes_to_end >= 0 )
			break; /* success! */
		
		size_t new_buf_len = (buf_len * 3) / 2;
		line_buf = skit_loaf_resize(line_buf, new_buf_len);
		next_chunk = skit_slice_of(line_buf->as_slice, buf_len, new_buf_len);
		buf_len = new_buf_len;
	}
	
	/* skit_try_fgets knows how many bytes were read in only a portion of */
	/* the entire buffer.  This is how we add in the rest of the buffer: */
	ssize_t line_length = (sSPTR(next_chunk) + nbytes_to_end) - sLPTR(*line_buf);
	
	/* The rest is trivial. */
	return skit_slice_of(line_buf->as_slice, 0, line_length);
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
	if ( sLPTR(pstreami->name) == NULL )
	{
		skit_stream_writeln(output, sSLICE("Unopened skit_pfile_stream"));
		return;
	}
	
	skit_stream_writefln(output, "Opened skit_pfile_stream with the following properties:");
	skit_stream_writefln(output, "File name: %s\n", skit_loaf_as_cstr(pstreami->name));
	skit_stream_writefln(output, "File handle: %p\n", pstreami->file_handle);
	skit_stream_writefln(output, "EOF reached: %d\n", pstreami->eof_reached);

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
