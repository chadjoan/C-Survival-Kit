
#include <fcntl.h>
#include "survival_kit/streams/stream.h"

void skit_pfile_stream_vtable_init(skit_stream_vtable_t *table)
{
	skit_stream_vtable_init(table);
	table->init          = &skit_pfile_stream_init;
	table->read_line     = &skit_pfile_stream_read_line;
	table->read_bytes    = &skit_pfile_stream_read_bytes;
	table->write_line    = &skit_pfile_stream_write_line;
	table->write_bytes   = &skit_pfile_stream_write_bytes;
	table->flush         = &skit_pfile_stream_flush;
	table->rewind        = &skit_pfile_stream_rewind;
	table->slurp         = &skit_pfile_stream_slurp;
	table->to_slice      = &skit_pfile_stream_to_slice;
	table->dump          = &skit_pfile_stream_dump;
	table->open          = &skit_pfile_stream_open;
	table->close         = &skit_pfile_stream_close;
}