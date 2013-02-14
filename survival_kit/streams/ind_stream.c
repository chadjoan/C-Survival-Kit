
#if defined(__DECC)
#pragma module skit_streams_ind_stream
#endif

#include <stdarg.h>
#include <stdio.h>
#include "survival_kit/assert.h"
#include "survival_kit/string.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/ind_stream.h"
#include "survival_kit/streams/text_stream.h" /* For unittesting. */

#define SKIT_STREAM_T skit_ind_stream
#define SKIT_VTABLE_T skit_stream_vtable_ind
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

/* ------------------------------------------------------------------------- */

static int skit_ind_stream_initialized = 0;
static skit_stream_vtable_t skit_ind_stream_vtable;

/* ------------------------------------------------------------------------- */

static void skit_ind_stream_vtable_init(skit_stream_vtable_t *arg_table)
{
	skit_stream_vtable_init(arg_table);
	skit_stream_vtable_ind *table = (skit_stream_vtable_ind*)arg_table;
	table->readln        = &skit_ind_stream_readln;
	table->read          = &skit_ind_stream_read;
	table->read_fn       = &skit_ind_stream_read_fn;
	table->appendln      = &skit_ind_stream_appendln;
	table->appendf_va    = &skit_ind_stream_appendf_va;
	table->append        = &skit_ind_stream_append;
	table->flush         = &skit_ind_stream_flush;
	table->rewind        = &skit_ind_stream_rewind;
	table->slurp         = &skit_ind_stream_slurp;
	table->to_slice      = &skit_ind_stream_to_slice;
	table->dump          = &skit_ind_stream_dump;
	table->dtor          = &skit_ind_stream_dtor;
	
	table->incr_indent   = &skit_ind_stream_incr_indent;
	table->decr_indent   = &skit_ind_stream_decr_indent;
	table->get_ind_lvl   = &skit_ind_stream_get_ind_lvl;
	table->get_ind_str   = &skit_ind_stream_get_ind_str;
	table->set_ind_str   = &skit_ind_stream_set_ind_str;
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_static_init()
{
	if ( skit_ind_stream_initialized )
		return;
	
	skit_ind_stream_initialized = 1;
	skit_ind_stream_vtable_init(&skit_ind_stream_vtable);
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_ctor(skit_ind_stream *istream, skit_stream *backing)
{
	skit_stream *stream = &istream->as_stream;
	skit_stream_ctor(stream);
	stream->meta.vtable_ptr = &skit_ind_stream_vtable;
	stream->meta.class_name = sSLICE("skit_ind_stream");
	
	skit_ind_stream_internal *istreami = &istream->as_internal;
	istreami->backing_stream = backing;
	istreami->indent_str = "\t";
	istreami->indent_level = 0;
	istreami->last_write_was_newline = 0;
}

/* ------------------------------------------------------------------------- */

skit_ind_stream *skit_ind_stream_downcast(const skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_ind_stream_vtable )
		return (skit_ind_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_ind_stream_readln(skit_ind_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	return skit_stream_readln(istreami->backing_stream, buffer);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_ind_stream_read(skit_ind_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	return skit_stream_read(istreami->backing_stream, buffer, nbytes);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_ind_stream_read_fn(skit_ind_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ))
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	return skit_stream_read_fn(istreami->backing_stream, buffer, context, accept_char);
}

/* ------------------------------------------------------------------------- */

static void skit_istream_append_indent(skit_ind_stream_internal *istreami)
{
	size_t i;
	for ( i = 0; i < istreami->indent_level; i++ )
		skit_stream_append(istreami->backing_stream, skit_slice_of_cstr(istreami->indent_str));
}

static void skit_istream_check_last_line(skit_ind_stream_internal *istreami)
{
	if ( istreami->last_write_was_newline )
		skit_istream_append_indent(istreami);
	
	istreami->last_write_was_newline = 0;
}

static void skit_istream_internal_append(skit_ind_stream_internal *istreami, skit_slice slice)
{
	size_t i;
	size_t end_of_last_newline = 0;
	size_t slice_len = sSLENGTH(slice);
	i = 0;
	while ( i < slice_len )
	{
		int nl_size = skit_slice_match_nl(slice, i);
		if ( nl_size )
		{
			/* Append everything up to the indent we now need to insert. */
			i += nl_size;
			skit_stream_append(istreami->backing_stream,
				skit_slice_of(slice, end_of_last_newline, i));
			end_of_last_newline = i;
			
			if ( i+nl_size < slice_len )
			{
				/* Insert the indent. */
				skit_istream_append_indent(istreami);
			}
			else
			{
				/* The user may change indentation levels later, before */
				/*   printing the next line.  Because of this fact, we can't */
				/*   immediately write indentation out.  We'll have to wait */
				/*   until they write more text.  We remember this waiting */
				/*   state by setting the last_write_was_newline flag. */
				istreami->last_write_was_newline = 1;
			}
		}
		else
			i++;
	}
	
	if ( end_of_last_newline != slice_len )
	{
		skit_stream_append(istreami->backing_stream,
			skit_slice_of(slice, end_of_last_newline, slice_len));
	}
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_appendln(skit_ind_stream *stream, skit_slice line)
{
	sASSERT(stream != NULL);
	sASSERT(sSPTR(line) != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	
	skit_istream_check_last_line(istreami);
	skit_istream_internal_append(istreami, line);
	skit_istream_check_last_line(istreami);
	skit_stream_appendln(istreami->backing_stream, sSLICE(""));
	
	/* The user may change indentation levels later, before */
	/*   printing the next line.  Because of this fact, we can't */
	/*   immediately write indentation out.  We'll have to wait */
	/*   until they write more text.  We remember this waiting */
	/*   state by setting the last_write_was_newline flag. */
	istreami->last_write_was_newline = 1;
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_appendf(skit_ind_stream *stream, const char *fmtstr, ...)
{
	va_list vl;
	va_start(vl, fmtstr);
	skit_ind_stream_appendf_va(stream, fmtstr, vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_appendf_va(skit_ind_stream *stream, const char *fmtstr, va_list vl)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	size_t n_newlines = 0;
	size_t i;
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	
	/* Count the number of newlines in the format string. */
	size_t fmtstr_len = strlen(fmtstr);
	skit_slice fmtstr_slice = skit_slice_of_cstrn(fmtstr, fmtstr_len);
	for ( i = 0; i < fmtstr_len; i++ )
	{
		int nl_size = skit_slice_match_nl(fmtstr_slice, i);
		if ( nl_size )
		{
			i += (nl_size-1);
			n_newlines++;
		}
	}
	
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	skit_istream_check_last_line(istreami);
	
	size_t indent_str_len = strlen(istreami->indent_str);
	
	/* Allocate space for a second format string: we will copy the fmtstr */
	/*   into it while substituting newlines with the appropriate amount */
	/*   of indentation. */
	SKIT_LOAF_ON_STACK(fmtstr2, 1024);
	sSCOPE_EXIT(skit_loaf_free(&fmtstr2));
	size_t fmtstr2_len = fmtstr_len + 
		(n_newlines *
		istreami->indent_level *
		indent_str_len);
	
	skit_loaf_resize(&fmtstr2, fmtstr2_len);
	
	char *fmtbuf = (char*)sLPTR(fmtstr2);
	size_t j = 0;
	size_t k;
	i = 0;
	while ( i < fmtstr_len )
	{
		int nl_size = skit_slice_match_nl(fmtstr_slice, i);
		if ( nl_size )
		{
			if ( i+nl_size < fmtstr_len )
			{
				/* Output the newline. */
				memcpy(&fmtbuf[j], &fmtstr[i], nl_size);
				j += nl_size;
				i += nl_size;
				
				/* Output the indent. */
				for ( k = 0; k < istreami->indent_level; k++ )
				{
					memcpy(&fmtbuf[j], istreami->indent_str, indent_str_len);
					j += indent_str_len;
				}
			}
			else
			{
				/* Output the newline. */
				memcpy(&fmtbuf[j], &fmtstr[i], nl_size);
				j += nl_size;
				i += nl_size;
				
				/* The user may change indentation levels later, before */
				/*   printing the next line.  Because of this fact, we can't */
				/*   immediately write indentation out.  We'll have to wait */
				/*   until they write more text.  We remember this waiting */
				/*   state by setting the last_write_was_newline flag. */
				istreami->last_write_was_newline = 1;
			}			
		}
		else /* Output a single non-newline character. */
			fmtbuf[j++] = fmtstr[i++];
	}
	
	skit_stream_appendf_va(istreami->backing_stream, fmtbuf, vl);
sEND_SCOPE

/* ------------------------------------------------------------------------- */

void skit_ind_stream_append(skit_ind_stream *stream, skit_slice slice)
{
	sASSERT(stream != NULL);
	sASSERT(sSPTR(slice) != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	skit_istream_check_last_line(istreami);
	
	skit_istream_internal_append(istreami, slice);
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_flush(skit_ind_stream *stream)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	skit_stream_flush(istreami->backing_stream);
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_rewind(skit_ind_stream *stream)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	skit_stream_rewind(istreami->backing_stream);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_ind_stream_slurp(skit_ind_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	return skit_stream_slurp(istreami->backing_stream, buffer);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_ind_stream_to_slice(skit_ind_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_dump(const skit_ind_stream *stream, skit_stream *output)
{
	if ( skit_stream_dump_null(output, stream, sSLICE("NULL skit_ind_stream\n")) )
		return;
	
	/* Check for improperly cast streams.  Downcast will make sure we have the right vtable. */
	skit_ind_stream *istream = skit_ind_stream_downcast(&(stream->as_stream));
	if ( istream == NULL )
	{
		skit_stream_appendln(output, sSLICE("skit_stream (Error: invalid call to skit_ind_stream_dump() with a first argument that isn't an indent stream.)"));
		return;
	}
	
	skit_ind_stream_internal *istreami = &istream->as_internal;
	if ( istreami->backing_stream == NULL )
	{
		skit_stream_appendln(output, sSLICE("Invalid skit_ind_stream with NULL backing_stream."));
		return;
	}
	
	skit_stream_appendf(output, "skit_ind_stream with the following properties:\n");
	skit_stream_appendf(output, "Indent String: \"%s\"\n", istreami->indent_str);
	skit_stream_appendf(output, "Indent Level:  %d\n", istreami->indent_level);
	skit_stream_appendf(output, "Last write was newline?: %d\n", istreami->last_write_was_newline);
	skit_stream_appendln(output, sSLICE("Wrapped around the following stream:"));
	skit_stream_dump(istreami->backing_stream, output);
	return;
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_incr_indent(skit_ind_stream *stream)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	(istreami->indent_level)++;
}

void skit_ind_stream_decr_indent(skit_ind_stream *stream)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	short ilevel = istreami->indent_level;
	if ( ilevel > 0 )
		ilevel--;
	istreami->indent_level = ilevel;
}

short skit_ind_stream_get_ind_lvl(const skit_ind_stream *stream)
{
	sASSERT(stream != NULL);
	const skit_ind_stream_internal *istreami = &(stream->as_internal);
	return istreami->indent_level;
}

const char *skit_ind_stream_get_ind_str(const skit_ind_stream *stream)
{
	sASSERT(stream != NULL);
	const skit_ind_stream_internal *istreami = &(stream->as_internal);
	return istreami->indent_str;
}

void skit_ind_stream_set_ind_str(skit_ind_stream *stream, const char *c)
{
	sASSERT(stream != NULL);
	skit_ind_stream_internal *istreami = &(stream->as_internal);
	istreami->indent_str = c;
}

/* ------------------------------------------------------------------------- */

void skit_ind_stream_dtor(skit_ind_stream *stream)
{
	// No resources are owned by an indent stream.
}

/* ------------------------------------------------------------------------- */

static skit_slice skit_ind_stream_utest_contents( void *context, int expected_size )
{
	skit_ind_stream *istream = context;
	skit_ind_stream_internal *istreami = &istream->as_internal;
	skit_text_stream *tstream = skit_text_stream_downcast(istreami->backing_stream);
	skit_text_stream_internal *tstreami = &tstream->as_internal;
	
	return tstreami->text;
}

static void skit_ind_stream_run_utest(
	void (*utest_function)(
		skit_stream *stream,
		void *context,
		skit_slice (*get_stream_contents)(void *context, int expected_size) ),
	skit_slice initial_contents
)
{
	skit_text_stream tstream;
	skit_ind_stream istream;
	skit_text_stream_init_str(&tstream, initial_contents);
	skit_ind_stream_ctor(&istream, &tstream.as_stream);
	skit_stream *stream = &istream.as_stream;
	
	utest_function(stream, stream, &skit_ind_stream_utest_contents);
	
	skit_ind_stream_dtor(&istream);
	skit_text_stream_dtor(&tstream);
}

void skit_ind_stream_unittests()
{
	printf("skit_ind_stream_unittests()\n");
	
	skit_ind_stream_run_utest(&skit_stream_readln_unittest,     sSLICE(SKIT_READLN_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_read_unittest,       sSLICE(SKIT_READ_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_read_xNN_unittest,   sSLICE(SKIT_READ_XNN_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_read_fn_unittest,    sSLICE(SKIT_READ_FN_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_appendln_unittest,   sSLICE(SKIT_APPENDLN_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_appendf_unittest,    sSLICE(SKIT_APPENDF_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_append_unittest,     sSLICE(SKIT_APPEND_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_append_xNN_unittest, sSLICE(SKIT_APPEND_XNN_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_rewind_unittest,     sSLICE(SKIT_REWIND_UNITTEST_CONTENTS));
	skit_ind_stream_run_utest(&skit_stream_indent_unittest,     sSLICE(SKIT_INDENT_UNITTEST_CONTENTS));
	
	printf("  skit_ind_stream_unittests passed!\n");
	printf("\n");
}
