
#ifndef SKIT_STREAMS_PFILE_STREAM_INCLUDED
#define SKIT_STREAMS_PFILE_STREAM_INCLUDED

#include <stdio.h>

#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/file_stream.h"
#include "survival_kit/streams/ind_stream.h"

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

void skit_pfile_stream_module_init();

/* Internal use stuff.  Macro backing. */
extern skit_pfile_stream *skit__pfile_stdout_cache;
extern skit_pfile_stream *skit__pfile_stdin_cache;
extern skit_pfile_stream *skit__pfile_stderr_cache;
skit_pfile_stream *skit__pfile_stream_cached( FILE *file_handle, skit_pfile_stream **cached_stream, skit_slice name );

extern skit_ind_stream *skit__ind_stdout_cache;
extern skit_ind_stream *skit__ind_stdin_cache;
extern skit_ind_stream *skit__ind_stderr_cache;

/** 
These expose the standard stdout, stdin, and stderr file streams as pointers
to skit_pfile_stream objects.  They are statically allocated on an as-needed
basis and should never have their destructors called or be closed or free'd in
any way. 
*/
#define skit_pfile_stream_stdout (skit__pfile_stream_cached(stdout, &skit__pfile_stdout_cache, sSLICE("stdout")))
#define skit_pfile_stream_stdin  (skit__pfile_stream_cached(stdin,  &skit__pfile_stdin_cache,  sSLICE("stdin")))
#define skit_pfile_stream_stderr (skit__pfile_stream_cached(stderr, &skit__pfile_stderr_cache, sSLICE("stderr")))

#define skit_ind_stream_stdout (skit__ind_stream_cached(&skit_pfile_stream_stdout->as_stream, &skit__ind_stdout_cache ))
#define skit_ind_stream_stdin  (skit__ind_stream_cached(&skit_pfile_stream_stdin->as_stream , &skit__ind_stdin_cache  ))
#define skit_ind_stream_stderr (skit__ind_stream_cached(&skit_pfile_stream_stderr->as_stream, &skit__ind_stderr_cache ))

#define skit_stream_stdout (&skit_ind_stream_stdout->as_stream)
#define skit_stream_stdin  (&skit_ind_stream_stdin->as_stream)
#define skit_stream_stderr (&skit_ind_stream_stderr->as_stream)

/**
Allocates a new skit_pfile_stream and calls skit_pfile_stream_ctor(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_pfile_stream_ctor(*)
directly.
*/
skit_pfile_stream *skit_pfile_stream_new();

/**
Casts the given stream into a pfile stream.
This will return NULL if the given stream isn't actually a skit_pfile_stream.
*/
skit_pfile_stream *skit_pfile_stream_downcast(const skit_stream *stream);

void skit_pfile_stream_ctor(skit_pfile_stream *stream);
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

// As of this writing, this macro symbol is defined nowhere.
// It is placed as a way to indicate that the given function definition is
// not supposed to exist, because it is actually implemented as a macro.
// In the future, a documentation generator might define a macro symbol like
// this to allow itself to separate the user-readable function definition
// from the macro details.
#ifdef SKIT__DOCUMENTATION_ONLY
/// Opens the file at the given path, using the given pfile stream for all
/// subsequent read/write operations.
/// The access mode is the same as given in POSIX specifications for fopen,
/// For details on path handling and the mode parameter, see:
///   http://pubs.opengroup.org/onlinepubs/009695399/functions/fopen.html
///   or
///   http://www.cplusplus.com/reference/cstdio/fopen/
void skit_pfile_stream_open(skit_pfile_stream *stream, skit_slice file_path, const char *mode);
#endif

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

/// Opens a file stream using the given handle.
/// Unlike with skit_pfile_stream_open, the caller will be responsible for
/// calling fclose/pclose/whatever on the file_handle.
/// The caller must still call skit_pfile_stream_close when they are done.
/// NOTE: It is odd that the name and access_mode must be supplied for an
///   already-opened file_handle.  Perhaps there is a better way?
///   This might change in the future: these arguments may be removed.
void skit_pfile_stream_use_handle(
	skit_pfile_stream *pstream,
	FILE *file_handle,
	skit_slice name,
	const char *access_mode );

/// Releases any file-handling resources related to the stream.
/// This does not free the stream object itself, nor does it free any internal
/// buffers used by the stream.  Thus, it is possible to efficiently re-use
/// a pfile stream on multiple files and minimize the amount of malloc/free
/// calls per-file.
void skit_pfile_stream_close(skit_pfile_stream *stream);

/* Internal use functions that are used in the skit_pfile_stream_open macro. */
/* Do not use directly. */
const char *skit__pfile_stream_populate(skit_pfile_stream *stream, skit_slice fname, const char *access_mode);
void skit__pfile_stream_assign_fp(skit_pfile_stream *stream, FILE *fp);
#define SKIT_PFILE_FIRST_VARG(...) SKIT_PFILE_FIRST_VARG_(__VA_ARGS__, throwaway)
#define SKIT_PFILE_FIRST_VARG_(first,...) first

/// This function implements convenient one-line file slurp functionality.
/// It will place the contents of the file at 'file_path' into the given
/// 'buffer' and return the slice that corresponds to the contents (the buffer
/// might be slightly larger for the sake of efficient buffering).
///
/// Under the hood, this will use a skit_pfile_stream to do the dirty work.
///
/// Unlike many of the stream functions that take a buffer argument, this
/// buffer is mandatory, and the returned slice WILL be a slice of that buffer.
/// This allows the caller to persist the text buffer after its originating
/// stream has long since been closed and freed.
skit_slice skit_pfile_slurp_into( skit_slice file_path, skit_loaf *buffer );

void skit_pfile_stream_unittests();

#endif
