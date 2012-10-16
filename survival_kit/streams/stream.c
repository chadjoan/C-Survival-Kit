
#if defined(__DECC)
#pragma module skit_streams_stream
#endif

#include <stdarg.h>

#include "survival_kit/feature_emulation.h"

#include "survival_kit/streams/meta.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h" /* Used in skit_stream_throw_exc(...) */

#define SKIT_STREAM_T skit_stream
#define SKIT_VTABLE_T skit_stream_vtable_base
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

/* ------------------------- vtable stuff ---------------------------------- */
static int skit_stream_initialized = 0;
static skit_stream_vtable_t skit_stream_vtable;

static void skit_stream_func_not_impl(skit_stream* stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sTHROW(SKIT_NOT_IMPLEMENTED,
		"Attempt to call a virtual function on an "
		"instance of the abstract class skit_stream (or skit_file_stream).");
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

static void skit_stream_append_not_impl(skit_stream *stream, skit_slice slice)
{
	skit_stream_func_not_impl(stream);
}

static void skit_stream_appendva_not_impl(skit_stream *stream, const char *fmtstr, va_list vl)
{
	skit_stream_func_not_impl(stream);
}

static void skit_stream_dump_not_impl(skit_stream *stream, skit_stream* output)
{
	skit_stream_func_not_impl(stream);
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
	table->appendln       = &skit_stream_append_not_impl;
	table->appendfln_va   = &skit_stream_appendva_not_impl;
	table->append         = &skit_stream_append_not_impl;
	table->flush         = &skit_stream_func_not_impl;
	table->rewind        = &skit_stream_func_not_impl;
	table->slurp         = &skit_stream_read_not_impl;
	table->to_slice      = &skit_stream_read_not_impl;
	table->dump          = &skit_stream_dump_not_impl;
	table->dtor          = &skit_stream_func_not_impl;
	table->open          = &skit_file_stream_open_not_impl;
	table->close         = &skit_file_stream_close_not_impl;
}

void skit_stream_static_init()
{
	if ( skit_stream_initialized )
		return;
	
	skit_stream_initialized = 1;
	skit_stream_vtable_init(&skit_stream_vtable);
}

/* ------------------------ virtual methods -------------------------------- */

void skit_stream_init(skit_stream *stream)
{
	skit_stream_internal *streami = &stream->as_internal;
	streami->meta.vtable_ptr = &skit_stream_vtable;
	streami->meta.class_name = sSLICE("skit_stream");
	streami->common_fields.indent_str = "\t";
	streami->common_fields.indent_level = 0;
}

skit_slice skit_stream_readln(skit_stream *stream, skit_loaf *buffer)
{
	return SKIT_STREAM_DISPATCH(stream, readln, buffer);
}

skit_slice skit_stream_read(skit_stream *stream, skit_loaf *buffer, size_t nbytes )
{
	return SKIT_STREAM_DISPATCH(stream, read, buffer, nbytes);
}

void skit_stream_appendln(skit_stream *stream, skit_slice line)
{
	SKIT_STREAM_DISPATCH(stream, appendln, line);
}

void skit_stream_appendfln(skit_stream *stream, const char* fmtstr, ...)
{
	va_list vl;
	va_start(vl, fmtstr);
	SKIT_STREAM_DISPATCH(stream, appendfln_va, fmtstr, vl);
	va_end(vl);
}

void skit_stream_appendfln_va(skit_stream *stream, const char* fmtstr, va_list vl)
{
	SKIT_STREAM_DISPATCH(stream, appendfln_va, fmtstr, vl);
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

void skit_stream_dump(skit_stream *stream, skit_stream *out)
{
	SKIT_STREAM_DISPATCH(stream, dump, out);
}

void skit_stream_dtor(skit_stream *stream)
{
	SKIT_STREAM_DISPATCH(stream, dtor);
}

void skit_stream_delete(skit_stream *stream)
{
	skit_stream_dtor(stream);
	skit_free(stream);
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
	return 0;
}

const char *skit_stream_get_indent_str(skit_stream *stream)
{
	return "\t";
}

void skit_stream_set_indent_str(skit_stream *stream, const char *c)
{
}

/* --------------------- Useful non-virtual stuff -------------------------- */

void skit_stream_throw_exc( skit_err_code ecode, skit_stream *stream, const char *msg, ... )
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	skit_text_stream err_stream;
	skit_text_stream_init(&err_stream);
	va_list vl;
	
	/* Prevent memory leaks. */
	/* sSCOPE_EXIT is awesome and allows us to do this despite using this */
	/*   very string in the sTHROW expression that leaves this function. */
	/* sTHROW does specify that it copies its string argument, so there is */
	/*   no need to keep our copy of the string data around while the stack */
	/*   unwinds. */
	sSCOPE_EXIT(skit_text_stream_dtor(&err_stream));
	
	/* Mention what we are. */
	skit_text_stream_appendfln( &err_stream, "" );
	skit_text_stream_appendfln( &err_stream, "%s error:", sSPTR(stream->meta.class_name) ); /* HACK: using sSPTR to return c string for class_name slice.  The class_name slice might not be nul-terminated.  (It should be at the time of writing.) */
	
	/* Dump the message. It's probably something from errno. */
	va_start(vl, msg);
	skit_text_stream_appendfln_va( &err_stream, msg, vl );
	va_end(vl);
	
	/* Dump our file stream's info.  This makes it easier to track what went wrong and where/why. */
	skit_text_stream_appendfln( &err_stream, "" );
	skit_text_stream_appendfln( &err_stream, "Stream metadata at time of error:" );
	skit_stream_dump( stream, &(err_stream.as_stream) );
	
	/* Convert the text stream into a string that we can feed into the exception. */
	skit_slice errtxt = skit_text_stream_slurp( &err_stream, NULL );
	
	/* HACK: TODO: text_stream doesn't specify that it holds text in a nul-terminated string. */
	/* This can be easily fixed when string formatters are written that handle skit_slice's. */
	sTHROW( ecode, (char*)sSPTR(errtxt) );
sEND_SCOPE

/* ------------------------- generic unittests ----------------------------- */

// The given stream has the contents "foo\n\n\0bar\nbaz\n"
void skit_stream_readln_unittest(skit_stream *stream)
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
void skit_stream_read_unittest(skit_stream *stream)
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
	printf("  skit_stream_appendln_unittest passed.\n");
}

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
	printf("  skit_stream_appendfln_unittest passed.\n");
}

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
	printf("  skit_stream_append_unittest passed.\n");
}

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
	printf("  skit_stream_rewind_unittest passed.\n");
}
