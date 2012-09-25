
#include "survival_kit/streams/stream.h"
#include "survival_kit/streams/text_stream.h"
#include "survival_kit/streams/file_stream.h"
#include "survival_kit/streams/pfile_stream.h"

void skit_stream_static_init_all()
{
	skit_stream_vtable_init      (&skit_stream_vtables[SKIT_CLASS_STREAM]);
	skit_text_stream_vtable_init (&skit_stream_vtables[SKIT_CLASS_TEXT_STREAM]);
	skit_file_stream_vtable_init (&skit_stream_vtables[SKIT_CLASS_FILE_STREAM]);
	skit_pfile_stream_vtable_init(&skit_stream_vtables[SKIT_CLASS_PFILE_STREAM]);
	astd_rms_stream_vtable_init  (&astd_stream_vtables[SKIT_CLASS_RMS_STREAM]);
}