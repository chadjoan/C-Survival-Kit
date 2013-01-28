
#ifndef SKIT_STREAMS_PFILE_STREAM_INCLUDED
#define SKIT_STREAMS_PFILE_STREAM_INCLUDED

#include <stdio.h>

#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/file_stream.h"

typedef struct skit_pfile_stream_internal skit_pfile_stream_internal;
struct skit_pfile_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 name;
	skit_loaf                 read_buffer;
	skit_loaf                 access_mode;
	FILE                      *file_handle;
	int                       handle_owned_by_stream;
};

/** File stream backed by POSIX style file I/O or equivalent. */
typedef union skit_pfile_stream skit_pfile_stream;
union skit_pfile_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_file_stream           as_file_stream;
	skit_pfile_stream_internal as_internal;
};

void skit_pfile_stream_static_init();

/* Internal use stuff.  Macro backing. */
extern skit_pfile_stream *skit__pfile_stdout_cache;
extern skit_pfile_stream *skit__pfile_stdin_cache;
extern skit_pfile_stream *skit__pfile_stderr_cache;
skit_pfile_stream *skit__pfile_stream_cached( FILE *file_handle, skit_pfile_stream **cached_stream, skit_slice name );

/** 
These expose the standard stdout, stdin, and stderr file streams as pointers
to skit_pfile_stream objects.  They are statically allocated on an as-needed
basis and should never have their destructors called or be closed or free'd in
any way. 
*/
#define skit_pfile_stream_stdout (skit__pfile_stream_cached(stdout, &skit__pfile_stdout_cache, sSLICE("stdout")))
#define skit_pfile_stream_stdin  (skit__pfile_stream_cached(stdin,  &skit__pfile_stdin_cache,  sSLICE("stdin")))
#define skit_pfile_stream_stderr (skit__pfile_stream_cached(stderr, &skit__pfile_stderr_cache, sSLICE("stderr")))

#define skit_stream_stdout (&skit_pfile_stream_stdout->as_stream)
#define skit_stream_stdin  (&skit_pfile_stream_stdin->as_stream)
#define skit_stream_stderr (&skit_pfile_stream_stderr->as_stream)

/**
Constructor
Allocates a new skit_pfile_stream and calls skit_pfile_stream_init(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_pfile_stream_init(*)
directly.
*/
skit_pfile_stream *skit_pfile_stream_new();

/**
Casts the given stream into a pfile stream.
This will return NULL if the given stream isn't actually a skit_pfile_stream.
*/
skit_pfile_stream *skit_pfile_stream_downcast(const skit_stream *stream);

void skit_pfile_stream_init(skit_pfile_stream *stream);
skit_slice skit_pfile_stream_readln(skit_pfile_stream *stream, skit_loaf *buffer);
skit_slice skit_pfile_stream_read(skit_pfile_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_pfile_stream_read_fn(skit_pfile_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_pfile_stream_appendln(skit_pfile_stream *stream, skit_slice line);
void skit_pfile_stream_appendf(skit_pfile_stream *stream, const char *fmtstr, ... );
void skit_pfile_stream_appendf_va(skit_pfile_stream *stream, const char *fmtstr, va_list vl );
void skit_pfile_stream_append(skit_pfile_stream *stream, skit_slice slice);
void skit_pfile_stream_flush(skit_pfile_stream *stream);
void skit_pfile_stream_rewind(skit_pfile_stream *stream);
skit_slice skit_pfile_stream_slurp(skit_pfile_stream *stream, skit_loaf *buffer);
skit_slice skit_pfile_stream_to_slice(skit_pfile_stream *stream, skit_loaf *buffer);
void skit_pfile_stream_dump(const skit_pfile_stream *stream, skit_stream *output);
void skit_pfile_stream_dtor(skit_pfile_stream *stream);
void skit_pfile_stream_close(skit_pfile_stream *stream);

/* OpenVMS has it's own very special fopen function with variadic args.
It can't be asked to provide a version with a va_list argument, so it is
necessary to implement this as a C99 variadic macro that can forward the
arguments to the OpenVMS fopen. */
#ifdef __VMS
#define skit_pfile_stream_open(stream,fname,...) \
	skit__pfile_stream_assign_fp(stream, fopen(skit__pfile_stream_populate(stream, fname, SKIT_PFILE_FIRST_VARG(__VA_ARGS__)), __VA_ARGS__))
#else
#define skit_pfile_stream_open(stream,fname,...) \
	skit__pfile_stream_assign_fp(stream, fopen(skit__pfile_stream_populate(stream, fname, SKIT_PFILE_FIRST_VARG(__VA_ARGS__)), SKIT_PFILE_FIRST_VARG(__VA_ARGS__)))
#endif

/* Internal use functions that are used in the skit_pfile_stream_open macro. */
/* Do not use directly. */
const char *skit__pfile_stream_populate(skit_pfile_stream *stream, skit_slice fname, const char *access_mode);
void skit__pfile_stream_assign_fp(skit_pfile_stream *stream, FILE *fp);
#define SKIT_PFILE_FIRST_VARG(...) SKIT_PFILE_FIRST_VARG_(__VA_ARGS__, throwaway)
#define SKIT_PFILE_FIRST_VARG_(first,...) first

void skit_pfile_stream_unittests();

#endif
