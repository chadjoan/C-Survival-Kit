
#if defined(__DECC)
#pragma module skit_streams_file_stream
#endif

#include "survival_kit/streams/stream.h"

static int skit_file_stream_initialized = 0;
static skit_stream_vtable_t skit_file_stream_vtable;

void skit_file_stream_vtable_init(skit_stream_vtable_t *table)
{
	/* Invalid method calls to an abstract class. */
	skit_stream_vtable_init(table);
}

void skit_file_stream_static_init()
{
	if ( skit_file_stream_initialized )
		return;
	
	skit_file_stream_initialized = 1;
	skit_file_stream_vtable_init(&skit_file_stream_vtable);
}
