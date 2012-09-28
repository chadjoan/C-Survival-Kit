
#ifndef SKIT_STREAMS_VTABLE_INCLUDED
#define SKIT_STREAMS_VTABLE_INCLUDED

#include "survival_kit/string.h"

union skit_stream;
union skit_file_stream;

typedef struct skit_stream_vtable_t skit_stream_vtable_t;
struct skit_stream_vtable_t
{
	/* Basic stream operations. */
	skit_slice (*readln)      (union skit_stream*,skit_loaf*);
	skit_slice (*read)        (union skit_stream*,skit_loaf*,size_t);
	void       (*writeln)     (union skit_stream*,skit_slice);
	void       (*writefln_va) (union skit_stream*,const char*,va_list);
	void       (*write)       (union skit_stream*,skit_slice);
	void       (*flush)       (union skit_stream*);
	void       (*rewind)      (union skit_stream*);
	skit_slice (*slurp)       (union skit_stream*,skit_loaf*);
	skit_slice (*to_slice)    (union skit_stream*,skit_loaf*);
	void       (*dump)        (union skit_stream*,union skit_stream*);
	void       (*dtor)        (union skit_stream*);
	
	/* File stream operations. */
	void       (*open)        (union skit_file_stream*,skit_slice,const char*);
	void       (*close)       (union skit_file_stream*);
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

#define SKIT_STREAM_DISPATCH(...) SKIT_MACRO_DISPATCHER3(SKIT_STREAM_DISPATCH, __VA_ARGS__)(__VA_ARGS__)

#define SKIT_STREAM_DISPATCH1(stream) \
	(char * "Invalid use of SKIT_STREAM_DISPATCH macro.")

#define SKIT_STREAM_DISPATCH2(stream, func_name) \
	((stream)->meta.vtable_ptr->func_name(stream))

#define SKIT_STREAM_DISPATCH3(stream, func_name, ...) \
	((stream)->meta.vtable_ptr->func_name(stream, __VA_ARGS__))

#endif
