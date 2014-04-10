
#if defined(__DECC)
#pragma module skit_streams_empty_stream
#endif

#include <stdio.h>
#include <stdarg.h>
#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/empty_stream.h"
#include "survival_kit/streams/ind_stream.h"

#define SKIT_STREAM_T skit_empty_stream
#define SKIT_VTABLE_T skit_stream_vtable_empty
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

static int skit_empty_stream_initialized = 0;
static skit_stream_vtable_t skit_empty_stream_vtable;

void skit_empty_stream_vtable_init(skit_stream_vtable_t *arg_table)
{
	skit_stream_vtable_init(arg_table);
	skit_stream_vtable_empty *table = (skit_stream_vtable_empty*)arg_table;
	table->readln        = &skit_empty_stream_readln;
	table->read          = &skit_empty_stream_read;
	table->read_fn       = &skit_empty_stream_read_fn;
	table->appendln      = &skit_empty_stream_appendln;
	table->appendf_va    = &skit_empty_stream_appendf_va;
	table->append        = &skit_empty_stream_append;
	table->flush         = &skit_empty_stream_flush;
	table->rewind        = &skit_empty_stream_rewind;
	table->slurp         = &skit_empty_stream_slurp;
	table->to_slice      = &skit_empty_stream_to_slice;
	table->dump          = &skit_empty_stream_dump;
	table->dtor          = &skit_empty_stream_dtor;
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_module_init()
{
	if ( skit_empty_stream_initialized )
		return;

	skit_empty_stream_initialized = 1;
	skit_empty_stream_vtable_init(&skit_empty_stream_vtable);
}

/* ------------------------------------------------------------------------- */

static skit_empty_stream skit__empty_stream_static_alloc;
static skit_empty_stream *skit__empty_stream_cache = NULL;

skit_empty_stream *skit__empty_stream_cached()
{
	if ( skit__empty_stream_cache == NULL )
	{
		skit__empty_stream_cache = &skit__empty_stream_static_alloc;
		skit_empty_stream_ctor(skit__empty_stream_cache);
	}
	
	return skit__empty_stream_cache;
}

skit_ind_stream *skit__ind_empty_cache = NULL;

/* ------------------------------------------------------------------------- */

skit_empty_stream *skit_empty_stream_new()
{
	skit_empty_stream *result = skit_malloc(sizeof(skit_empty_stream));
	skit_empty_stream_ctor(result);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_empty_stream *skit_empty_stream_downcast(const skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_empty_stream_vtable )
		return (skit_empty_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_ctor(skit_empty_stream *estream)
{
	skit_stream *stream = &estream->as_stream;
	skit_stream_ctor(stream);
	stream->meta.vtable_ptr = &skit_empty_stream_vtable;
	stream->meta.class_name = sSLICE("skit_empty_stream");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_empty_stream_readln(skit_empty_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return skit_slice_null(); // The empty stream is ALWAYS past the end.
}

/* ------------------------------------------------------------------------- */

skit_slice skit_empty_stream_read(skit_empty_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	sASSERT(stream != NULL);
	return skit_slice_null(); // The empty stream is ALWAYS past the end.
}

/* ------------------------------------------------------------------------- */

skit_slice skit_empty_stream_read_fn(skit_empty_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ))
{
	sASSERT(stream != NULL);
	return skit_slice_null(); // The empty stream is ALWAYS past the end.
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_appendln(skit_empty_stream *stream, skit_slice line)
{
	sASSERT(stream != NULL);
	sASSERT(!skit_slice_is_null(line));
	// No-op: the empty stream is ALWAYS empty, so appending text to it does nothing.
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_appendf(skit_empty_stream *stream, const char *fmtstr, ...)
{
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	// No-op: the empty stream is ALWAYS empty, so appending text to it does nothing.
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_appendf_va(skit_empty_stream *stream, const char *fmtstr, va_list vl)
{
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	// No-op: the empty stream is ALWAYS empty, so appending text to it does nothing.
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_append(skit_empty_stream *stream, skit_slice slice)
{
	sASSERT(stream != NULL);
	sASSERT(sSPTR(slice) != NULL);
	// No-op: the empty stream is ALWAYS empty, so appending text to it does nothing.
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_flush(skit_empty_stream *stream)
{
	/* Empty streams never need flushing. */
	/* This is only for things like disk/network/terminal I/O. */
	sASSERT(stream != NULL);
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_rewind(skit_empty_stream *stream)
{
	sASSERT(stream != NULL);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_empty_stream_slurp(skit_empty_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return sSLICE("");
}

/* ------------------------------------------------------------------------- */

skit_slice skit_empty_stream_to_slice(skit_empty_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_dump(const skit_empty_stream *stream, skit_stream *output)
{
	if ( skit_stream_dump_null(output, stream, sSLICE("NULL skit_empty_stream\n")) )
		return;
	
	/* Check for improperly cast streams.  Downcast will make sure we have the right vtable. */
	skit_empty_stream *estream = skit_empty_stream_downcast(&(stream->as_stream));
	if ( estream == NULL )
	{
		skit_stream_appendln(output, sSLICE("skit_stream (Error: invalid call to skit_empty_stream_dump() with a first argument that isn't a text stream.)"));
		return;
	}
	
	skit_stream_appendf(output, "skit_empty_stream (This stream is ALWAYS empty, regardless of what is written to it.)\n");
	return;
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_dtor(skit_empty_stream *stream)
{
	sASSERT(stream != NULL);
}

/* ------------------------------------------------------------------------- */

void skit_empty_stream_unittests()
{
	printf("skit_empty_stream_unittests()\n");

	skit_stream *stream = skit_empty_stream_instance;

	sASSERT_EQS(skit_stream_readln(stream, NULL), skit_slice_null());
	sASSERT_EQS(skit_stream_read(stream,NULL,1), skit_slice_null());

	skit_stream_appendln(stream, sSLICE("a"));
	
	sASSERT_EQS(skit_stream_readln(stream, NULL), skit_slice_null());
	sASSERT_EQS(skit_stream_read(stream,NULL,1), skit_slice_null());

	skit_stream_appendf(stream, "At line %d in function %s.\n", __LINE__, __func__);
	
	sASSERT_EQS(skit_stream_readln(stream, NULL), skit_slice_null());
	sASSERT_EQS(skit_stream_read(stream,NULL,1), skit_slice_null());

	skit_stream_append(stream, sSLICE("a"));
	
	sASSERT_EQS(skit_stream_readln(stream, NULL), skit_slice_null());
	sASSERT_EQS(skit_stream_read(stream,NULL,1), skit_slice_null());

	sASSERT_EQS(skit_stream_slurp(stream,NULL), sSLICE(""));

	printf("  skit_empty_stream_unittests passed!\n");
	printf("\n");
}

