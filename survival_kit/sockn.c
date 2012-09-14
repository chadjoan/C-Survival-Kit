
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "survival_kit/misc.h"
#include "survival_kit/sockn.h"
#include "survival_kit/string.h"
#include "survival_kit/feature_emulation.h"


ssize_t sendn(int sock, skit_string text)
{
	ssize_t text_len = skit_string_len(text);
	uint32_t length_nwb = htonl(text_len);
	ssize_t n_bytes_sent = 0;
	
	n_bytes_sent = send(sock, &length_nwb, 4, 0);
	if ( n_bytes_sent != 4) {
		return 0;
	}
	
	n_bytes_sent = send(sock, text.chars, text_len, 0);
	return n_bytes_sent;
}

void recvn(int sock, skit_loaf *buf)
{
	SKIT_USE_FEATURE_EMULATION;

	int64_t received = -1;
	char buffer[4];
	size_t msg_length = 0;
	
	skit_utf8c *msg_start = NULL;
	skit_utf8c *msg_cursor = NULL;
	skit_utf8c *msg_end = NULL;
	
	/* Receive message */
	if ((received = recv(sock, buffer, 4, 0)) < 0)
		sTHROW(SOCKET_EXCEPTION,"recvn: Length information not received. recv() call failed with error code %d.", received);
	
	if ( received < 4 )
		sTHROW(SOCKET_EXCEPTION,"recvn: Length information not received. Received %d bytes instead.", received);

	msg_length = ntohl(((uint32_t*)buffer)[0]);
	buf = skit_loaf_resize(buf, msg_length);
	
	msg_start = buf->chars;
	msg_cursor = msg_start;
	msg_end = msg_start + msg_length;
	
	while (1)
	{
		
		/* Check for more data */
		if ((received = recv(sock, msg_cursor, msg_end - msg_cursor, 0)) < 0)
			sTHROW(SOCKET_EXCEPTION,"recvn: Not enough bytes received to complete message.", received);
		
		if ( received == 0 )
			break;
		
		if ( msg_cursor + received > msg_end )
		{
			printf("Error: received too many bytes: %ld\n", msg_cursor + received - msg_end);
			received = msg_end - msg_cursor;
		}
		
		/* memcpy(msg_cursor, buffer, received); */
		msg_cursor += received;
		
		/* Exit when we've got the message. */
		/* If we don't do this, we'll wait forever for bytes that never come. */
		/* TODO: what if the client doesn't send all the bytes it says it will?
		     It would create a deadlock situation.  There should be some kind
		     of timeout that can break the deadlock. */
		if ( msg_cursor == msg_end )
			break;
	}
}

ssize_t sendnf(int sock, char *fmtstr, ...)
{
	ssize_t n_bytes_sent = 0;
	char response[1024];
	int response_length = 0;
	va_list vl;
	va_start(vl, fmtstr);
	response_length = vsnprintf(response,1024,fmtstr,vl);
	va_end(vl);
	skit_loaf sendme = skit_loaf_copy_cstr(response);
	n_bytes_sent = sendn(sock, sendme.as_string);
	if (n_bytes_sent != response_length) {
		skit_die("Failed to send bytes to client");
	}
	skit_loaf_free(&sendme);
	return n_bytes_sent;
}
