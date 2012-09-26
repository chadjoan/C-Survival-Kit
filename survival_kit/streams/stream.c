
#include "survival_kit/streams/meta.h"
#include "survival_kit/streams/stream.h"

/* ------------------------- vtable stuff ---------------------------------- */
static int skit_stream_initialized = 0;
static skit_stream_vtable_t skit_stream_vtable;

static void skit_stream_func_not_impl(skit_stream* stream)
{
	sTHROW(SKIT_NOT_IMPLEMENTED,
		"Attempt to call a virtual function on an "
		"instance of the abstract class skit_stream (or skit_file_stream).");
}

static skit_slice skit_stream_read_not_impl(skit_stream* stream)
{
	skit_stream_func_not_impl(stream);
	return skit_slice_null();
}

static void skit_stream_write_not_impl(skit_stream *stream, skit_slice slice)
{
	skit_stream_func_not_impl(stream);
}

static void skit_stream_dump_not_impl(skit_stream* stream, skit_stream* output)
{
	skit_stream_func_not_impl(stream);
}

static void skit_file_stream_open_not_impl(skit_file_stream* stream, skit_slice fname, const char *mode)
{
	skit_stream_func_not_impl((skit_stream*)stream);
}

static void skit_file_stream_close_not_impl(skit_file_stream* stream)
{
	skit_stream_func_not_impl((skit_stream*)stream);
}

void skit_stream_vtable_init(skit_stream_vtable_t *table)
{
	table->init          = &skit_stream_func_not_impl;
	table->read_line     = &skit_stream_read_not_impl;
	table->read_bytes    = &skit_stream_read_not_impl;
	table->write_line    = &skit_stream_write_not_impl;
	table->write_bytes   = &skit_stream_write_not_impl;
	table->flush         = &skit_stream_func_not_impl;
	table->rewind        = &skit_stream_func_not_impl;
	table->slurp         = &skit_stream_read_not_impl;
	table->to_slice      = &skit_stream_read_not_impl;
	table->dump          = &skit_stream_dump_not_impl;
	table->open          = &skit_file_stream_open_not_impl;
	table->close         = &skit_file_stream_close_not_impl;
}

void skit_stream_static_init()
{
	if ( skit_stream_initialized )
		return;
	
	skit_stream_initialized = 1;
	skit_stream_vtable_init(skit_stream_vtable);
	skit_stream_register_vtable(skit_stream_vtable);
}

/* ------------------------ virtual methods -------------------------------- */

/* skit_stream does not have an init method because it is an abstract class. */

skit_slice skit_stream_read_line(skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, read_line);
}

skit_slice skit_stream_read_bytes(skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, read_bytes);
}

void skit_stream_write_line(skit_stream *stream, skit_slice slice)
{
	SKIT_STREAM_DISPATCH(stream, write_line, slice);
}

void skit_stream_write_bytes(skit_stream *stream, skit_slice slice)
{
	SKIT_STREAM_DISPATCH(stream, write_bytes, slice);
}

void skit_stream_flush(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, flush);
}

void skit_stream_rewind(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, rewind);
}

skit_slice skit_stream_slurp(skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, slurp);
}

skit_slice skit_stream_to_slice(skit_stream *stream)
{
	return SKIT_STREAM_DISPATCH(stream, to_slice);
}

void skit_stream_dump(skit_stream *stream, skit_stream *out)
{
	return SKIT_STREAM_DISPATCH(stream, dump, out);
}

/* Streams can't be opened/closed in general, so those methods are absent. */

/* -------------------------- final methods -------------------------------- */

void skit_stream_incr_indent(skit_stream *stream)
{
}

void skit_stream_decr_indent(skit_stream *stream)
{
}

short skit_stream_get_indent_lvl(skit_stream *stream)
{
}

char skit_stream_get_indent_char(skit_stream *stream)
{
}

void skit_stream_set_indent_char(skit_stream *stream, char c)
{
}

