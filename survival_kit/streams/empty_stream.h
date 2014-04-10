
#ifndef SKIT_STREAMS_EMPTY_STREAM_INCLUDED
#define SKIT_STREAMS_EMPTY_STREAM_INCLUDED

#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/ind_stream.h"

typedef struct skit_empty_stream_internal skit_empty_stream_internal;
struct skit_empty_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
};

typedef union skit_empty_stream skit_empty_stream;
union skit_empty_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_empty_stream_internal as_internal;
};

void skit_empty_stream_module_init();

// Internal use stuff.  Macro backing.
skit_empty_stream *skit__empty_stream_cached();

extern skit_ind_stream *skit__ind_empty_cache;

/// These expose the standard stdout, stdin, and stderr file streams as pointers
/// to skit_pfile_stream objects.  They are statically allocated on an as-needed
/// basis and should never have their destructors called or be closed or free'd in
/// any way. 
#define skit_raw_empty_stream_instance (skit__empty_stream_cached())

#define skit_ind_empty_stream_instance (skit__ind_stream_cached(&skit_raw_empty_stream_instance->as_stream, &skit__ind_empty_cache ))

#define skit_empty_stream_instance (&skit_ind_empty_stream_instance->as_stream)

/**
Constructor
Allocates a new skit_empty_stream and calls skit_empty_stream_ctor(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_empty_stream_ctor(*)
directly.
*/
skit_empty_stream *skit_empty_stream_new();

/**
Casts the given stream into an empty stream.
This will return NULL if the given stream isn't actually a skit_empty_stream.
*/
skit_empty_stream *skit_empty_stream_downcast(const skit_stream *stream);

void skit_empty_stream_ctor(skit_empty_stream *estream);

skit_slice skit_empty_stream_readln(skit_empty_stream *stream, skit_loaf *buffer);
skit_slice skit_empty_stream_read(skit_empty_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_empty_stream_read_fn(skit_empty_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_empty_stream_appendln(skit_empty_stream *stream, skit_slice line);

void skit_empty_stream_appendf(skit_empty_stream *stream, const char *fmtstr, ... );
void skit_empty_stream_appendf_va(skit_empty_stream *stream, const char *fmtstr, va_list vl );
void skit_empty_stream_append(skit_empty_stream *stream, skit_slice slice);
void skit_empty_stream_flush(skit_empty_stream *stream);
void skit_empty_stream_rewind(skit_empty_stream *stream);
skit_slice skit_empty_stream_slurp(skit_empty_stream *stream, skit_loaf *buffer);
skit_slice skit_empty_stream_to_slice(skit_empty_stream *stream, skit_loaf *buffer);
void skit_empty_stream_dump(const skit_empty_stream *stream, skit_stream *output);
void skit_empty_stream_dtor(skit_empty_stream *stream);

void skit_empty_stream_unittests();

#endif

