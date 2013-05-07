
#if defined(__DECC)
#pragma module skit_streams_tcp_stream
#endif

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#if defined(__DECC)
/* On VMS, this isn't defined in sys/socket.h like the standard says. */
/* We'll have to do it ourselves. */
/* (It doesn't seem to appear in any of the system header files at all... */
/*    even Java defines its own.) */
typedef size_t socklen_t; 
#endif

#include "survival_kit/assert.h"
#include "survival_kit/memory.h"
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/tcp_stream.h"
#include "survival_kit/string.h"

#define SKIT_STREAM_T skit_tcp_stream
#define SKIT_VTABLE_T skit_stream_vtable_tcp
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

/* ------------------------------------------------------------------------- */

static skit_loaf *skit_tcp_get_read_buffer( skit_tcp_stream_internal *tstreami, skit_loaf *arg_buffer )
{
	return skit_stream_get_read_buffer(&(tstreami->read_buffer), arg_buffer);
}

/* ------------------------------------------------------------------------- */

static size_t skit_tcp_read_bytes( skit_tcp_stream *stream, void *dst, size_t nbytes )
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_stream_internal *tstreami = &(stream->as_internal);

	/* Read the next chunk of bytes from the file. */
	ssize_t nbytes_read = recv( tstreami->connection_fd, dst, nbytes, 0 );
	if ( nbytes_read < 0 )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
	
	/* TODO: this could be a place where EOS detection is placed. */
	/* I am, as of yet, unsure of how to do this in a sane manner, if it's even possible. --Chad. */
	
	return nbytes_read;
}

/* ------------------------------------------------------------------------- */

static int skit_tcp_stream_initialized = 0;
static skit_stream_vtable_t skit_tcp_stream_vtable;

void skit_tcp_stream_vtable_init(skit_stream_vtable_t *arg_table)
{
	skit_stream_vtable_init(arg_table);
	skit_stream_vtable_tcp *table = (skit_stream_vtable_tcp*)arg_table;
	table->readln        = &skit_tcp_stream_readln;
	table->read          = &skit_tcp_stream_read;
	table->read_fn       = &skit_tcp_stream_read_fn;
	table->appendln      = &skit_tcp_stream_appendln;
	table->appendf_va    = &skit_tcp_stream_appendf_va;
	table->append        = &skit_tcp_stream_append;
	table->flush         = &skit_tcp_stream_flush;
	table->rewind        = &skit_tcp_stream_rewind;
	table->slurp         = &skit_tcp_stream_slurp;
	table->to_slice      = &skit_tcp_stream_to_slice;
	table->dump          = &skit_tcp_stream_dump;
	table->dtor          = &skit_tcp_stream_dtor;
	table->close         = &skit_tcp_stream_close;
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_module_init()
{
	if ( skit_tcp_stream_initialized )
		return;

	skit_tcp_stream_initialized = 1;
	skit_tcp_stream_vtable_init(&skit_tcp_stream_vtable);
}

/* ------------------------------------------------------------------------- */

skit_tcp_stream *skit_tcp_stream_new()
{
	skit_tcp_stream *result = skit_malloc(sizeof(skit_tcp_stream));
	skit_tcp_stream_ctor(result);
	return result;
}

/* ------------------------------------------------------------------------- */

skit_tcp_stream *skit_tcp_stream_downcast(const skit_stream *stream)
{
	sASSERT(stream != NULL);
	if ( stream->meta.vtable_ptr == &skit_tcp_stream_vtable )
		return (skit_tcp_stream*)stream;
	else
		return NULL;
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_ctor(skit_tcp_stream *tstream)
{
	skit_stream *stream = &(tstream->as_stream);
	skit_stream_ctor(stream);
	stream->meta.vtable_ptr = &skit_tcp_stream_vtable;
	stream->meta.class_name = sSLICE("skit_tcp_stream");
	
	skit_tcp_stream_internal *tstreami = &tstream->as_internal;
	tstreami->read_buffer = skit_loaf_null();
	tstreami->connection_fd = -1;
	tstreami->socket_fd = -1;
}

/* ------------------------------------------------------------------------- */

skit_slice skit_tcp_stream_readln(skit_tcp_stream *stream, skit_loaf *buffer)
{
	SKIT_USE_FEATURE_EMULATION;
	sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "readln not implemented for TCP/IP right now.  Use read instead."));
	return skit_slice_null();
}

