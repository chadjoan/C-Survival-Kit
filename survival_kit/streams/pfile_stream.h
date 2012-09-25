
#include <fcntl.h>

#include "survival_kit/streams/stream.h"

typedef struct skit_pfile_stream_internal skit_pfile_stream_internal;
struct skit_pfile_stream_internal
{
	skit_stream_metadata      meta;
	skit_stream_common_fields common_fields;
	skit_loaf                 name;
	skit_loaf                 err_msg_buf;
	int                       file_descriptor;
};

/** File stream backed by POSIX style file I/O or equivalent. */
typedef union skit_pfile_stream skit_pfile_stream;
union skit_pfile_stream
{
	skit_stream_metadata       meta;
	skit_stream                as_stream;
	skit_file_stream           as_file_stream;
	skit_pfile_stream_internal as_internal;
};