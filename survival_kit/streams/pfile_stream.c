
#if defined(__DECC)
#pragma module skit_streams_pfile_stream
#endif

#include <stdarg.h>
#include <stdio.h>
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/feature_emulation.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h"
#include "survival_kit/streams/file_stream.h"
#include "survival_kit/streams/pfile_stream.h"

#define SKIT_STREAM_T skit_pfile_stream
#define SKIT_VTABLE_T skit_stream_vtable_pfile
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

/* ------------------------------------------------------------------------- */

static skit_loaf *skit_pfile_get_read_buffer( skit_pfile_stream_internal *pstreami, skit_loaf *arg_buffer )
{
	return skit_stream_get_read_buffer(&(pstreami->read_buffer), arg_buffer);
}

/* ------------------------------------------------------------------------- */

static void skit_pfile_stream_newline(skit_pfile_stream_internal *pstreami)
{
	int errval = fputc( '\n', pstreami->file_handle );
	if ( errval != '\n' )
	{
		char errbuf[1024];
		skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, (skit_stream*)pstreami,
			skit_errno_to_cstr(errbuf, sizeof(errbuf)));
	}
}

/* ------------------------------------------------------------------------- */

static int skit_pfile_stream_initialized = 0;
static skit_stream_vtable_t skit_pfile_stream_vtable;

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_vtable_init(skit_stream_vtable_t *arg_table)
{
	skit_stream_vtable_init(arg_table);
	skit_stream_vtable_pfile *table = (skit_stream_vtable_pfile*)arg_table;
	table->readln        = &skit_pfile_stream_readln;
	table->read          = &skit_pfile_stream_read;
	table->read_fn       = &skit_pfile_stream_read_fn;
	table->appendln      = &skit_pfile_stream_appendln;
	table->appendf_va    = &skit_pfile_stream_appendf_va;
	table->append        = &skit_pfile_stream_append;
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
	skit_pfile_stream_init(result);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_pfile_stream *skit_pfile_stream_downcast(const skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_pfile_stream_vtable )
		return (skit_pfile_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_init(skit_pfile_stream *pstream)
{
	skit_stream *stream = &(pstream->as_stream);
	skit_stream_init(stream);
	stream->meta.vtable_ptr = &skit_pfile_stream_vtable;
	stream->meta.class_name = sSLICE("skit_pfile_stream");
	
	skit_pfile_stream_internal *pstreami = &(pstream->as_internal);
	pstreami->name = skit_loaf_null();
	pstreami->read_buffer = skit_loaf_null();
	pstreami->file_handle = NULL;
	pstreami->handle_owned_by_stream = 1;
}

/* ------------------------------------------------------------------------- */

static skit_pfile_stream *skit_pfile_stream_from_handle( FILE *file_handle, skit_slice name )
{
	skit_pfile_stream *result = skit_pfile_stream_new();
	skit_pfile_stream_internal *pstreami = &(result->as_internal);
	
	pstreami->name = skit_loaf_dup(name);
	pstreami->access_mode = skit_loaf_copy_cstr("rw");
	
	pstreami->file_handle = file_handle;
	
	pstreami->handle_owned_by_stream = 0;
	
	return result;
}

/* ------------------------------------------------------------------------- */

skit_pfile_stream *skit__pfile_stdout_cache = NULL;
skit_pfile_stream *skit__pfile_stdin_cache  = NULL;
skit_pfile_stream *skit__pfile_stderr_cache = NULL;

skit_pfile_stream *skit__pfile_stream_cached( FILE *file_handle, skit_pfile_stream **cached_stream, skit_slice name )
{
	if ( *cached_stream == NULL )
		*cached_stream = skit_pfile_stream_from_handle(file_handle, name);
	
	return *cached_stream;
}

/* ------------------------------------------------------------------------- */

/* This readln implementation uses the advice posted by "R.." in the following
article:
http://stackoverflow.com/questions/4411345/c-reading-a-text-file-with-variable-length-lines-line-by-line-using-fread-f
It should be able to handle lines with '\0' characters embedded in them, 
read no more characters from the file than exactly one line, and also
terminate correctly at the end-of-file.
*/
static void skit_try_fgets(
	skit_pfile_stream_internal *pstreami,
	skit_slice next_chunk,
	size_t *nbytes_read,
	int    *done )
{
	SKIT_USE_FEATURE_EMULATION;
	char *buf = (char*)sSPTR(next_chunk);
	size_t buf_len = sSLENGTH(next_chunk);
	*done = 0;
	
	/* Check for potentially dumb cases. */
	if ( buf_len < 1 )
	{
		*nbytes_read = 0;
		return; /* Force the caller to allocate more. */
	}
	
	/* Fill the chunk with newline characters. */
	memset(buf, '\n', buf_len); 
	
	/* Read data over it. */
	char *ret = fgets(buf, buf_len, pstreami->file_handle);
	
	/* Catch errors. */
	if ( ferror(pstreami->file_handle) )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, (skit_stream*)pstreami, skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
	
	/* fgets returns NULL when there is an error or when EOF occurs. */
	/* We already dealt with errors, so it must be an EOF. */
	/* This also indicates that "no characters have been read". */
	/* In this case the buffer is left untouched, so we can't differentiate */
	/*   what happens based on it's contents like the later code does, so */
	/*   it needs to be special-cased right here. */
	if ( ret == NULL )
	{
		*done = 1; /* This means we're done, one way or another. */
		*nbytes_read = 0;
		return;
	}
	
	/* Find the first newline. */
	char *ptr = memchr(buf, '\n', buf_len);
	
	if ( !ptr ) 
	{
		/* No newlines found: our buffer was completely overwritten, and is thus not large enough. */
		/* We will have filled the buffer /except/ for the '\0' character that fgets puts at the end. */
		*nbytes_read = buf_len - 1;
		return;
	}
	
	/* The remaining cases are: */
	/* end-of-line */
	/* end-of-file */
	/* Having placed a bunch of newlines in our buffer will help in */
	/*   in distinguishing between these possibilities, because fgets */
	/*   doesn't do anything helpful like returning a "number of bytes */
	/*   read" value. */
	if ( *(ptr+1) == '\0' )
	{
		/* Termination due to end-of-line. */
		sASSERT_EQ( *ptr, '\n', "%02x" );
		*done = 1;
		
		/* Don't return the last '\n'. */
		/* The '\n' is supposed to be absent per the stream interface. */
		*nbytes_read = ptr - buf;
		
		/* Be nice and nul-terminate things, just in case. */
		*ptr = '\0';
	}
	else
	{
		/* The newline was from our memset. */
		/* This means that end-of-file was reached. */
		
		*done = 1;
		
		/* There should be a '\0' placed before the '\n' by fgets. */
		/* This is the end-of-line for the last line in the file. */
		/* The '\0' isn't from the file, so we don't return it.  */
		/* Because we aren't return the '\n' from the memset or the '\0' from fgets, */
		/*   we'll subtract 1 from our final pointer. */
		sASSERT_EQ( *(ptr-1), '\0', "%02x" );
		*nbytes_read = ptr - buf - 1;
	}
	
	return;
}

skit_slice skit_pfile_stream_readln(skit_pfile_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	skit_loaf *read_buf;
	size_t buf_len;
	
	/* Return NULL slices when attempting to read from an already-exhausted stream. */
	if ( feof(pstreami->file_handle) )
		return skit_slice_null();
	
	/* Figure out which buffer to use. */
	read_buf = skit_pfile_get_read_buffer(pstreami, buffer);
	buf_len = sLLENGTH(*read_buf);
	
	/* Just say NO to improperly sized buffers. */
	if ( buf_len < 2 )
	{
		read_buf = skit_loaf_resize(read_buf, 16);
		buf_len = sLLENGTH(*read_buf);
	}
	
	/* Fill the buffer with one line. */
	skit_slice next_chunk = read_buf->as_slice;
	size_t nbytes_read = 0;
	size_t offset = 0;
	int done = 0;
	while ( 1 )
	{
		skit_try_fgets(pstreami, next_chunk, &nbytes_read, &done);
		if ( done )
			break; /* success! */
		
		/* Upsize the buffer: we need more. */
		buf_len = (buf_len * 3) / 2;
		read_buf = skit_loaf_resize(read_buf, buf_len);
		
		/* Advance the offset in our buffer to point to the new chunk. */
		offset += nbytes_read;
		next_chunk = skit_slice_of(read_buf->as_slice, offset, buf_len);
	}
	
	/* skit_try_fgets only knows about a potion of the entire buffer, and */
	/*   it returns how many bytes were written to that. */
	/* This is how we consider the rest of the buffer: */
	ssize_t line_length = offset + nbytes_read;
	
	/* The rest is trivial. */
	return skit_slice_of(read_buf->as_slice, 0, line_length);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_read(skit_pfile_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	skit_loaf *read_buf;
	
	/* Return NULL slices when attempting to read from an already-exhausted stream. */
	if ( feof(pstreami->file_handle) )
		return skit_slice_null();
	
	/* Figure out which buffer to use. */
	read_buf = skit_pfile_get_read_buffer(pstreami, buffer);
	
	/* The provided buffer might not be large enough. */
	/* Thankfully, the necessary size is easy to calculate in this case. */
	if ( sLLENGTH(*read_buf) < nbytes )
		skit_loaf_resize(read_buf, nbytes);
	
	/* Do the read. */
	size_t nbytes_read = fread( sLPTR(*read_buf), 1, nbytes, pstreami->file_handle );
	if ( ferror(pstreami->file_handle) )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
	
	return skit_slice_of(read_buf->as_slice, 0, nbytes_read);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_read_fn(skit_pfile_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ))
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	skit_custom_read_context ctx;
	skit_loaf *read_buf;
	
	/* Return NULL slices when attempting to read from an already-exhausted stream. */
	if ( feof(pstreami->file_handle) )
		return skit_slice_null();
	
	/* Figure out which buffer to use. */
	read_buf = skit_pfile_get_read_buffer(pstreami, buffer);
	ssize_t length = sLLENGTH(*read_buf);
	skit_utf8c *buf_ptr = sLPTR(*read_buf);
	ctx.caller_context = context; /* Pass the caller's context along. */
	
	/* Iterate and feed the caller 1 character at a time. */
	FILE *fhandle = pstreami->file_handle;
	ssize_t nbytes = 0;
	int done = 0;
	while ( !done )
	{
		/* Get the next character. */
		int c = fgetc(fhandle);
		
		/* EOF check. */
		if ( c == EOF )
			break;
		
		/* Upsize buffer as needed. */
		nbytes++;
		if ( length < nbytes )
		{
			/* Slow, but w/e. */
			skit_loaf_resize(read_buf, nbytes+64);
			length = sLLENGTH(*read_buf);
			buf_ptr = sLPTR(*read_buf);
		}
		
		/* Place the new character into the read buffer. */
		buf_ptr[nbytes-1] = c;
		
		/* Push the character to the caller. */
		ctx.current_char = c;
		ctx.current_slice = skit_slice_of(read_buf->as_slice, 0, nbytes);
		if ( !accept_char(&ctx) )
			break;
	}
	
	/* fgetc returns EOF to signal errors as well. */
	/* To distinguish that from legit EOFs, we need to check the ferror function. */
	if ( ferror(fhandle) )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
	
	return ctx.current_slice;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_appendln(skit_pfile_stream *stream, skit_slice line)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	/* The error handling is handled by subsequent calls. */
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	skit_pfile_stream_append(stream, line);
	sTRACE(skit_pfile_stream_newline(pstreami));
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_appendf(skit_pfile_stream *stream, const char *fmtstr, ... )
{
	va_list vl;
	va_start(vl, fmtstr);
	skit_pfile_stream_appendf_va(stream, fmtstr, vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_appendf_va(skit_pfile_stream *stream, const char *fmtstr, va_list vl )
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	if( pstreami->file_handle == NULL )
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), "Attempt to write to an unopened stream."));
	
	vfprintf( pstreami->file_handle, fmtstr, vl ); /* TODO: error handling? */
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_append(skit_pfile_stream *stream, skit_slice slice)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	if( pstreami->file_handle == NULL )
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), "Attempt to write to an unopened stream."));
	
	ssize_t length = sSLENGTH(slice);
	
	/* Optimize this case. */
	if ( length == 0 )
		return;
		/*sRETURN();*/
	
	/* Do the write. */
	fwrite( sSPTR(slice), 1, length, pstreami->file_handle );
	if ( ferror(pstreami->file_handle) )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_flush(skit_pfile_stream *stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	if( pstreami->file_handle == NULL )
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), "Attempt to flush an unopened stream."));
	
	int errval = fflush(pstreami->file_handle);
	if ( errval != 0 )
	{
		char errbuf[1024];
		/* TODO: throw exception for invalid streams when (errval == EBADF) */
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_rewind(skit_pfile_stream *stream)
{
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	rewind(pstreami->file_handle);
}

/* ------------------------------------------------------------------------- */

static size_t skit_pfile_slurp_source(void *context, void *sink, size_t requested_chunk_size)
{
	SKIT_USE_FEATURE_EMULATION;
	
	skit_pfile_stream *stream = context;
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);

	/* Read the next chunk of bytes from the file. */
	size_t nbytes_read = fread( sink, 1, requested_chunk_size, pstreami->file_handle );
	if ( ferror(pstreami->file_handle) )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
	
	return nbytes_read;
}