/* ------------------------------------------------------------------------- */

skit_slice skit_tcp_stream_read(skit_tcp_stream *stream, skit_loaf *buffer, size_t nbytes)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	skit_tcp_stream_internal *tstreami = &(stream->as_internal);
	skit_loaf *read_buf;
	
	/* Freak out when weird crap happens. */
	if ( tstreami->connection_fd <= 0 )
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Attempt to read from an unopened stream."));
	
	/* Figure out which buffer to use. */
	read_buf = skit_tcp_get_read_buffer(tstreami, buffer);
	
	/* The provided buffer might not be large enough. */
	/* Thankfully, the necessary size is easy to calculate in this case. */
	if ( sLLENGTH(*read_buf) < nbytes )
		skit_loaf_resize(read_buf, nbytes);
	
	/* No net I/O for the weird 0-byte requests.  This prevents segfaulting. */
	if ( nbytes == 0 )
		return skit_slice_of(read_buf->as_slice, 0, 0);
	
	/* Do the read. */
	size_t nbytes_read = skit_tcp_read_bytes( stream, sLPTR(*read_buf), nbytes );
	
	/* EOS. */
	if ( nbytes_read == 0 )
		return skit_slice_null();
	
	return skit_slice_of(read_buf->as_slice, 0, nbytes_read);
}

/* ------------------------------------------------------------------------- */

skit_slice skit_tcp_stream_read_fn(skit_tcp_stream *stream, skit_loaf *buffer, void *context, int (*accept_char)( skit_custom_read_context *ctx ))
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	skit_tcp_stream_internal *tstreami = &(stream->as_internal);
	skit_loaf *read_buf;
	skit_custom_read_context ctx;
	
	/* Freak out when weird crap happens. */
	if ( tstreami->connection_fd <= 0 )
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Attempt to read from an unopened stream."));
	
	/* Figure out which buffer to use. */
	read_buf = skit_tcp_get_read_buffer(tstreami, buffer);
	ssize_t length = sLLENGTH(*read_buf);
	skit_utf8c *buf_ptr = sLPTR(*read_buf);
	ctx.caller_context = context; /* Pass the caller's context along. */
	
	/* Grab 1 character at a time and feed it to the caller. */
	int nbytes = 0;
	int done = 0;
	while ( !done )
	{
		/* Grab a byte. */
		skit_utf8c c;
		int success = skit_tcp_read_bytes(stream, &c, 1);
		
		/* Check for those times when the other end hung up. */
		if ( !success )
			break;
		
		/* Upsize buffer as needed. */
		nbytes++;
		if ( length < nbytes )
		{
			/* Slow, but w/e. */
			skit_loaf_resize(read_buf, nbytes+64);
			length = sLLENGTH(*read_buf);
			buf_ptr = sLPTR(*read_buf);
		}
		
		/* Place the new character into the read buffer. */
		buf_ptr[nbytes-1] = c;
		
		/* Push the character to the caller. */
		ctx.current_char = c;
		ctx.current_slice = skit_slice_of(read_buf->as_slice, 0, nbytes);
		if ( !accept_char(&ctx) )
			break;
	}
	
	return ctx.current_slice;
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_appendln(skit_tcp_stream *stream, skit_slice line)
{
	sASSERT(stream != NULL);
	/* The error handling is handled by subsequent calls. */
	skit_tcp_stream_append(stream, line);
	skit_tcp_stream_append(stream, sSLICE("\n"));
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_appendf(skit_tcp_stream *stream, const char *fmtstr, ... )
{
	va_list vl;
	va_start(vl, fmtstr);
	skit_tcp_stream_appendf_va(stream, fmtstr, vl);
	va_end(vl);
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_appendf_va(skit_tcp_stream *stream, const char *fmtstr, va_list vl )
{
	SKIT_USE_FEATURE_EMULATION;
	const size_t buf_size = 1024;
	char buffer[buf_size];
	size_t nchars_printed = 0;
	
	sASSERT(stream != NULL);
	sASSERT(fmtstr != NULL);
	
	skit_tcp_stream_internal *tstreami = &(stream->as_internal);
	if( tstreami->connection_fd <= 0 )
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &(stream->as_stream), "Attempt to write to an unopened stream."));
	
	nchars_printed = vsnprintf(buffer, buf_size, fmtstr, vl);
	skit_tcp_stream_append(stream, skit_slice_of_cstrn(buffer, nchars_printed));
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_append(skit_tcp_stream *stream, skit_slice slice)
{
	SKIT_USE_FEATURE_EMULATION;
	sASSERT(stream != NULL);
	
	skit_tcp_stream_internal *tstreami = &(stream->as_internal);
	if( tstreami->connection_fd <= 0 )
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &(stream->as_stream), "Attempt to write to an unopened stream."));
	
	ssize_t length = sSLENGTH(slice);
	
	/* Optimize this case. */
	if ( length == 0 )
		return;
	
	/* Do the write. */
	ssize_t nbytes_sent = send( tstreami->connection_fd, sSPTR(slice), length, 0 );
	if ( nbytes_sent < 0 )
	{
		char errbuf[1024];
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &(stream->as_stream), skit_errno_to_cstr(errbuf, sizeof(errbuf))));
	}
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_flush(skit_tcp_stream *stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "It is impossible to flush TCP streams."));
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_rewind(skit_tcp_stream *stream)
{
	SKIT_USE_FEATURE_EMULATION;
	sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "It is impossible to rewind TCP streams."));
}

