
/**
A stream that indents text that is appended to it.
This stream will proxy all reads and writes to another stream that is assigned
at the time it is constructed.
*/

#ifndef SKIT_STREAMS_IND_STREAM_INCLUDED
#define SKIT_STREAMS_IND_STREAM_INCLUDED

#include "survival_kit/string.h"
#include "survival_kit/streams/stream.h"

#include <inttypes.h>

typedef struct skit_ind_stream_internal skit_ind_stream_internal;
struct skit_ind_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_stream               *backing_stream;

	// This buffer used to be stack allocated (with 1024 chars) and then
	// malloc'd only if the caller presented a larger format string.
	// This proved to be a liability on OpenVMS where per-thread stack memory
	// is fairly scarce (~40kB), so it has been moved into the stream's
	// internal object.  This should still be reasonably fast, since the
	// allocation is only likely to happen when indent streams are created,
	// which is infrequent.
	skit_loaf                 fmtstr_buf;

	// This bit is set when the skit_ind_stream instance owns its backing
	// stream, and thus must call skit_stream_free() on it whenever
	// the destructor is called.
	int                       owns_backing_stream;

	const char                *indent_str;
	int16_t                   indent_level;
	int16_t                   last_peak_level;
	char                      last_write_was_newline;
};

typedef union skit_ind_stream skit_ind_stream;
union skit_ind_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_ind_stream_internal   as_internal;
};

void skit_ind_stream_module_init();

// Internal use only.
// This is used by modules like pfile_stream and empty_stream as an easy way to
// create singletons that are indentable by default.
skit_ind_stream *skit__ind_stream_cached( skit_stream *src, skit_ind_stream **cached_stream );

/**
Allocates a new skit_ind_stream and calls skit_ind_stream_ctor(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_ind_stream_ctor(*)
directly.
*/
skit_ind_stream *skit_ind_stream_new(skit_stream *backing);

/**
Casts the given stream into an indented stream.
This will return NULL if the given stream isn't actually a skit_ind_stream.
*/
skit_ind_stream *skit_ind_stream_downcast(const skit_stream *stream);

/// Allows calling code to determine whether or not this skit_ind_stream
/// owns its backing stream.  If the skit_ind_stream owns its backing stream,
/// then it will call skit_stream_free on that stream whenever it is itself
/// freed using skit_stream_free or skit_stream_dtor.
///
/// By default, skit_ind_stream does not own its backing stream.
void skit_ind_stream_set_own(skit_ind_stream *stream, int has_backing_stream_ownership );

void skit_ind_stream_ctor(skit_ind_stream *istream, skit_stream *backing);
skit_slice skit_ind_stream_readln(skit_ind_stream *stream, skit_loaf *buffer);
skit_slice skit_ind_stream_read(skit_ind_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_ind_stream_read_fn(skit_ind_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_ind_stream_appendln(skit_ind_stream *stream, skit_slice line);

/** BUG: any newlines that appear from the expansion of format arguments will not be indented. */
/** (Only the formatstring itself will be subject to indentation replacement.) */
void skit_ind_stream_appendf(skit_ind_stream *stream, const char *fmtstr, ... );
void skit_ind_stream_appendf_va(skit_ind_stream *stream, const char *fmtstr, va_list vl );
void skit_ind_stream_append(skit_ind_stream *stream, skit_slice slice);
void skit_ind_stream_flush(skit_ind_stream *stream);
void skit_ind_stream_rewind(skit_ind_stream *stream);
skit_slice skit_ind_stream_slurp(skit_ind_stream *stream, skit_loaf *buffer);
skit_slice skit_ind_stream_to_slice(skit_ind_stream *stream, skit_loaf *buffer);
void skit_ind_stream_dump(const skit_ind_stream *stream, skit_stream *output);

void skit_ind_stream_incr_indent(skit_ind_stream *stream);
void skit_ind_stream_decr_indent(skit_ind_stream *stream);
int16_t skit_ind_stream_get_ind_lvl(const skit_ind_stream *stream);
int16_t skit_ind_stream_get_peak(const skit_ind_stream *stream);
const char *skit_ind_stream_get_ind_str(const skit_ind_stream *stream);
void skit_ind_stream_set_ind_str(skit_ind_stream *stream, const char *c);


/// Note that this does NOT free the backing stream by default.
/// To make it free the backing stream, first call
/// 'skit_ind_stream_set_own(stream,1)'.
void skit_ind_stream_dtor(skit_ind_stream *stream);

void skit_ind_stream_unittests();

#endif
