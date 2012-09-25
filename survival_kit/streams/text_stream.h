
#include "survival_kit/streams/stream.h"

typedef struct skit_text_stream_internal skit_text_stream_internal;
struct skit_text_stream_internal
{
	skit_stream_metadata meta;
	skit_stream_common_fields common_fields;
	skit_loaf   buffer;
	skit_slice  text;
};

typedef union skit_text_stream skit_text_stream;
union skit_text_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_text_stream_internal  as_internal;
};