/* ------------------------------------------------------------------------- */

static size_t skit_tcp_slurp_source(void *context, void *sink, size_t requested_chunk_size)
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_stream *stream = context;

	/* Read the next chunk of bytes from the file. */
	return sETRACE(skit_tcp_read_bytes( stream, sink, requested_chunk_size ));
}

skit_slice skit_tcp_stream_slurp(skit_tcp_stream *stream, skit_loaf *buffer)
{
	SKIT_USE_FEATURE_EMULATION;
	
	sASSERT(stream != NULL);
	skit_tcp_stream_internal *tstreami = &(stream->as_internal);
	skit_loaf *read_buf;
	
	/* Figure out which buffer to use. */
	read_buf = skit_tcp_get_read_buffer(tstreami, buffer);
	
	/* Delegate the ugly stuff to the skit_stream_buffered_slurp function. */
	return sETRACE(skit_stream_buffered_slurp(stream, read_buf, &skit_tcp_slurp_source));
}

/* ------------------------------------------------------------------------- */

skit_slice skit_tcp_stream_to_slice(skit_tcp_stream *stream, skit_loaf *buffer)
{
	sASSERT(stream != NULL);
	return stream->meta.class_name;
}

/* ------------------------------------------------------------------------- */
static void skit_tcp_dump_addr( const char *name, struct sockaddr_in *addr_struct, skit_stream *output )
{
	char errbuf[1024];
	char ipbuf[SKIT_MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
	const char *ipaddr = inet_ntop(AF_INET, &addr_struct->sin_addr, ipbuf, sizeof(ipbuf));
	if ( ipaddr )
		skit_stream_appendf(output, "%6s IP:     '%s'\n", name, ipaddr);
	else
		skit_stream_appendf(output, "%6s IP:     '%s'\n", name, skit_errno_to_cstr(errbuf, sizeof(errbuf)));

	skit_stream_appendf(output, "%6s port:   %d\n", name, ntohs(addr_struct->sin_port));
}

void skit_tcp_stream_dump(const skit_tcp_stream *stream, skit_stream *output)
{
	char errbuf[1024];
	if ( skit_stream_dump_null(output, stream, sSLICE("NULL skit_text_stream\n")) )
		return;
	
	/* Check for improperly cast streams.  Downcast will make sure we have the right vtable. */
	skit_tcp_stream *tstream = skit_tcp_stream_downcast(&(stream->as_stream));
	if ( tstream == NULL )
	{
		skit_stream_appendln(output, sSLICE("skit_stream (Error: invalid call to skit_tcp_stream_dump() with a first argument that isn't a tcp stream.)"));
		return;
	}
	
	skit_tcp_stream_internal *tstreami = &tstream->as_internal;
	if ( tstreami->connection_fd <= 0 )
	{
		skit_stream_appendln(output, sSLICE("Unconnected skit_tcp_stream"));
		skit_stream_appendf(output, "connection_fd: %d\n", tstreami->connection_fd );
		return;
	}
	
	skit_stream_appendf(output, "Opened skit_tcp_stream with the following properties:\n");
	
	if ( tstreami->socket_fd > 0 )
		skit_stream_appendln(output, sSLICE("Stream is operating in server mode. (socket_fd > 0)") );
	else
		skit_stream_appendln(output, sSLICE("Stream is operating in client mode. (socket_fd == -1)") );
	
	skit_stream_appendf(output, "socket_fd:     %d\n", tstreami->socket_fd );
	skit_stream_appendf(output, "connection_fd: %d\n", tstreami->connection_fd );
	
	struct sockaddr_in addr_struct;
	socklen_t addr_len = sizeof(addr_struct);
	if ( 0 == getpeername(tstreami->connection_fd, (struct sockaddr*)&addr_struct, &addr_len ) )
		skit_tcp_dump_addr( "Peer", &addr_struct, output );
	else
	{
		skit_stream_appendln(output, sSLICE("Call to getpeername failed.  Cannot lookup peer IP info.") );
		skit_stream_appendf(output, "getpeername error: %s\n", skit_errno_to_cstr(errbuf, sizeof(errbuf)) );
	}
	
	if ( tstreami->socket_fd > 0 )
	{
		addr_len = sizeof(addr_struct);
		if ( 0 == getsockname(tstreami->socket_fd, (struct sockaddr*)&addr_struct, &addr_len) )
			skit_tcp_dump_addr( "Listen", &addr_struct, output );
		else
		{
			skit_stream_appendln(output, sSLICE("Call to getsockname failed.  Cannot lookup host IP info.") );
			skit_stream_appendf(output, "getsockname error: %s\n", skit_errno_to_cstr(errbuf, sizeof(errbuf)) );
		}
	}

	return;
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_dtor(skit_tcp_stream *stream)
{
	skit_tcp_stream_close(stream);
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_accept(skit_tcp_stream *stream, int socket_fd)
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_stream_internal *tstreami = &stream->as_internal;
	
	if ( tstreami->connection_fd > 0 )
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Asked to accept while already open!"));
	
	int connection_fd = accept(socket_fd, NULL, NULL);

	if(0 > connection_fd)
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Accept failed."));
	
	tstreami->connection_fd = connection_fd;
	tstreami->socket_fd = socket_fd;
}

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_connect(skit_tcp_stream *stream, skit_slice ip_addr, int port)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;

	/* Parse the address first.  This can fail and throw exceptions, */
	/*   but it does not need the socket to be opened for it to happen. */
	/* We can avoid some resource-cleanup worries by doing it first. */
	struct sockaddr_in addr_struct;
	int addr;
	
	memset(&addr_struct, 0, sizeof(addr_struct));
	addr_struct.sin_family = AF_INET;
	addr_struct.sin_port = htons(port);
	char *ip_as_cstr = skit_slice_dup_as_cstr(ip_addr);
	sSCOPE_EXIT(skit_free(ip_as_cstr));
	
	/* iP TO Number: the standard for parsing IPv4 and IPv6 addresses. */
	addr = inet_pton(AF_INET, ip_as_cstr, &addr_struct.sin_addr);
	
	if (addr < 0)
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Test Client: first argument to inet_pton is not a valid address family."));
	else if (0 == addr)
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Test Client: second argument to inet_pton does not contain a valid ip address."));

	/* Now that we've parsed the IP address, we should grab a file descriptor. */
	int connection_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (-1 == connection_fd)
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Test Client: Could not create TCP socket."));

	/* Make sure we free it if we don't use it. */
	sSCOPE_FAILURE(close(connection_fd)); 
	
	/* Now we connect.  For real, this time. */
	if (-1 == connect(connection_fd, (struct sockaddr *)&addr_struct, sizeof(addr_struct)))
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Test Client: connect failed."));

	/* Commit our newly obtained connection to the stream. */
	skit_tcp_stream_internal *tstreami = &stream->as_internal;
	tstreami->connection_fd = connection_fd;
	tstreami->socket_fd = -1; /* Make sure we indicate that we are client-side. */
sEND_SCOPE

/* ------------------------------------------------------------------------- */

void skit_tcp_stream_close(skit_tcp_stream *stream)
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_stream_internal *tstreami = &stream->as_internal;
	
	if ( tstreami->connection_fd < 0 )
		return;

	if (-1 == shutdown(tstreami->connection_fd, SHUT_RDWR))
		sTRACE(skit_stream_throw_exc(SKIT_TCP_IO_EXCEPTION, &stream->as_stream, "Could not shutdown connection."));

	close( tstreami->connection_fd );
	
	tstreami->connection_fd = -1;
	tstreami->socket_fd = -1;
}

/* ------------------------------------------------------------------------- */

static int skit_tcp_get_bound_socket(int port)
{
	SKIT_USE_FEATURE_EMULATION;
	
	struct sockaddr_in addr_struct;
	int socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(socket_fd == -1)
	{
		if ( errno == EMFILE )
			printf("EMFILE\n");
		else if ( errno == ENFILE )
			printf("ENFILE\n");
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: Could not create TCP socket.");
	}

	/* Enable address reuse.  This gets rid of the "address in use" errors. */
	int on = 1;
	if(-1 == setsockopt( socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ))
	{
		close(socket_fd);
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: setsockopt(SO_REUSEADDR) failed.");
	}

	/* Initialize this struct before passing it to bind. */
	memset(&addr_struct, 0, sizeof(addr_struct));

	addr_struct.sin_family = AF_INET;
	addr_struct.sin_port = htons(port);
	addr_struct.sin_addr.s_addr = INADDR_ANY;
	
	/* Bind and catch any "address in use" errors. */
	/* The caller will want to try again if that happens. */
	if(-1 == bind(socket_fd,(struct sockaddr *)&addr_struct, sizeof(addr_struct)))
	{
		if ( errno == EADDRINUSE )
		{
			close(socket_fd);
			printf("  Test Server: bind failed on port %d due to 'Address in use' error.\n", port);
			return -1;
		}
		else
			sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: bind failed.");
	}
	return socket_fd;
}

static int skit_start_tcp_test_server(int *port)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
	int socket_fd = -1;
	while ( 1 )
	{
		socket_fd = skit_tcp_get_bound_socket(*port);
		if ( socket_fd != -1 )
			break;

		sleep(1);
		(*port)++;
		printf("  Test Server: Trying port %d.\n", *port);
	}

	sSCOPE_FAILURE(close(socket_fd));

	if(-1 == listen(socket_fd, 10))
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: listen failed.");
	
	sRETURN(socket_fd);
sEND_SCOPE

static int skit_connect_tcp_test_server(int socket_fd)
{
	SKIT_USE_FEATURE_EMULATION;
	char errbuf[1024];
	
	int connection_fd = accept(socket_fd, NULL, NULL);
	if (0 > connection_fd)
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: Accept failed.");
		
/*
	// This does not work on VMS.  No idea why.  Use ioctl instead.
	// (The fcntl calls will return success values but the socket will
	//    block on recv in the skit_tcp_utest_contents function)
	int flags = fcntl(connection_fd, F_GETFL, 0);
	if (0 > flags || 0 > fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK))
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: Could not set non-blocking mode.  Reason:\n%s",
			skit_errno_to_cstr(errbuf, sizeof(errbuf)));
*/

	/* Blocking is bad for unittests.  Turn that junk off. */
	int dont_block = 1;
	if (0 > ioctl(connection_fd, FIONBIO, &dont_block))
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: Could not set non-blocking mode.  Reason:\n%s",
			skit_errno_to_cstr(errbuf, sizeof(errbuf)));

	return connection_fd;
}