skit_slice skit_pfile_stream_slurp(skit_pfile_stream *stream, skit_loaf *buffer)
{
	SKIT_USE_FEATURE_EMULATION;
	
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	skit_loaf *read_buf;
	
	/* Figure out which buffer to use. */
	read_buf = skit_pfile_get_read_buffer(pstreami, buffer);
	
	/* Delegate the ugly stuff to the skit_stream_buffered_slurp function. */
	skit_slice result = sETRACE(skit_stream_buffered_slurp(stream, read_buf, &skit_pfile_slurp_source));
	return result;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_pfile_stream_to_slice(skit_pfile_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	/* skit_pfile_stream_internal *pstreami = &(skit_pfile_stream_downcast(stream)->as_internal); */
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_dump(const skit_pfile_stream *stream, skit_stream *output)
{
	if ( skit_stream_dump_null(output, stream, sSLICE("NULL skit_pfile_stream\n")) )
		return;
	
	/* Check for improperly cast streams.  Downcast will make sure we have the right vtable. */
	skit_pfile_stream *pstream = skit_pfile_stream_downcast(&(stream->as_stream));
	if ( pstream == NULL )
	{
		skit_stream_appendln(output, sSLICE("skit_stream (Error: invalid call to skit_pfile_stream_dump() with a first argument that isn't a pfile stream.)"));
		return;
	}
	
	skit_pfile_stream_internal *pstreami = &pstream->as_internal;
	if ( pstreami->file_handle == NULL )
	{
		skit_stream_appendln(output, sSLICE("Unopened skit_pfile_stream"));
		return;
	}
	
	skit_stream_appendf(output, "Opened skit_pfile_stream with the following properties:\n");
	skit_stream_appendf(output, "File name:    '%s'\n", skit_loaf_as_cstr(pstreami->name));
	skit_stream_appendf(output, "File handle:  %p\n", pstreami->file_handle);
	skit_stream_appendf(output, "Access:       %s\n", skit_loaf_as_cstr(pstreami->access_mode));
	skit_stream_appendf(output, "Handle owner: ");
	if ( pstreami->handle_owned_by_stream )
		skit_stream_appendf(output, "The stream owns its file handle.\n");
	else
		skit_stream_appendf(output, "The caller's FILE* pointer owns the handle.\n");

	return;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_dtor(skit_pfile_stream *stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	if ( pstreami->file_handle != NULL )
	{
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), 
			"Attempt to free resources from an open stream.\n"
			"Use skit_pfile_stream_close(stream) first!"));
	}
	
	if ( !skit_loaf_is_null(pstreami->name) )
		skit_loaf_free(&pstreami->name);
	
	if ( !skit_loaf_is_null(pstreami->read_buffer) )
		skit_loaf_free(&pstreami->read_buffer);
	
	if ( !skit_loaf_is_null(pstreami->access_mode) )
		skit_loaf_free(&pstreami->access_mode);
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_open(skit_pfile_stream *stream, skit_slice fname, const char *access_mode)
{
	SKIT_USE_FEATURE_EMULATION;
	char errbuf[1024];
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	
	if ( pstreami->file_handle != NULL )
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), "Already open to another file."));
	
	pstreami->name = skit_loaf_dup(fname);
	pstreami->access_mode = skit_loaf_copy_cstr(access_mode);
	
	const char *cfname = skit_loaf_as_cstr(pstreami->name);
	pstreami->file_handle = fopen(cfname, access_mode);
	if ( pstreami->file_handle == NULL )
	{
		/* TODO: generate different kinds of exceptions depending on what went wrong. */
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
	
	pstreami->handle_owned_by_stream = 1;
}

