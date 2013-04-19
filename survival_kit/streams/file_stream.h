
#ifndef SKIT_STREAMS_FILE_STREAM_INCLUDED
#define SKIT_STREAMS_FILE_STREAM_INCLUDED

typedef union skit_file_stream skit_file_stream;
union skit_file_stream
{
	skit_stream_metadata meta;
	skit_stream          as_stream;
};

void skit_file_stream_module_init();

#endif