/* ------------------------------------------------------------------------- */

static skit_tcp_stream *skit_start_tcp_test_client(int port)
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_stream *result = skit_tcp_stream_new();
	sTRACE(skit_tcp_stream_ctor(result));
	sTRACE(skit_tcp_stream_connect(result, sSLICE("127.0.0.1"), port));
	return result;
}

static void skit_stop_tcp_test_client(skit_tcp_stream *stream)
{
	skit_stream_free(&stream->as_stream);
}

/* ------------------------------------------------------------------------- */

typedef struct skit_tcp_utest_context skit_tcp_utest_context;
struct skit_tcp_utest_context
{
	int server_fd;
	skit_loaf *content_buffer;
};

static skit_slice skit_tcp_utest_contents( void *context, int expected_size )
{
	SKIT_USE_FEATURE_EMULATION;
	
	skit_tcp_utest_context *ctx = context;
	char errbuf[1024];
	
	/* How many hundredths of a second to wait before timing out
	on receiving the expected unittest value. */
	int hundredths_max = 200; /* 2 seconds. */
	
	/* How many hundredths of a second we have spent waiting
	for the data we want. */
	int hundredths_waited = 0;
	
	/* Number of bytes received. */
	ssize_t received = 0;
	
	/* Sleep to allow packets to catch up and coalesce. */
	/* VMS seems to require up to 200 ms for this to happen. */
	/* Linux seems instantaneous. */
	/* We intentionally introduce a minimum wait to catch situations */
	/*   where the stream might send too many bytes: in those cases */
	/*   we could exit too early if we don't wait for the OS to flush */
	/*   all of its sockets, and end up with a deceptive pass. */
	#ifdef __VMS
	int hundredths_min = 25; /* 250 ms to be safe. */
	#else
	int hundredths_min = 5; /* For now, this handles Linux, which is instant. */
	#endif
	
	usleep(hundredths_min * 10 * 1000); 
	hundredths_waited += hundredths_min;
	
	/* Repeatedly peek at the bytes we've received. */
	/* When we've got enough, we'll return them to the test. */
	while ( hundredths_waited < hundredths_max )
	{
		/* Do a non-blocking peek at the receiving end. */
		/* This should yield whatever has been written by append operations
		    on the other side of the unittests. */
		received = recv(
			ctx->server_fd,
			sLPTR(*ctx->content_buffer),
			sLLENGTH(*ctx->content_buffer),
			MSG_PEEK );
		
		/* We expect to receive nothing sometimes. */
		/* In those cases recv may issue a EWOULDBLOCK error */
		/*   because receiving any amount of bytes from an empty buffer */
		/*   would require the call to block and wait for the buffer to fill. */
		/* We don't care, so we just ignore the error code. */
		if ( received == -1 && errno == EWOULDBLOCK )
			received = 0;
		
		/* Other stuff is bad maybe? */
		if ( received == -1 && errno != EWOULDBLOCK )
			sTHROW(SKIT_TCP_IO_EXCEPTION, "Test server: %s", skit_errno_to_cstr(errbuf, sizeof(errbuf)) );
		
		/* Good! We got all the bytes we came for. */
		if ( received >= expected_size )
			break;
		
		/* Sleep for 10ms (1/100 sec) to allow packets to catch up and coalesce. */
		/* Otherwise the unittest might see PART of the test results arriving. */
		/* This seems to actually matter on OpenVMS. */
		usleep(10 * 1000);
		
		hundredths_waited++;
	}
	/* If we time-out, we'll just return whatever we got. */
	/* If it's not good enough, then an assertion will trigger and tests */
	/*   will fail. (as we want them to, if something is wrong) */
	
	if ( hundredths_waited > hundredths_min )
	{
		printf("\n");
		printf("The system had to wait longer than expected for tcp packets to arrive.\n");
		printf("This could result in false positives in the tests.\n");
		printf("Please adjust hundredths_min so that it is safely larger than the amount of\n");
		printf("  of time required for tcp packets to travel to localhost on this system.\n");
		printf("(On this test, it waited %d +/- 10 milliseconds.)\n\n", hundredths_waited*10);
	}
	
	skit_slice result = skit_slice_of(ctx->content_buffer->as_slice, 0, received);
	return result;
}

