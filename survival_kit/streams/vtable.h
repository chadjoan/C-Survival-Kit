/**
Template vtable: a type-safe way to accomplish dispatch without implicit
	type conversion support from the language.

It is the caller's responsibility to undefine template parameters after 
#include'ing this file.

Parameters:

SKIT_STREAM_T - The first argument to all virtual functions in the table will
	have a type of SKIT_STREAM_T*.

SKIT_VTABLE_T - The desired type name for the vtable.  Ex: skit_stream_vtable
*/

typedef struct SKIT_VTABLE_T SKIT_VTABLE_T;
struct SKIT_VTABLE_T
{
	/* Basic stream operations. */
	skit_slice (*readln)       (SKIT_STREAM_T*,skit_loaf*);
	skit_slice (*read)         (SKIT_STREAM_T*,skit_loaf*,size_t);
	skit_slice (*read_fn)      (SKIT_STREAM_T*,skit_loaf*, void* context, int (*accept_char)(skit_custom_read_context *ctx));
	void       (*appendln)     (SKIT_STREAM_T*,skit_slice);
	void       (*appendf_va)   (SKIT_STREAM_T*,const char*,va_list);
	void       (*append)       (SKIT_STREAM_T*,skit_slice);
	void       (*flush)        (SKIT_STREAM_T*);
	void       (*rewind)       (SKIT_STREAM_T*);
	skit_slice (*slurp)        (SKIT_STREAM_T*,skit_loaf*);
	skit_slice (*to_slice)     (SKIT_STREAM_T*,skit_loaf*);
	void       (*dump)         (SKIT_STREAM_T*,skit_stream*);
	void       (*dtor)         (SKIT_STREAM_T*);
	
	/* File stream operations. */
	void       (*open)        (SKIT_STREAM_T*,skit_slice,const char*);
	void       (*close)       (SKIT_STREAM_T*);
};
