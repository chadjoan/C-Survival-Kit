
#include "survival_kit/streams/stream.h"

void skit_pfile_stream_vtable_init(skit_stream_vtable_t *table)
{
	skit_stream_vtable_init(table);
	table->init          = &skit_text_stream_init;
	table->read_line     = &skit_text_stream_read_line;
	table->read_bytes    = &skit_text_stream_read_bytes;
	table->write_line    = &skit_text_stream_write_line;
	table->write_bytes   = &skit_text_stream_write_bytes;
	table->flush         = &skit_text_stream_flush;
	table->rewind        = &skit_text_stream_rewind;
	table->slurp         = &skit_text_stream_slurp;
	table->to_slice      = &skit_text_stream_to_slice;
	table->dump          = &skit_text_stream_dump;
}