static void skit_tcp_run_read_utest(
	int *test_port,
	skit_slice server_sent_text,
	void (*utest_function)(
		skit_stream *stream,
		void *context,
		skit_slice (*get_stream_contents)(void *context, int expected_size) )
	)
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_utest_context ctx;
	SKIT_LOAF_ON_STACK(content_buffer, 1024);
	ctx.content_buffer = &content_buffer;
	
	int socket_fd = sETRACE(skit_start_tcp_test_server(test_port));
	skit_tcp_stream *client = sETRACE(skit_start_tcp_test_client(*test_port));
	ctx.server_fd = sETRACE(skit_connect_tcp_test_server(socket_fd));
	
	sTRACE(send( ctx.server_fd, sSPTR(server_sent_text), sSLENGTH(server_sent_text), 0 ));
	
	if (-1 == shutdown(ctx.server_fd, SHUT_RDWR))
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: Could not shutdown server_fd.");
	
	sTRACE(utest_function(&(client->as_stream), &ctx, &skit_tcp_utest_contents));
	
	sTRACE(skit_stop_tcp_test_client(client));
	close(ctx.server_fd);
	close(socket_fd);
	(*test_port)++;
	
	skit_loaf_free(&content_buffer);
}

static void skit_tcp_run_write_utest(
	int *test_port,
	skit_slice server_sent_text,
	void (*utest_function)(
		skit_stream *stream,
		void *context,
		skit_slice (*get_stream_contents)(void *context, int expected_size) )
	)
{
	SKIT_USE_FEATURE_EMULATION;
	skit_tcp_utest_context ctx;
	SKIT_LOAF_ON_STACK(content_buffer, 1024);
	ctx.content_buffer = &content_buffer;
	
	int socket_fd = sETRACE(skit_start_tcp_test_server(test_port));
	skit_tcp_stream *client = sETRACE(skit_start_tcp_test_client(*test_port));
	ctx.server_fd = sETRACE(skit_connect_tcp_test_server(socket_fd));
	
	sTRACE(send( ctx.server_fd, sSPTR(server_sent_text), sSLENGTH(server_sent_text), 0 ));

	sTRACE(utest_function(&(client->as_stream), &ctx, &skit_tcp_utest_contents));
	
	sTRACE(skit_stop_tcp_test_client(client));

	/* This shutdown comes at a different place than in the read test. */
	/* This is important because you'll get a broken pipe if you try */
	/*   to write stuff out to a server that already shut down. */
	if (-1 == shutdown(ctx.server_fd, SHUT_RDWR))
		sTHROW(SKIT_TCP_IO_EXCEPTION, "Test Server: Could not shutdown server_fd.");
	close(ctx.server_fd);
	close(socket_fd);
	
	(*test_port)++;
	
	skit_loaf_free(&content_buffer);
}
	

