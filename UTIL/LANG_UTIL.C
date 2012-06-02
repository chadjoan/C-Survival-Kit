#include "LANG_UTIL.H"
#include <stdlib.h> /* For size_t */
#include <stdio.h>  /* For snprintf */
#include <unistd.h> /* For ssize_t */

frame_info __frame_info_stack[FRAME_INFO_STACK_SIZE];
size_t     __frame_info_end = 0;

/* TODO: use malloc instead of global/static stuff. */
#define MSG_BUF_SIZE 32768
static char msg_buf[MSG_BUF_SIZE];

char *lang_util_stack_to_str()
{
	printf("stack_to_str()\n");
	char *msg_pos = 0;
	ssize_t msg_rest_length = MSG_BUF_SIZE;
	ssize_t i;
	for ( i = __frame_info_end-1; i >= 0 ; i-- )
	{
		frame_info fi = __frame_info_stack[i];
		printf("i = %d;\n",i);
		printf("fi.file_name   = %s\n", fi.file_name);
		printf("fi.line_number = %d\n", fi.line_number);
		printf("fi.func_name   = %s\n", fi.func_name);
		printf("msg_buf = \n%s\n", msg_buf);
		ssize_t nchars = snprintf(
			msg_pos, msg_rest_length,
			"%s(%d): %s\r\n",
			fi.file_name,
			fi.line_number,
			fi.func_name);
		if ( nchars < 0 )
			continue;
		msg_pos += nchars;
		msg_rest_length -= nchars;
		if ( msg_rest_length < 0 )
			break;
	}
	
	return msg_buf;
}
