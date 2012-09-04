
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "survival_kit/sockn.h"
#include "survival_kit/str.h"
#include "survival_kit/features.h"


ssize_t sendn(int sock, string text)
{
	uint32_t length_nwb = htonl(text.length);
	ssize_t n_bytes_sent = 0;
	
	n_bytes_sent = send(sock, &length_nwb, 4, 0);
	if ( n_bytes_sent != 4) {
		return 0;
	}
	
	n_bytes_sent = send(sock, text.ptr, text.length, 0);
	return n_bytes_sent;
}

void recvn(int sock, string *buf)
{
	int64_t received = -1;
	char buffer[4];
	size_t msg_length = 0;
	
	char *msg_start = NULL;
	char *msg_cursor = NULL;
	char *msg_end = NULL;
	
	/* Receive message */
	if ((received = recv(sock, buffer, 4, 0)) < 0)
		RAISE(SOCKET_EXCEPTION,"recvn: Length information not received. recv() call failed with error code %d.", received);
	
	if ( received < 4 )
		RAISE(SOCKET_EXCEPTION,"recvn: Length information not received. Received %d bytes instead.", received);

	msg_length = ntohl(((uint32_t*)buffer)[0]);
	buf = str_realloc(buf, msg_length);
	
	msg_start = buf->ptr;
	msg_cursor = msg_start;
	msg_end = msg_start + msg_length;
	
	while (1)
	{
		
		/* Check for more data */
		if ((received = recv(sock, msg_cursor, msg_end - msg_cursor, 0)) < 0)
			RAISE(SOCKET_EXCEPTION,"recvn: Not enough bytes received to complete message.", received);
		
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
	string sendme = str_literal(response);
	n_bytes_sent = sendn(sock, sendme);
	if (n_bytes_sent != response_length) {
		die("Failed to send bytes to client");
	}
	str_delete(&sendme);
	return n_bytes_sent;
}