void skit_tcp_stream_unittests()
{
	printf("skit_tcp_stream_unittests()\n");
	int test_port = 13373;
	
	/* Not implemented. */
	/*skit_tcp_run_read_utest(&test_port, sSLICE(SKIT_READLN_UNITTEST_CONTENTS),    &skit_stream_readln_unittest);*/
	
	skit_tcp_run_read_utest (&test_port, sSLICE(SKIT_READ_UNITTEST_CONTENTS),       &skit_stream_read_unittest);
	skit_tcp_run_read_utest (&test_port, sSLICE(SKIT_READ_XNN_UNITTEST_CONTENTS),   &skit_stream_read_xNN_unittest);
	skit_tcp_run_read_utest (&test_port, sSLICE(SKIT_READ_FN_UNITTEST_CONTENTS),    &skit_stream_read_fn_unittest);
	skit_tcp_run_write_utest(&test_port, sSLICE(SKIT_APPENDLN_UNITTEST_CONTENTS),   &skit_stream_appendln_unittest);
	skit_tcp_run_write_utest(&test_port, sSLICE(SKIT_APPENDF_UNITTEST_CONTENTS),    &skit_stream_appendf_unittest);
	skit_tcp_run_write_utest(&test_port, sSLICE(SKIT_APPEND_UNITTEST_CONTENTS),     &skit_stream_append_unittest);
	skit_tcp_run_write_utest(&test_port, sSLICE(SKIT_APPEND_XNN_UNITTEST_CONTENTS), &skit_stream_append_xNN_unittest);
	
	/* Not possible. */
	/* skit_tcp_run_write_utest(sSLICE(SKIT_REWIND_UNITTEST_CONTENTS),    &skit_stream_rewind_unittest); */

	printf("  skit_tcp_stream_unittests passed!\n");
	printf("\n");
}


