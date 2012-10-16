
#ifndef SKIT_STREAMS_META_INCLUDED
#define SKIT_STREAMS_META_INCLUDED

#include "survival_kit/string.h"

union skit_stream;

#define SKIT_STREAM_T void
#define SKIT_VTABLE_T skit_stream_vtable_t
#include "survival_kit/streams/vtable.h"
#undef SKIT_STREAM_T
#undef SKIT_VTABLE_T

typedef struct skit_stream_metadata skit_stream_metadata;
struct skit_stream_metadata
{
	/* Inheritence/polymorphism stuff. */
	skit_slice           class_name;
	skit_stream_vtable_t *vtable_ptr;
};

/* These haven't been needed yet.
extern int skit_stream_num_vtables;
extern skit_stream_vtable_t *skit_stream_vtables[];

void skit_stream_register_vtable();
*/

#define SKIT_STREAM_DISPATCH(...) SKIT_MACRO_DISPATCHER3(SKIT_STREAM_DISPATCH, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_STREAM_DISPATCH1(stream) \
	(char * "Invalid use of SKIT_STREAM_DISPATCH macro.")

#define SKIT_STREAM_DISPATCH2(stream, func_name) \
	((stream)->meta.vtable_ptr->func_name(stream))

#define SKIT_STREAM_DISPATCH3(stream, func_name, ...) \
	((stream)->meta.vtable_ptr->func_name(stream, __VA_ARGS__))

#endif
