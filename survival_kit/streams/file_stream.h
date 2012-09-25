

typedef union skit_file_stream skit_file_stream;
union skit_file_stream
{
	skit_stream_metadata meta;
	skit_stream          as_stream;
};