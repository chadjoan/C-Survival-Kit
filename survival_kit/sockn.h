#ifndef SOCKN_INCLUDED
#define SOCKN_INCLUDED

#include "survival_kit/str.h"
#include "survival_kit/features.h"

/* Sends 'text' over 'sock'.  The length of 'text' will become the first 
4 bytes of the message, stored in network-byte-order.  Use recvn to
receive a message sent with sendn.
Returns the number of bytes sent, or 0 if there is a failure. */
ssize_t sendn(int sock, string text);

/* 
Receives a message of arbitrary length from 'sock'.  The length is 
determined from the sender's side and is encoded as an integer in the first
4 bytes of the message in network-byte-order.   This is used to receive 
messages sent with sendn.

The argument 'buf' must be non-NULL.  buf->ptr however, may be NULL, and
this will cause recvn to allocate the necessary amount of memory to receive
the message entirely.  If buf->ptr is non-NULL, then buf's memory will be
reallocated to the correct size and then used to store the message.
All of this means that buf->ptr may be changed by recvn, and the caller
will always own the resulting memory and is responsible for calling str_delete
on it later.

Returns a string with a NULL ptr if there was an error reading the message 
length.  
*/
void recvn(int sock, string *buf);

ssize_t sendnf(int sock, char *fmtstr, ...);


#endif
