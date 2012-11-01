
#ifndef SKIT_STREAMS_TCP_STREAM_INCLUDED
#define SKIT_STREAMS_TCP_STREAM_INCLUDED

#include "survival_kit/streams/stream.h"

typedef struct skit_tcp_stream_internal skit_tcp_stream_internal;
struct skit_tcp_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 read_buffer;
	int socket_fd;
	int connection_fd;
};

typedef union skit_tcp_stream skit_tcp_stream;
union skit_tcp_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_tcp_stream_internal   as_internal;
};

void skit_tcp_stream_static_init();

/**
Constructor
Allocates a new skit_tcp_stream and calls skit_tcp_stream_init(*) on it.
If the caller wishes to stack-allocate the new instance, then they do not need
to call this function.  They must instead call skit_tcp_stream_init(*)
directly.
*/
skit_tcp_stream *skit_tcp_stream_new();

/**
Casts the given stream into a tcp stream.
This will return NULL if the given stream isn't actually a skit_tcp_stream.
*/
skit_tcp_stream *skit_tcp_stream_downcast(skit_stream *stream);

void skit_tcp_stream_init(skit_tcp_stream *tstream);
skit_slice skit_tcp_stream_readln(skit_tcp_stream *stream, skit_loaf *buffer);
skit_slice skit_tcp_stream_read(skit_tcp_stream *stream, skit_loaf *buffer, size_t nbytes);
skit_slice skit_tcp_stream_read_fn(skit_tcp_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ));
void skit_tcp_stream_appendln(skit_tcp_stream *stream, skit_slice line);

/** TODO: the number of characters that can be written this way is currently
limited to 1024.  This restriction should be lifted in the future, assuming
sufficient programming time/resources to do so. */
void skit_tcp_stream_appendfln(skit_tcp_stream *stream, const char *fmtstr, ... );
void skit_tcp_stream_appendfln_va(skit_tcp_stream *stream, const char *fmtstr, va_list vl );
void skit_tcp_stream_append(skit_tcp_stream *stream, skit_slice slice);
void skit_tcp_stream_flush(skit_tcp_stream *stream);
void skit_tcp_stream_rewind(skit_tcp_stream *stream);
skit_slice skit_tcp_stream_slurp(skit_tcp_stream *stream, skit_loaf *buffer);
skit_slice skit_tcp_stream_to_slice(skit_tcp_stream *stream, skit_loaf *buffer);
void skit_tcp_stream_dump(skit_tcp_stream *stream, skit_stream *output);
void skit_tcp_stream_dtor(skit_tcp_stream *stream);

/**
This serves the same function as the Berkeley sockets accept() function, but
populates a skit_tcp_stream instead of returning a connection file descriptor.
The stream will free its own connection file descriptor when _dtor is called.
The stream will NOT free the socket_fd: that is the caller's responsibility.
*/
void skit_tcp_stream_accept(skit_tcp_stream *stream, int socket_fd);

/**
This serves the same function as the Berkeley sockets connect() function, but
populates a skit_tcp_stream instead of returning a connection file descriptor.

The port given here should be in host/native byte order, not network byte order
as would be expected by Berkeley socket functions.  This is for convenience.
*/
void skit_tcp_stream_connect(skit_tcp_stream *stream, skit_slice ip_addr, int port);


void skit_tcp_stream_close(skit_tcp_stream *stream);

void skit_tcp_stream_unittests();

#endif
