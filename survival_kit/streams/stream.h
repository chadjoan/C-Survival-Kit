
#ifndef SKIT_STREAMS_STREAM_INCLUDED
#define SKIT_STREAMS_STREAM_INCLUDED

#include "survival_kit/streams/meta.h"

typedef struct skit_stream_common_fields skit_stream_common_fields;
struct skit_stream_common_fields
{
	char   indent_char;
	short  indent_level;
};


typedef struct skit_text_stream_internal skit_text_stream_internal;
struct skit_text_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 buffer;
	skit_slice                text;
};

typedef union skit_stream skit_stream;
union skit_stream
{
	skit_stream_metadata meta;
};

void skit_stream_vtable_init(skit_stream_vtable_t *table);

#endif
