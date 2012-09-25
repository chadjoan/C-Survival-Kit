
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h"
#include "survival_kit/streams/file_stream.h"
#include "survival_kit/streams/pfile_stream.h"
#include "survival_kit/streams/rms_stream.h"

/* These haven't been needed yet.  I'm not going to bother testing them unless they get used.
int skit_stream_num_vtables = 0;
skit_stream_vtable_t *skit_stream_vtables = NULL;

void skit_stream_register_vtable(skit_stream_vtable_t *vtable, stream_type)
{
	skit_stream_num_vtables++;
	if ( skit_stream_num_vtables <= 1 )
		skit_stream_vtables = skit_malloc(sizeof(*skit_stream_vtable_t));
	else
		skit_stream_vtables = skit_realloc(skit_stream_num_vtables * sizeof(*skit_stream_vtable_t));

	skit_stream_vtables[skit_stream_num_vtables-1] = vtable;
}
*/
