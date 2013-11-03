
#ifndef SKIT_STREAMS_TCP_STREAM_INCLUDED
#define SKIT_STREAMS_TCP_STREAM_INCLUDED

#include "survival_kit/streams/stream.h"

typedef struct skit_tcp_stream_internal skit_tcp_stream_internal;
struct skit_tcp_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 read_buffer;
	int listener_fd;
	int connection_fd;
};

typedef union skit_tcp_stream skit_tcp_stream;
union skit_tcp_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_tcp_stream_internal   as_internal;
};

void skit_tcp_stream_module_init();

/**
Constructor
Allocates a new skit_tcp_stream and calls skit_tcp_stream_ctor(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_tcp_stream_ctor(*)
directly.
*/
skit_tcp_stream *skit_tcp_stream_new();

/**
Casts the given stream into a tcp stream.
This will return NULL if the given stream isn't actually a skit_tcp_stream.
*/
skit_tcp_stream *skit_tcp_stream_downcast(const skit_stream *stream);

void skit_tcp_stream_ctor(skit_tcp_stream *tstream);
skit_slice skit_tcp_stream_readln(skit_tcp_stream *stream, skit_loaf *buffer);
skit_slice skit_tcp_stream_read(skit_tcp_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_tcp_stream_read_fn(skit_tcp_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_tcp_stream_appendln(skit_tcp_stream *stream, skit_slice line);

/** TODO: the number of characters that can be written this way is currently
limited to 1024.  This restriction should be lifted in the future, assuming
sufficient programming time/resources to do so. */
void skit_tcp_stream_appendf(skit_tcp_stream *stream, const char *fmtstr, ... );
void skit_tcp_stream_appendf_va(skit_tcp_stream *stream, const char *fmtstr, va_list vl );
void skit_tcp_stream_append(skit_tcp_stream *stream, skit_slice slice);
void skit_tcp_stream_flush(skit_tcp_stream *stream);
void skit_tcp_stream_rewind(skit_tcp_stream *stream);
skit_slice skit_tcp_stream_slurp(skit_tcp_stream *stream, skit_loaf *buffer);
skit_slice skit_tcp_stream_to_slice(skit_tcp_stream *stream, skit_loaf *buffer);
void skit_tcp_stream_dump(const skit_tcp_stream *stream, skit_stream *output);
void skit_tcp_stream_dtor(skit_tcp_stream *stream);

/**
This serves the same function as the Berkeley sockets accept() function, but
populates a skit_tcp_stream instead of returning a connection file descriptor.
The stream will free its own connection file descriptor when _dtor is called.
The stream will NOT free the socket_fd: that is the caller's responsibility.

This will throw a SKIT_TCP_IO_EXCEPTION if the underlying call to the Berkeley
sockets accept() fails.

This will throw a SKIT_TCP_IO_EXCEPTION if the given stream is already open.
*/
void skit_tcp_stream_accept(skit_tcp_stream *stream, int socket_fd);

/**
This serves the same function as the Berkeley sockets connect() function, but
populates a skit_tcp_stream instead of returning a connection file descriptor.

The port given here should be in host/native byte order, not network byte order
as would be expected by Berkeley socket functions.  This is for convenience.
*/
void skit_tcp_stream_connect(skit_tcp_stream *stream, skit_slice ip_addr, int port);

/**
Returns the underlying socket descriptor.

This can be useful when implementing any functionality that hasn't been
considered in the tcp stream interface (or the stream interface in general).

It is expected that the caller will leave the socket open and avoid doing
anything that would desync the stream's state and the socket's state.  In
other words: consider access to the returned socket to be read-only.
*/
int skit_tcp_stream_get_socket_fd(skit_tcp_stream *stream);

void skit_tcp_stream_close(skit_tcp_stream *stream);

void skit_tcp_stream_unittests();

#endif
