
#ifndef SKIT_STREAMS_VTABLE_INCLUDED
#define SKIT_STREAMS_VTABLE_INCLUDED

#include "survival_kit/string.h"
//#include "survival_kit/streams/stream.h"
//#include "survival_kit/streams/file_stream.h"

union skit_stream;
union skit_file_stream;

typedef struct skit_stream_vtable_t skit_stream_vtable_t;
struct skit_stream_vtable_t
{
	/* Basic stream operations. */
	void       (*init)        (skit_stream*);
	skit_slice (*read_line)   (skit_stream*);
	skit_slice (*read_bytes)  (skit_stream*);
	void       (*write_line)  (skit_stream*,skit_slice);
	void       (*write_bytes) (skit_stream*,skit_slice);
	void       (*flush)       (skit_stream*);
	void       (*rewind)      (skit_stream*);
	skit_slice (*slurp)       (skit_stream*);
	skit_slice (*to_slice)    (skit_stream*);
	void       (*dump)        (skit_stream*,skit_stream*);
	
	/* File stream operations. */
	void       (*open)        (skit_file_stream*,skit_slice,const char*);
	void       (*close)       (skit_file_stream*);
};

typedef struct skit_stream_metadata skit_stream_metadata;
struct skit_stream_metadata
{
	/* Inheritence/polymorphism stuff. */
	skit_slice            class_name;
	skit_stream_vtable_t  *vtable_ptr;
};

/* These haven't been needed yet.
extern int skit_stream_num_vtables;
extern skit_stream_vtable_t *skit_stream_vtables[];

void skit_stream_register_vtable();
*/

#define SKIT_STREAM_DISPATCH(stream, func_name, ...) \
	((stream)->meta.vtable_ptr->func_name(stream, ##__VA_ARGS__))

#endif
