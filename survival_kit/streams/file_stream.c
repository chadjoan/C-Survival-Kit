
#include "survival_kit/streams/stream.h"

void skit_file_stream_vtable_init(skit_stream_vtable_t *table)
{
	/* Invalid method calls to an abstract class. */
	skit_stream_vtable_init(table);
}

