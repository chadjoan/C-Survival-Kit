
#if defined(__DECC)
#pragma module skit_streams_init
#endif

#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h"
#include "survival_kit/streams/file_stream.h"
#include "survival_kit/streams/pfile_stream.h"

void skit_stream_static_init_all()
{
	skit_stream_static_init();
	skit_text_stream_static_init();
	skit_file_stream_static_init();
	skit_pfile_stream_static_init();
}