/* ------------------------------------------------------------------------- */

void skit_pfile_stream_close(skit_pfile_stream *stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	skit_pfile_stream_internal *pstreami = &(stream->as_internal);
	if( pstreami->file_handle == NULL )
		sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), "Attempt to close an unopened stream."));
	
	if ( pstreami->handle_owned_by_stream )
	{
		int errval = fclose(pstreami->file_handle);
		if ( errval != 0 )
		{
			char errbuf[1024];
			/* TODO: throw exception for invalid file handles when (errval == EBADF) */
			sTRACE(skit_stream_throw_exc(SKIT_FILE_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
		}
	}
	
	pstreami->file_handle = NULL;
}

/* ------------------------------------------------------------------------- */

#define SKIT_PFILE_UTEST_FILE "skit_pfile_unittest.txt"

typedef struct skit_pfile_utest_context skit_pfile_utest_context;
struct skit_pfile_utest_context
{
	skit_pfile_stream *under_test;
	skit_loaf *content_buffer;
};

static skit_slice skit_pfile_utest_contents( void *context )
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	skit_pfile_utest_context *ctx = context;
	
	/* Flush the stream under test: otherwise we might not see its writes. */
	skit_pfile_stream_flush(ctx->under_test);
	
	skit_pfile_stream pstream;
	skit_pfile_stream_init(&pstream);
	skit_pfile_stream_open(&pstream, sSLICE(SKIT_PFILE_UTEST_FILE), "r");
	sSCOPE_EXIT_BEGIN
		skit_pfile_stream_close(&pstream);
		skit_pfile_stream_dtor(&pstream);
	sEND_SCOPE_EXIT
	
	skit_slice file_contents = skit_pfile_stream_slurp(&pstream, ctx->content_buffer);
	
	sRETURN(file_contents);
sEND_SCOPE

static void skit_pfile_utest_prep_file(skit_slice contents)
{
	SKIT_USE_FEATURE_EMULATION;
	FILE *f = fopen(SKIT_PFILE_UTEST_FILE, "w");
	
	/* fputc is used instead of fputs because fputs will not be able to */
	/*  write '\0' null characters to a file.  This has upset unittests that */
	/*  expect to find a null character in the test data but find an */
	/*  end-of-file instead. */
	size_t i;
	skit_utf8c *ptr = sSPTR(contents);
	size_t length = sSLENGTH(contents);
	for ( i = 0; i < length; i++ )
		fputc(ptr[i], f);

	/* */
	if ( ferror(f) )
		sTHROW(SKIT_EXCEPTION,"Could not write to unittesting file: %s", SKIT_PFILE_UTEST_FILE);
	
	fclose(f);
}

static void skit_pfile_utest_rm()
{
	SKIT_USE_FEATURE_EMULATION;
	int errval = remove(SKIT_PFILE_UTEST_FILE);
	if ( errval != 0 )
		sTHROW(SKIT_EXCEPTION,"Could not delete unittesting file: %s", SKIT_PFILE_UTEST_FILE);
}

static void skit_pfile_run_utest(
	skit_pfile_stream *stream,
	skit_slice initial_file_contents,
	void (*utest_function)(
		skit_stream *stream,
		void *context,
		skit_slice (*get_stream_contents)(void *context) )
)
{
	skit_pfile_utest_context ctx;
	SKIT_LOAF_ON_STACK(content_buffer, 1024);
	ctx.content_buffer = &content_buffer;
	
	skit_pfile_utest_prep_file(initial_file_contents);
	skit_pfile_stream_open(stream, sSLICE(SKIT_PFILE_UTEST_FILE), "r+");
	ctx.under_test = stream;
	utest_function(&(stream->as_stream), &ctx, &skit_pfile_utest_contents);
	skit_pfile_stream_close(stream);
	skit_pfile_utest_rm();
	skit_loaf_free(&content_buffer);
}

void skit_pfile_stream_unittests()
{
	skit_pfile_stream pstream;
	printf("skit_pfile_stream_unittests()\n");
	
	skit_pfile_stream_init(&pstream);
	
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_READLN_UNITTEST_CONTENTS),     &skit_stream_readln_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_READ_UNITTEST_CONTENTS),       &skit_stream_read_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_READ_XNN_UNITTEST_CONTENTS),   &skit_stream_read_xNN_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_READ_FN_UNITTEST_CONTENTS),    &skit_stream_read_fn_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_APPENDLN_UNITTEST_CONTENTS),   &skit_stream_appendln_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_APPENDF_UNITTEST_CONTENTS),    &skit_stream_appendf_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_APPEND_UNITTEST_CONTENTS),     &skit_stream_append_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_APPEND_XNN_UNITTEST_CONTENTS), &skit_stream_append_xNN_unittest);
	skit_pfile_run_utest(&pstream, sSLICE(SKIT_REWIND_UNITTEST_CONTENTS),     &skit_stream_rewind_unittest);
	
	skit_pfile_stream_dtor(&pstream);

	/* TODO: It would be nice if there was some way to test this automatically.  For now, this will at least make sure it doesn't crash. */
	skit_stream_appendln(&skit_pfile_stream_stdout->as_stream, sSLICE("  skit_pfile_stream_stdout test passed."));
	
	printf("  skit_pfile_stream_unittests passed!\n");
	printf("\n");
}
