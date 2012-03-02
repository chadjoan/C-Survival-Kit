#ifndef SOCKN_INCLUDED
#define SOCKN_INCLUDED

#include "STR_UTIL.H"

/* Sends 'text' over 'sock'.  The length of 'text' will become the first 
4 bytes of the message, stored in network-byte-order.  Use recvn to
receive a message sent with sendn.
Returns the number of bytes sent, or 0 if there is a failure. */
ssize_t sendn(int sock, string text);

/* Receives a message of arbitrary length from 'sock'.  The length is 
determined from the sender's side and is encoded as an integer in the first
4 bytes of the message in network-byte-order.  The returned string will be
all of the bytes following that length information.  The memory needed will 
be allocated by this function, so be sure to free it up later.  This is used
to receive messages sent with sendn.
Returns a string with a NULL ptr if there was an error reading the message 
length.  */
string recvn(int sock);

ssize_t sendnf(int sock, char *fmtstr, ...);


#endif
